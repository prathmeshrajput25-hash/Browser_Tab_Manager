#include "browser.h"

#include <stdio.h>
#include <stdlib.h>

static Page *page_create(const char *url)
{
    Page *page = (Page *)malloc(sizeof(*page));
    if (page == NULL) {
        return NULL;
    }

    (void)snprintf(page->url, sizeof(page->url), "%s", url);
    page->next = NULL;
    return page;
}

Page *page_stack_clone(const Page *stack)
{
    Page *copy = NULL;
    Page **tail = &copy;

    while (stack != NULL) {
        Page *page = page_create(stack->url);
        if (page == NULL) {
            page_stack_free(copy);
            return NULL;
        }
        *tail = page;
        tail = &page->next;
        stack = stack->next;
    }
    return copy;
}

void page_stack_free(Page *stack)
{
    while (stack != NULL) {
        Page *next = stack->next;
        free(stack);
        stack = next;
    }
}

void page_stack_clear(Page **stack)
{
    if (stack != NULL) {
        page_stack_free(*stack);
        *stack = NULL;
    }
}

static void closed_tab_free(Tab *tab)
{
    if (tab != NULL) {
        page_stack_free(tab->backStack);
        page_stack_free(tab->forwardStack);
        free(tab);
    }
}

int browser_restore_closed_tab(Browser *browser)
{
    ClosedTab *closed;
    Tab *tab;
    Tab *tail;

    if (browser == NULL || browser->closedTabs == NULL) {
        return 0;
    }

    closed = browser->closedTabs;
    tab = closed->tab;
    if (tab == NULL || browser_find_tab_by_id(browser, tab->id) != NULL) {
        return 0;
    }

    browser->closedTabs = closed->next;
    free(closed);

    tab->prev = NULL;
    tab->next = NULL;
    if (browser->head == NULL) {
        browser->head = tab;
    } else {
        tail = browser->head;
        while (tail->next != NULL) {
            tail = tail->next;
        }
        tail->next = tab;
        tab->prev = tail;
    }
    browser->active = tab;
    return 1;
}

void browser_clear_closed_tabs(Browser *browser)
{
    if (browser != NULL) {
        ClosedTab *closed = browser->closedTabs;
        while (closed != NULL) {
            ClosedTab *next = closed->next;
            closed_tab_free(closed->tab);
            free(closed);
            closed = next;
        }
        browser->closedTabs = NULL;
    }
}
