#ifndef BROWSER_H
#define BROWSER_H

#include <stddef.h>

#define MAX_URL 256
#define MAX_TITLE 100
#define SESSION_FILE "browser_session.dat"
#define MAX_SESSION_TABS 1000U
#define MAX_SESSION_STACK_ITEMS 10000U

typedef struct Page {
    char url[MAX_URL];
    struct Page *next;
} Page;

typedef struct Tab {
    int id;
    char title[MAX_TITLE];
    char currentURL[MAX_URL];
    Page *backStack;
    Page *forwardStack;
    struct Tab *prev;
    struct Tab *next;
} Tab;

typedef struct History {
    char url[MAX_URL];
    struct History *next;
} History;

typedef struct ClosedTab {
    Tab *tab;
    struct ClosedTab *next;
} ClosedTab;

typedef struct Browser {
    Tab *head;
    Tab *active;
    History *historyHead;
    History *historyTail;
    ClosedTab *closedTabs;
    int nextId;
} Browser;

void browser_init(Browser *browser);
void browser_cleanup(Browser *browser);
int browser_open_tab(Browser *browser, const char *title, const char *url);
int browser_close_tab(Browser *browser, int id);
int browser_switch_tab(Browser *browser, int id);
int browser_visit(Browser *browser, const char *url);
int browser_go_back(Browser *browser);
int browser_go_forward(Browser *browser);
int browser_restore_closed_tab(Browser *browser);
void browser_list_tabs(const Browser *browser);
void browser_show_active_tab(const Browser *browser);
void browser_show_history(const Browser *browser);
void browser_clear_history(Browser *browser);
Tab *browser_find_tab_by_id(const Browser *browser, int id);
Tab *browser_find_tab_by_title(const Browser *browser, const char *query);
int browser_tab_count(const Browser *browser);
void browser_clear_closed_tabs(Browser *browser);

Page *page_stack_clone(const Page *stack);
void page_stack_free(Page *stack);
void page_stack_clear(Page **stack);

int session_save(const Browser *browser, const char *filename);
int session_load(Browser *browser, const char *filename);

#endif
