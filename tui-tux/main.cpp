#include <ncurses.h>
#include <cstdio>     // For popen and pclose
#include <cstdlib>
#include <string>
#include <cstring>
#include <iostream>

#define CTRL(x) ((x) & 0x1f)

WINDOW* assistant_win;
WINDOW* bash_win;

void switch_focus(WINDOW *&current_win, WINDOW *assistant_win, WINDOW *bash_win, bool &is_assistant_focused) {
    is_assistant_focused = !is_assistant_focused;
    current_win = is_assistant_focused ? assistant_win : bash_win;
    wbkgd(assistant_win, is_assistant_focused ? COLOR_PAIR(1) : COLOR_PAIR(2));
    wbkgd(bash_win, is_assistant_focused ? COLOR_PAIR(2) : COLOR_PAIR(1));
    wrefresh(assistant_win);
    wrefresh(bash_win);
}

void execute_command(WINDOW* win, const std::string& command) {
    int x, y;
    getyx(win, y, x);

    // Clear the current input line
    wmove(win, y, 1); // Start from column 1 to avoid border
    wclrtoeol(win);
    wrefresh(win);

    // Open a pipe to execute the command
    FILE* fp = popen(command.c_str(), "r");
    if (fp == nullptr) {
        wprintw(win, "Failed to run command\n");
        wrefresh(win);
        return;
    }

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), fp) != nullptr) {
        wprintw(win, "%s", buffer);
        getyx(win, y, x);
        if (y >= getmaxy(win) - 2) {
            wscrl(win, 1);
            y = getmaxy(win) - 2;
        }
        wmove(win, y, 1);
        wrefresh(win);
    }
    pclose(fp);

    // Move cursor to the next line and print the prompt again
    getyx(win, y, x); // Get current cursor position after command output
    y++;
    wmove(win, y, 1);
    wclrtoeol(win);

    // Correctly determine which prompt to display
    if (win == assistant_win) {
        wprintw(win, "assistant> ");
    } else if (win == bash_win) {
        wprintw(win, "bash> ");
    }
    wrefresh(win);
}

int main() {
    initscr();
    start_color();
    init_pair(1, COLOR_BLACK, COLOR_WHITE); // Focused window
    init_pair(2, COLOR_WHITE, COLOR_BLACK); // Unfocused window
    noecho();
    cbreak();             // Disable line buffering
    keypad(stdscr, TRUE); // Enable function keys

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // Create two windows
    assistant_win = newwin(rows / 2 - 1, cols - 2, 0, 1);
    bash_win = newwin(rows / 2 - 1, cols - 2, rows / 2, 1);

    // Draw boxes around the windows
    box(assistant_win, 0, 0);
    box(bash_win, 0, 0);

    // Add labels
    mvwprintw(assistant_win, 1, 1, "assistant>");
    mvwprintw(bash_win, 1, 1, "bash>");

    wbkgd(assistant_win, COLOR_PAIR(1)); // Initially focus on assistant
    wbkgd(bash_win, COLOR_PAIR(2));
    wrefresh(assistant_win);
    wrefresh(bash_win);

    WINDOW *current_win = assistant_win;
    bool is_assistant_focused = true;

    char command[256];
    int command_len = 0;

    int ch;
    while ((ch = wgetch(current_win)) != 'q') {
        if (ch == '\t' && (ch & CTRL('I'))) { // Check for Ctrl + Tab
            switch_focus(current_win, assistant_win, bash_win, is_assistant_focused);
            command_len = 0;
            memset(command, 0, sizeof(command));
            wmove(current_win, 1, 1);
            wclrtoeol(current_win);
            mvwprintw(current_win, 1, 1, "%s> ", (current_win == assistant_win) ? "assistant" : "bash");
            wrefresh(current_win);
        } else {
            switch (ch) {
                case '\n':
                    if (command_len > 0) {
                        command[command_len] = '\0';
                        std::string command_str(command);
                        execute_command(current_win, command_str);
                        command_len = 0;
                        memset(command, 0, sizeof(command));
                    }
                    break;
                case KEY_BACKSPACE:
                case 127:
                    if (command_len > 0) {
                        command_len--;
                        int x, y;
                        getyx(current_win, y, x);
                        mvwdelch(current_win, y, x - 1);
                    }
                    break;
                default:
                    if (command_len < sizeof(command) - 1) {
                        command[command_len++] = ch;
                        waddch(current_win, ch);
                    }
                    break;
            }
            wrefresh(current_win);
        }
    }

    // Cleanup
    delwin(assistant_win);
    delwin(bash_win);
    endwin();

    return 0;
}
