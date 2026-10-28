# Browser Tab and Session Manager

A console-based browser simulator written in standard C. It demonstrates a
doubly linked list for open tabs, linked stacks for page navigation and recently
closed tabs, a singly linked browsing-history list, linear tab search, and file
handling for saved sessions. It does not connect to or render real websites.

## Project files

- `main.c` - menu, input validation, and feature dispatch
- `browser.h` - shared structures and declarations
- `browser.c` - tab list, searching, tab display, and core management
- `stack.c` - page-stack support and recently closed tabs
- `history.c` - browsing history and back/forward navigation
- `session.c` - binary session save and restore

The session is saved to `browser_session.dat` in the program's current working
directory. A session stores tab titles, current URLs, tab IDs, the active tab,
and both navigation stacks. Loading a session replaces the open tabs only after
the complete file has been validated. A missing or invalid file leaves the
current open tabs unchanged. Browsing history is kept in memory and is not part
of the saved session.

## Compile with GCC

Open a PowerShell terminal in the `Browser_Tab_Manager` directory and run:

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic main.c browser.c stack.c history.c session.c -o browser_manager.exe
```

## Run in PowerShell

```powershell
.\browser_manager.exe
```

## Run in Visual Studio Code on Windows

1. Install Visual Studio Code and the Microsoft C/C++ extension.
2. Install GCC for Windows (for example, MinGW-w64) and ensure `gcc` is on
   `PATH`. Confirm by running `gcc --version` in the VS Code terminal.
3. In VS Code, choose **File > Open Folder...** and open this
   `Browser_Tab_Manager` folder.
4. Choose **Terminal > New Terminal**. The terminal should open in the project
   folder. If it does not, change directory to this folder.
5. Compile with the GCC command above.
6. Run with `.\browser_manager.exe`.
7. Select menu options by entering their number. Open a tab first, then use
   **Visit a website URL** to navigate. Save and restore sessions with options
   11 and 12.

## Data structure notes

- Open tabs are stored in a doubly linked list, making insertion and removal
  straightforward.
- Each tab has separate linked stacks for back and forward navigation. Visiting
  a new URL clears that tab's forward stack.
- Closed tabs are held in a stack and retain their page-navigation stacks.
- Browsing history is an append-only singly linked list until cleared.
- Tab search is a linear search by ID or a case-insensitive title substring.
- All dynamic allocations are released on tab close, history clear, session
  replacement, and application exit.
