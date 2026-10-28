#include "browser.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int read_line(const char *prompt, char *buffer, size_t capacity)
{
    size_t length;
    int character;

    printf("%s", prompt);
    if (fgets(buffer, (int)capacity, stdin) == NULL) {
        return 0;
    }
    length = strlen(buffer);
    if (length > 0U && buffer[length - 1U] == '\n') {
        buffer[--length] = '\0';
        if (length > 0U && buffer[length - 1U] == '\r') {
            buffer[--length] = '\0';
        }
        return 1;
    }
    if (feof(stdin)) {
        return 1;
    }

    while ((character = getchar()) != '\n' && character != EOF) {
    }
    printf("Input is too long. Please try again.\n");
    return 0;
}

static int read_integer(const char *prompt, int *value)
{
    char input[64];
    char *end;
    long parsed;

    if (!read_line(prompt, input, sizeof(input))) {
        return 0;
    }
    errno = 0;
    parsed = strtol(input, &end, 10);
    while (*end == ' ' || *end == '\t') {
        ++end;
    }
    if (input[0] == '\0' || end == input || *end != '\0' ||
        errno == ERANGE || parsed < INT_MIN || parsed > INT_MAX) {
        printf("Invalid number. Enter a whole number.\n");
        return 0;
    }
    *value = (int)parsed;
    return 1;
}

static void print_menu(const Browser *browser)
{
    printf("\n");
    printf("+======================================================================+\n");
    printf("|                 BROWSER TAB AND SESSION MANAGER                     |\n");
    printf("+======================================================================+\n");
    if (browser->active != NULL) {
        printf("| Active tab: %-5d | %-20.20s | %-28.28s |\n",
               browser->active->id, browser->active->title,
               browser->active->currentURL);
    } else {
        printf("| Active tab: none                                                     |\n");
    }
    printf("+----------------------------------------------------------------------+\n");
    printf("|  1. Open a new tab             9.  Display all open tabs             |\n");
    printf("|  2. Close a tab               10.  Search tabs by ID or title        |\n");
    printf("|  3. Switch active tab         11.  Save current session               |\n");
    printf("|  4. Visit a website URL       12.  Restore saved session              |\n");
    printf("|  5. Backward navigation       13.  Clear browsing history             |\n");
    printf("|  6. Forward navigation        14.  Display active tab                  |\n");
    printf("|  7. Display browsing history  15.  Exit safely                        |\n");
    printf("|  8. Restore recently closed tab                                      |\n");
    printf("+======================================================================+\n");
}

static void action_open_tab(Browser *browser)
{
    char title[MAX_TITLE];
    int id;
    if (!read_line("Enter a title for the new tab: ", title, sizeof(title)) ||
        title[0] == '\0') {
        printf("A non-empty title is required.\n");
        return;
    }
    id = browser_open_tab(browser, title, "about:blank");
    if (id == 0) {
        printf("Could not open tab. Memory may be unavailable or the ID limit was reached.\n");
    } else {
        printf("Tab opened successfully. Its ID is %d. Use option 4 to visit a URL.\n", id);
    }
}

static void action_close_tab(Browser *browser)
{
    int id;
    if (!read_integer("Enter the ID of the tab to close: ", &id)) {
        return;
    }
    if (browser_close_tab(browser, id)) {
        printf("Tab %d closed. You can restore it with option 8.\n", id);
    } else {
        printf("Tab not found, or there was not enough memory to close it safely.\n");
    }
}

static void action_switch_tab(Browser *browser)
{
    int id;
    if (!read_integer("Enter the ID of the tab to activate: ", &id)) {
        return;
    }
    if (browser_switch_tab(browser, id)) {
        printf("Tab %d is now active.\n", id);
    } else {
        printf("No open tab has ID %d.\n", id);
    }
}

static void action_visit(Browser *browser)
{
    char url[MAX_URL];
    if (browser->active == NULL) {
        printf("Open or restore a tab before visiting a website.\n");
        return;
    }
    if (!read_line("Enter the URL to visit: ", url, sizeof(url)) || url[0] == '\0') {
        printf("A non-empty URL is required.\n");
        return;
    }
    if (browser_visit(browser, url)) {
        printf("Navigation successful: %s\n", url);
    } else {
        printf("Navigation failed. Check the URL and available memory.\n");
    }
}

static void action_search(Browser *browser)
{
    int mode;
    Tab *tab;
    if (!read_integer("Search by 1) ID or 2) title: ", &mode)) {
        return;
    }
    if (mode == 1) {
        int id;
        if (!read_integer("Enter tab ID: ", &id)) {
            return;
        }
        tab = browser_find_tab_by_id(browser, id);
    } else if (mode == 2) {
        char title[MAX_TITLE];
        if (!read_line("Enter title text to search for: ", title, sizeof(title))) {
            return;
        }
        tab = browser_find_tab_by_title(browser, title);
    } else {
        printf("Choose 1 or 2.\n");
        return;
    }
    if (tab == NULL) {
        printf("No matching tab was found.\n");
    } else {
        printf("Match: ID %d | %s | %s%s\n", tab->id, tab->title, tab->currentURL,
               tab == browser->active ? " [ACTIVE]" : "");
    }
}

static void action_save_session(const Browser *browser)
{
    if (session_save(browser, SESSION_FILE)) {
        printf("Session saved to %s.\n", SESSION_FILE);
    } else {
        printf("Could not save the session to %s. Check file permissions and session size.\n",
               SESSION_FILE);
    }
}

static void action_load_session(Browser *browser)
{
    if (session_load(browser, SESSION_FILE)) {
        printf("Session restored from %s. Open tabs were replaced by the saved tabs.\n",
               SESSION_FILE);
    } else {
        printf("Could not restore %s. The file may be missing, corrupted, or incompatible; "
               "your current tabs were kept.\n", SESSION_FILE);
    }
}

int main(void)
{
    Browser browser;
    int running = 1;

    browser_init(&browser);
    printf("+======================================================================+\n");
    printf("|             WELCOME TO BROWSER TAB AND SESSION MANAGER               |\n");
    printf("|     A C data-structures project: tabs, stacks, history and files      |\n");
    printf("+======================================================================+\n");

    while (running) {
        int choice;
        print_menu(&browser);
        if (!read_integer("Select an option (1-15): ", &choice)) {
            if (feof(stdin)) {
                printf("\nInput closed. Exiting safely.\n");
                break;
            }
            continue;
        }
        switch (choice) {
        case 1:
            action_open_tab(&browser);
            break;
        case 2:
            action_close_tab(&browser);
            break;
        case 3:
            action_switch_tab(&browser);
            break;
        case 4:
            action_visit(&browser);
            break;
        case 5:
            if (browser_go_back(&browser)) {
                printf("Moved backward to %s.\n", browser.active->currentURL);
            } else {
                printf("No previous page is available, or memory is unavailable.\n");
            }
            break;
        case 6:
            if (browser_go_forward(&browser)) {
                printf("Moved forward to %s.\n", browser.active->currentURL);
            } else {
                printf("No forward page is available, or memory is unavailable.\n");
            }
            break;
        case 7:
            browser_show_history(&browser);
            break;
        case 8:
            if (browser_restore_closed_tab(&browser)) {
                printf("Recently closed tab restored and activated.\n");
            } else {
                printf("No recently closed tab is available to restore.\n");
            }
            break;
        case 9:
            browser_list_tabs(&browser);
            break;
        case 10:
            action_search(&browser);
            break;
        case 11:
            action_save_session(&browser);
            break;
        case 12:
            action_load_session(&browser);
            break;
        case 13:
            browser_clear_history(&browser);
            printf("Browsing history cleared.\n");
            break;
        case 14:
            browser_show_active_tab(&browser);
            break;
        case 15:
            running = 0;
            break;
        default:
            printf("Invalid option. Select a number from 1 to 15.\n");
            break;
        }
    }

    browser_cleanup(&browser);
    printf("All allocated browser data has been released. Goodbye!\n");
    return 0;
}
