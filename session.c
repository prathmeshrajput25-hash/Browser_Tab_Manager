#include "browser.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SESSION_VERSION 1U

typedef struct SessionHeader {
    char magic[8];
    uint32_t version;
    uint32_t tabCount;
    int32_t activeId;
    int32_t nextId;
} SessionHeader;

typedef struct SessionTab {
    int32_t id;
    char title[MAX_TITLE];
    char currentURL[MAX_URL];
    uint32_t backCount;
    uint32_t forwardCount;
} SessionTab;

static void free_tabs(Tab *tab)
{
    while (tab != NULL) {
        Tab *next = tab->next;
        page_stack_free(tab->backStack);
        page_stack_free(tab->forwardStack);
        free(tab);
        tab = next;
    }
}

static int count_stack(const Page *stack, uint32_t *count)
{
    uint32_t total = 0;
    while (stack != NULL) {
        if (total == MAX_SESSION_STACK_ITEMS) {
            return 0;
        }
        ++total;
        stack = stack->next;
    }
    *count = total;
    return 1;
}

static int write_stack(FILE *file, const Page *stack)
{
    while (stack != NULL) {
        if (fwrite(stack->url, sizeof(stack->url), 1U, file) != 1U) {
            return 0;
        }
        stack = stack->next;
    }
    return 1;
}

static int read_stack(FILE *file, uint32_t count, Page **stack)
{
    Page **tail = stack;
    uint32_t index;
    *stack = NULL;

    for (index = 0; index < count; ++index) {
        Page *page = (Page *)calloc(1, sizeof(*page));
        if (page == NULL) {
            return 0;
        }
        if (fread(page->url, sizeof(page->url), 1U, file) != 1U ||
            memchr(page->url, '\0', sizeof(page->url)) == NULL ||
            page->url[0] == '\0') {
            free(page);
            return 0;
        }
        *tail = page;
        tail = &page->next;
    }
    return 1;
}

static int write_session_tab(FILE *file, const Tab *tab)
{
    SessionTab record;
    (void)memset(&record, 0, sizeof(record));
    record.id = tab->id;
    (void)memcpy(record.title, tab->title, sizeof(record.title));
    (void)memcpy(record.currentURL, tab->currentURL, sizeof(record.currentURL));
    if (!count_stack(tab->backStack, &record.backCount) ||
        !count_stack(tab->forwardStack, &record.forwardCount)) {
        return 0;
    }
    return fwrite(&record, sizeof(record), 1U, file) == 1U &&
           write_stack(file, tab->backStack) &&
           write_stack(file, tab->forwardStack);
}

int session_save(const Browser *browser, const char *filename)
{
    const Tab *tab;
    SessionHeader header;
    FILE *file;
    char temporary[512];
    int tabCount;
    int success = 1;

    if (browser == NULL || filename == NULL || filename[0] == '\0') {
        return 0;
    }
    tabCount = browser_tab_count(browser);
    if ((unsigned int)tabCount > MAX_SESSION_TABS ||
        (tabCount > 0 && browser->active == NULL) ||
        (tabCount == 0 && browser->active != NULL)) {
        return 0;
    }
    if (snprintf(temporary, sizeof(temporary), "%s.tmp", filename) < 0 ||
        strlen(filename) + 4U >= sizeof(temporary)) {
        return 0;
    }

    (void)memset(&header, 0, sizeof(header));
    (void)memcpy(header.magic, "BTMSESS1", 8U);
    header.version = SESSION_VERSION;
    header.tabCount = (uint32_t)tabCount;
    header.activeId = browser->active != NULL ? browser->active->id : 0;
    header.nextId = browser->nextId;

    file = fopen(temporary, "wb");
    if (file == NULL) {
        return 0;
    }
    if (fwrite(&header, sizeof(header), 1U, file) != 1U) {
        success = 0;
    }
    for (tab = browser->head; success && tab != NULL; tab = tab->next) {
        if (!write_session_tab(file, tab)) {
            success = 0;
        }
    }
    if (fclose(file) != 0) {
        success = 0;
    }
    if (!success) {
        (void)remove(temporary);
        return 0;
    }

    if (remove(filename) != 0) {
        FILE *existing = fopen(filename, "rb");
        if (existing != NULL) {
            (void)fclose(existing);
            (void)remove(temporary);
            return 0;
        }
    }
    if (rename(temporary, filename) != 0) {
        (void)remove(temporary);
        return 0;
    }
    return 1;
}

static int id_exists(const Tab *head, int32_t id)
{
    while (head != NULL) {
        if (head->id == id) {
            return 1;
        }
        head = head->next;
    }
    return 0;
}

/* Parse into a temporary list; replace live tabs only after full validation. */
int session_load(Browser *browser, const char *filename)
{
    SessionHeader header;
    FILE *file;
    Tab *loadedHead = NULL;
    Tab *loadedTail = NULL;
    Tab *loadedActive = NULL;
    uint32_t index;
    int32_t maxId = 0;
    int valid = 1;
    int trailing;

    if (browser == NULL || filename == NULL || filename[0] == '\0') {
        return 0;
    }
    file = fopen(filename, "rb");
    if (file == NULL) {
        return 0;
    }
    if (fread(&header, sizeof(header), 1U, file) != 1U ||
        memcmp(header.magic, "BTMSESS1", 8U) != 0 ||
        header.version != SESSION_VERSION ||
        header.tabCount > MAX_SESSION_TABS ||
        header.nextId <= 0 ||
        (header.tabCount == 0U && header.activeId != 0) ||
        (header.tabCount > 0U && header.activeId <= 0)) {
        (void)fclose(file);
        return 0;
    }

    for (index = 0; index < header.tabCount && valid; ++index) {
        SessionTab record;
        Tab *tab;
        if (fread(&record, sizeof(record), 1U, file) != 1U ||
            record.id <= 0 ||
            record.backCount > MAX_SESSION_STACK_ITEMS ||
            record.forwardCount > MAX_SESSION_STACK_ITEMS ||
            memchr(record.title, '\0', sizeof(record.title)) == NULL ||
            memchr(record.currentURL, '\0', sizeof(record.currentURL)) == NULL ||
            record.title[0] == '\0' || record.currentURL[0] == '\0' ||
            id_exists(loadedHead, record.id)) {
            valid = 0;
            break;
        }

        tab = (Tab *)calloc(1, sizeof(*tab));
        if (tab == NULL) {
            valid = 0;
            break;
        }
        tab->id = record.id;
        (void)memcpy(tab->title, record.title, sizeof(tab->title));
        (void)memcpy(tab->currentURL, record.currentURL, sizeof(tab->currentURL));
        if (!read_stack(file, record.backCount, &tab->backStack) ||
            !read_stack(file, record.forwardCount, &tab->forwardStack)) {
            page_stack_free(tab->backStack);
            page_stack_free(tab->forwardStack);
            free(tab);
            valid = 0;
            break;
        }

        tab->prev = loadedTail;
        if (loadedTail == NULL) {
            loadedHead = tab;
        } else {
            loadedTail->next = tab;
        }
        loadedTail = tab;
        if (tab->id > maxId) {
            maxId = tab->id;
        }
        if (tab->id == header.activeId) {
            loadedActive = tab;
        }
    }

    trailing = valid ? fgetc(file) : EOF;
    if (fclose(file) != 0) {
        valid = 0;
    }
    if (!valid || trailing != EOF || (header.tabCount > 0U && loadedActive == NULL) ||
        header.nextId <= maxId) {
        free_tabs(loadedHead);
        return 0;
    }

    free_tabs(browser->head);
    browser->head = loadedHead;
    browser->active = loadedActive;
    browser->nextId = header.nextId;
    browser_clear_closed_tabs(browser);
    return 1;
}
