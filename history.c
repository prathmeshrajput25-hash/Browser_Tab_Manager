#include "browser.h"

#include <stdio.h>
#include <stdlib.h>

static int history_append(Browser *browser, const char *url)
{
    History *entry = (History *)malloc(sizeof(*entry));
    if (entry == NULL) {
        return 0;
    }
    (void)snprintf(entry->url, sizeof(entry->url), "%s", url);
    entry->next = NULL;

    if (browser->historyTail == NULL) {
        browser->historyHead = entry;
    } else {
        browser->historyTail->next = entry;
    }
    browser->historyTail = entry;
    return 1;
}

void browser_show_history(const Browser *browser)
{
    const History *entry;
    size_t index = 1U;

    printf("\n+--------------------------- BROWSING HISTORY ---------------------------+\n");
    if (browser == NULL || browser->historyHead == NULL) {
        printf("| No browsing history yet.                                               |\n");
    } else {
        for (entry = browser->historyHead; entry != NULL; entry = entry->next) {
            printf("| %3zu. %-67.67s |\n", index++, entry->url);
        }
    }
    printf("+------------------------------------------------------------------------+\n");
}

void browser_clear_history(Browser *browser)
{
    History *entry;
    if (browser == NULL) {
        return;
    }

    entry = browser->historyHead;
    while (entry != NULL) {
        History *next = entry->next;
        free(entry);
        entry = next;
    }
    browser->historyHead = NULL;
    browser->historyTail = NULL;
}

/* A fresh visit pushes the current page and discards any forward history. */
int browser_visit(Browser *browser, const char *url)
{
    Page *backPage = NULL;
    if (browser == NULL || browser->active == NULL || url == NULL || url[0] == '\0') {
        return 0;
    }

    if (browser->active->currentURL[0] != '\0') {
        backPage = (Page *)malloc(sizeof(*backPage));
        if (backPage == NULL) {
            return 0;
        }
        (void)snprintf(backPage->url, sizeof(backPage->url), "%s",
                       browser->active->currentURL);
        backPage->next = browser->active->backStack;
    }
    if (!history_append(browser, url)) {
        free(backPage);
        return 0;
    }

    if (backPage != NULL) {
        browser->active->backStack = backPage;
    }
    page_stack_clear(&browser->active->forwardStack);
    (void)snprintf(browser->active->currentURL,
                   sizeof(browser->active->currentURL), "%s", url);
    return 1;
}

int browser_go_back(Browser *browser)
{
    Page *destination;
    Page *forwardPage;

    if (browser == NULL || browser->active == NULL ||
        browser->active->backStack == NULL) {
        return 0;
    }

    destination = browser->active->backStack;
    forwardPage = (Page *)malloc(sizeof(*forwardPage));
    if (forwardPage == NULL) {
        return 0;
    }
    if (!history_append(browser, destination->url)) {
        free(forwardPage);
        return 0;
    }

    (void)snprintf(forwardPage->url, sizeof(forwardPage->url), "%s",
                   browser->active->currentURL);
    forwardPage->next = browser->active->forwardStack;
    browser->active->forwardStack = forwardPage;
    browser->active->backStack = destination->next;
    (void)snprintf(browser->active->currentURL,
                   sizeof(browser->active->currentURL), "%s", destination->url);
    free(destination);
    return 1;
}

int browser_go_forward(Browser *browser)
{
    Page *destination;
    Page *backPage;

    if (browser == NULL || browser->active == NULL ||
        browser->active->forwardStack == NULL) {
        return 0;
    }

    destination = browser->active->forwardStack;
    backPage = (Page *)malloc(sizeof(*backPage));
    if (backPage == NULL) {
        return 0;
    }
    if (!history_append(browser, destination->url)) {
        free(backPage);
        return 0;
    }

    (void)snprintf(backPage->url, sizeof(backPage->url), "%s",
                   browser->active->currentURL);
    backPage->next = browser->active->backStack;
    browser->active->backStack = backPage;
    browser->active->forwardStack = destination->next;
    (void)snprintf(browser->active->currentURL,
                   sizeof(browser->active->currentURL), "%s", destination->url);
    free(destination);
    return 1;
}
