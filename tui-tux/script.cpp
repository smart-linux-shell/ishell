#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <cstdlib>

void switch_focus(WINDOW *&current_win, WINDOW *dialog_win, WINDOW *bash_win, bool &is_dialog_focused) {
    is_dialog_focused = !is_dialog_focused;
    current_win = is_dialog_focused ? dialog_win : bash_win;
    wbkgd(dialog_win, is_dialog_focused ? COLOR_PAIR(1) : COLOR_PAIR(2));
    wbkgd(bash_win, is_dialog_focused ? COLOR_PAIR(2) : COLOR_PAIR(1));
    wrefresh(dialog_win);
    wrefresh(bash_win);
}

void execute_command(WINDOW *win, const char *command) {
    // Temporarily leave ncurses mode
    def_prog_mode();    // Save current tty modes
    endwin();           // End ncurses mode

    // Execute the command
    system(command);

    // Return to ncurses mode
    reset_prog_mode();  // Restore the previous tty modes
    refresh();          // Refresh ncurses screen
    wrefresh(win);      // Refresh current window
}

int main() {
    initscr();            // Initialize the window
    start_color();
    init_pair(1, COLOR_BLACK, COLOR_WHITE); // Focused window
    init_pair(2, COLOR_WHITE, COLOR_BLACK); // Unfocused window
    noecho();
    cbreak();             // Disable line buffering
    keypad(stdscr, TRUE); // Enable function keys

    int height, width;
    getmaxyx(stdscr, height, width);

    int window_height = height / 3;
    int window_width = width - 2;

    // Create windows for dialog and bash
    WINDOW *dialog_win = newwin(window_height, window_width, 0, 1);
    WINDOW *bash_win = newwin(window_height, window_width, window_height + 1, 1);

    box(dialog_win, 0, 0);
    box(bash_win, 0, 0);

    mvwprintw(dialog_win, 1, 1, "dialog> ");
    mvwprintw(bash_win, 1, 1, "bash> ");

    // Refresh the windows to show the content
    wbkgd(dialog_win, COLOR_PAIR(1)); // Initially focus on dialog
    wbkgd(bash_win, COLOR_PAIR(2));
    wrefresh(dialog_win);
    wrefresh(bash_win);

    WINDOW *current_win = dialog_win;
    bool is_dialog_focused = true;

    char command[256];
    int command_len = 0;

    int ch;
    while ((ch = wgetch(current_win)) != 'q') {
        switch (ch) {
            case '\t':
                switch_focus(current_win, dialog_win, bash_win, is_dialog_focused);
                command_len = 0;
                memset(command, 0, sizeof(command));
                mvwprintw(current_win, 1, 1, "%s> ", (current_win == dialog_win) ? "dialog" : "bash");
                wrefresh(current_win);
                break;
            case '\n':
                if (command_len > 0) {
                    command[command_len] = '\0';
                    execute_command(current_win, command);
                    command_len = 0;
                    memset(command, 0, sizeof(command));
                    mvwprintw(current_win, 1, 1, "%s> ", (current_win == dialog_win) ? "dialog" : "bash");
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

    // Clean up
    delwin(dialog_win);
    delwin(bash_win);
    endwin();

    return 0;
}
