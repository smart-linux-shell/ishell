#include <ncurses.h>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <string>
#include <cstring>
#include <vector>
#include <iostream>

#define CTRL(x)           ((x) & 0x1f)

void switch_focus(WINDOW *&current_win, WINDOW *dialog_win, WINDOW *bash_win, bool &is_dialog_focused) {
    is_dialog_focused = !is_dialog_focused;
    current_win = is_dialog_focused ? dialog_win : bash_win;
    wbkgd(dialog_win, is_dialog_focused ? COLOR_PAIR(1) : COLOR_PAIR(2));
    wbkgd(bash_win, is_dialog_focused ? COLOR_PAIR(2) : COLOR_PAIR(1));
    wrefresh(dialog_win);
    wrefresh(bash_win);
}

void execute_command(WINDOW* win, const std::string& command) {
    int x, y;
    getyx(win, y, x);

    // Clear the current input line
     wmove(win, y, 1); // Start from column 1 to avoid border
    wclrtoeol(win);
    wrefresh(win);

    // Fork a process to execute the command
    pid_t pid = fork();
    if (pid == 0) { // Child process
        // Redirect output to the window
        int pipe_fd[2];
        pipe(pipe_fd);
        if (fork() == 0) {
            dup2(pipe_fd[1], STDOUT_FILENO);
            dup2(pipe_fd[1], STDERR_FILENO);
            close(pipe_fd[0]);
            close(pipe_fd[1]);

            execlp("bash", "bash", "-c", command.c_str(), nullptr);
            _exit(1); // If exec fails
        } else {
            close(pipe_fd[1]);
            char buffer[128];
            int n;
            while ((n = read(pipe_fd[0], buffer, sizeof(buffer) - 1)) > 0) {
                buffer[n] = '\0';
                wmove(win, y, 1); // Move to the start of the current line (inside the border)
                waddstr(win, buffer);
                wrefresh(win);
                getyx(win, y, x);
            }
            close(pipe_fd[0]);
            wait(nullptr); // Wait for the child process to finish
            exit(0);
        }
    } else { // Parent process
        wait(nullptr); // Wait for the child process to finish
    }

    // Move cursor to the next line and print the prompt again
    getyx(win, y, x); // Get current cursor position after command output
    if (y >= getmaxy(win) - 2) {
        // Scroll the window if we're at the bottom line
        wscrl(win, 1);
        y = getmaxy(win) - 2;
    } else {
        y++;
    }
    wmove(win, y, 1);
    wclrtoeol(win);
    wprintw(win, "%s> ", (win == stdscr) ? "dialog" : "bash");
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
    WINDOW* dialog_win = newwin(rows / 2 - 1, cols - 2, 0, 1);
    WINDOW* bash_win = newwin(rows / 2 - 1, cols - 2, rows / 2, 1);

    // Draw boxes around the windows
    box(dialog_win, 0, 0);
    box(bash_win, 0, 0);

    // Add labels
    mvwprintw(dialog_win, 1, 1, "dialog>");
    mvwprintw(bash_win, 1, 1, "bash>");

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
                wmove(current_win, 1, 1);
                wclrtoeol(current_win);
                mvwprintw(current_win, 1, 1, "%s> ", (current_win == dialog_win) ? "dialog" : "bash");
                wrefresh(current_win);
                break;
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

    // Cleanup
    delwin(dialog_win);
    delwin(bash_win);
    endwin();

    return 0;
}
