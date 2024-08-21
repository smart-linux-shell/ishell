#include <ncurses.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>

#define CTRL(x) ((x) & 0x1f)

class TerminalMultiplexer {
public:
    TerminalMultiplexer() : assistant_win(nullptr), bash_win(nullptr), current_win(nullptr), is_assistant_focused(true) {
        init();
    }

    ~TerminalMultiplexer() {
        cleanup();
    }

    void run() {
        int ch;
        while ((ch = wgetch(current_win)) != 'q') {
            handle_input(ch);
        }
    }

private:
    WINDOW* assistant_win;
    WINDOW* bash_win;
    WINDOW* current_win;
    bool is_assistant_focused;
    char command[256];
    int command_len;

    void init() {
        initscr();
        start_color();
        init_pair(1, COLOR_BLACK, COLOR_WHITE); // Focused window
        init_pair(2, COLOR_WHITE, COLOR_BLACK); // Unfocused window
        noecho();
        cbreak();
        keypad(stdscr, TRUE);

        int rows, cols;
        getmaxyx(stdscr, rows, cols);

        // Create the windows with proper spacing
        assistant_win = newwin(rows / 2 - 1, cols - 4, 1, 2);
        bash_win = newwin(rows / 2 - 1, cols - 4, rows / 2 + 1, 2);

        // Draw borders around the windows
        box(assistant_win, 0, 0);
        box(bash_win, 0, 0);

        // Print the prompts aligned correctly
        mvwprintw(assistant_win, 1, 1, "assistant>");
        mvwprintw(bash_win, 1, 1, "bash>");

        // Set the focus on the assistant window
        current_win = assistant_win;
        wbkgd(assistant_win, COLOR_PAIR(1));
        wbkgd(bash_win, COLOR_PAIR(2));
        wrefresh(assistant_win);
        wrefresh(bash_win);

        command_len = 0;
        memset(command, 0, sizeof(command));
    }

    void cleanup() {
        delwin(assistant_win);
        delwin(bash_win);
        endwin();
    }

    void switch_focus() {
        is_assistant_focused = !is_assistant_focused;
        current_win = is_assistant_focused ? assistant_win : bash_win;
        wbkgd(assistant_win, is_assistant_focused ? COLOR_PAIR(1) : COLOR_PAIR(2));
        wbkgd(bash_win, is_assistant_focused ? COLOR_PAIR(2) : COLOR_PAIR(1));
        wrefresh(assistant_win);
        wrefresh(bash_win);

        // Clear and reset the prompt in the new focused window
        command_len = 0;
        memset(command, 0, sizeof(command));
        wmove(current_win, 2, 1);
        wclrtoeol(current_win);
        mvwprintw(current_win, 2, 1, "%s> ", is_assistant_focused ? "assistant" : "bash");
        wrefresh(current_win);
    }

    void execute_command(const std::string& command) {
        int x, y;
        getyx(current_win, y, x);

        wmove(current_win, y, 1);
        wclrtoeol(current_win);
        wrefresh(current_win);

        FILE* fp = popen(command.c_str(), "r");
        if (fp == nullptr) {
            wprintw(current_win, "Failed to run command\n");
            wrefresh(current_win);
            return;
        }

        char buffer[128];
        while (fgets(buffer, sizeof(buffer), fp) != nullptr) {
            wprintw(current_win, "%s", buffer);
            getyx(current_win, y, x);
            if (y >= getmaxy(current_win) - 2) {
                wscrl(current_win, 1);
                y = getmaxy(current_win) - 2;
            }
            wmove(current_win, y, 1);
            wrefresh(current_win);
        }
        pclose(fp);

        getyx(current_win, y, x);
        y++;
        wmove(current_win, y, 1);
        wclrtoeol(current_win);

        wprintw(current_win, "%s> ", is_assistant_focused ? "assistant" : "bash");
        wrefresh(current_win);
    }

    void handle_input(int ch) {
        if (ch == '\t' && (ch & CTRL('I'))) { // Ctrl + Tab
            switch_focus();
        } else {
            switch (ch) {
                case '\n':
                    if (command_len > 0) {
                        command[command_len] = '\0';
                        std::string command_str(command);
                        execute_command(command_str);
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
};

int main() {
    TerminalMultiplexer tmux;
    tmux.run();
    return 0;
}
