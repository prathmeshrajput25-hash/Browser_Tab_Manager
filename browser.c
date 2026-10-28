#include "browser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void browser_init(Browser *browser)
{
    if (browser != NULL) {
        (void)memset(browser, 0, sizeof(*browser));
        browser->nextId = 1;
    }
}

static void tab_free(Tab *tab)
{
    if (tab != NULL) {
        page_stack_free(tab->backStack);
        page_stack_free(tab->forwardStack);
        free(tab);
    }
}

static void browser_free_tabs(Tab *tab)
{
    while (tab != NULL) {
        Tab *next = tab->next;
        tab_free(tab);
        tab = next;
    }
}

static int page_stack_count(const Page *stack)
{
    int count = 0;
    while (stack != NULL) {
        ++count;
        stack = stack->next;
    }
    return count;
}

void browser_cleanup(Browser *browser)
{
    if (browser == NULL) {
        return;
    }
    browser_free_tabs(browser->head);
    browser->head = NULL;
    browser->active = NULL;
    browser_clear_history(browser);
    browser_clear_closed_tabs(browser);
}

Tab *browser_find_tab_by_id(const Browser *browser, int id)
{
    Tab *tab;
    if (browser == NULL) {
        return NULL;
    }
    for (tab = browser->head; tab != NULL; tab = tab->next) {
        if (tab->id == id) {
            return tab;
        }
    }
    return NULL;
}

Tab *browser_find_tab_by_title(const Browser *browser, const char *query)
{
    Tab *tab;
    if (browser == NULL || query == NULL || query[0] == '\0') {
        return NULL;
    }

    for (tab = browser->head; tab != NULL; tab = tab->next) {
        const unsigned char *text = (const unsigned char *)tab->title;
        const unsigned char *needle = (const unsigned char *)query;
        const unsigned char *start;
        while (*text != '\0') {
            start = text;
            needle = (const unsigned char *)query;
            while (*text != '\0' && *needle != '\0' &&
                   tolower(*text) == tolower(*needle)) {
                ++text;
                ++needle;
            }
            if (*needle == '\0') {
                return tab;
            }
            text = start + 1;
        }
    }
    return NULL;
}

int browser_tab_count(const Browser *browser)
{
    int count = 0;
    const Tab *tab;
    if (browser != NULL) {
        for (tab = browser->head; tab != NULL; tab = tab->next) {
            ++count;
        }
    }
    return count;
}

int browser_open_tab(Browser *browser, const char *title, const char *url)
{
    Tab *tab;
    Tab *tail;
    if (browser == NULL || title == NULL || title[0] == '\0' ||
        url == NULL || url[0] == '\0' || browser->nextId <= 0 ||
        browser->nextId == 2147483647) {
        return 0;
    }

    tab = (Tab *)calloc(1, sizeof(*tab));
    if (tab == NULL) {
        return 0;
    }
    tab->id = browser->nextId;
    (void)snprintf(tab->title, sizeof(tab->title), "%s", title);
    (void)snprintf(tab->currentURL, sizeof(tab->currentURL), "%s", url);

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
    ++browser->nextId;
    return tab->id;
}

/* Keep a deep copy on the closed-tab stack so navigation state can be restored. */
int browser_close_tab(Browser *browser, int id)
{
    Tab *tab;
    ClosedTab *closed;
    Tab *copy;

    if (browser == NULL || (tab = browser_find_tab_by_id(browser, id)) == NULL) {
        return 0;
    }

    copy = (Tab *)calloc(1, sizeof(*copy));
    closed = (ClosedTab *)malloc(sizeof(*closed));
    if (copy == NULL || closed == NULL) {
        free(copy);
        free(closed);
        return 0;
    }
    copy->id = tab->id;
    (void)memcpy(copy->title, tab->title, sizeof(copy->title));
    (void)memcpy(copy->currentURL, tab->currentURL, sizeof(copy->currentURL));
    if (tab->backStack != NULL) {
        copy->backStack = page_stack_clone(tab->backStack);
        if (copy->backStack == NULL) {
            free(copy);
            free(closed);
            return 0;
        }
    }
    if (tab->forwardStack != NULL) {
        copy->forwardStack = page_stack_clone(tab->forwardStack);
        if (copy->forwardStack == NULL) {
            page_stack_free(copy->backStack);
            free(copy);
            free(closed);
            return 0;
        }
    }
    closed->tab = copy;
    closed->next = browser->closedTabs;
    browser->closedTabs = closed;

    if (tab->prev != NULL) {
        tab->prev->next = tab->next;
    } else {
        browser->head = tab->next;
    }
    if (tab->next != NULL) {
        tab->next->prev = tab->prev;
    }
    if (browser->active == tab) {
        browser->active = tab->next != NULL ? tab->next : tab->prev;
    }
    tab_free(tab);
    return 1;
}

int browser_switch_tab(Browser *browser, int id)
{
    Tab *tab = browser_find_tab_by_id(browser, id);
    if (tab == NULL) {
        return 0;
    }
    browser->active = tab;
    return 1;
}

void browser_list_tabs(const Browser *browser)
{
    const Tab *tab;
    printf("\n+--------------------------- OPEN BROWSER TABS --------------------------+\n");
    if (browser == NULL || browser->head == NULL) {
        printf("| No tabs are currently open.                                            |\n");
    } else {
        for (tab = browser->head; tab != NULL; tab = tab->next) {
            printf("| %c ID: %-5d Title: %-18.18s URL: %-32.32s |\n",
                   tab == browser->active ? '>' : ' ', tab->id, tab->title,
                   tab->currentURL);
        }
    }
    printf("+------------------------------------------------------------------------+\n");
}

void browser_show_active_tab(const Browser *browser)
{
    if (browser == NULL || browser->active == NULL) {
        printf("\nNo active tab. Open a tab to get started.\n");
        return;
    }
    printf("\n+------------------------- ACTIVE TAB DETAILS --------------------------+\n");
    printf("| Tab ID : %-62d |\n", browser->active->id);
    printf("| Title  : %-62.62s |\n", browser->active->title);
    printf("| URL    : %-62.62s |\n", browser->active->currentURL);
    printf("| Back pages: %-58d |\n",
           page_stack_count(browser->active->backStack));
    printf("| Forward pages: %-55d |\n",
           page_stack_count(browser->active->forwardStack));
    printf("+------------------------------------------------------------------------+\n");
}
