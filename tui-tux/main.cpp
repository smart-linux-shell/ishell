#include <ncurses.h>
#include <cstdio>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <vector>

#define CTRL_KEY(x) ((x) & 0x1f)

class TerminalMultiplexer {
public:
    TerminalMultiplexer() : assistant_win(nullptr), bash_win(nullptr), current_win(nullptr), is_assistant_focused(true) {
        init_screen();
        init_windows();
    }

    ~TerminalMultiplexer() {
        cleanup();
    }

    void run() {
        int ch;
        while ((ch = wgetch(current_win)) != 'q') {
            if (ch == KEY_RESIZE) {
                init_windows();
            } else {
                handle_input(ch);
            }
        }
    }

private:
    WINDOW* assistant_win;
    WINDOW* bash_win;
    WINDOW* current_win;
    bool is_assistant_focused;

    std::vector<std::string> current_commands;  // Store the current command in each terminal
    std::vector<std::vector<std::string>> command_history;  // Store the history of commands for each terminal

    void init_screen() {
        initscr();
        start_color();
        init_pair(1, COLOR_BLACK, COLOR_WHITE);  // Focused window
        init_pair(2, COLOR_WHITE, COLOR_BLACK);  // Unfocused window
        noecho();
        cbreak();
        keypad(stdscr, TRUE);
        current_commands.resize(2); // 0 for assistant, 1 for bash
        command_history.resize(2);  // 0 for assistant, 1 for bash
    }

    void init_windows() {
        int rows, cols;
        getmaxyx(stdscr, rows, cols);

        if (assistant_win != nullptr) {
            delwin(assistant_win);
        }
        if (bash_win != nullptr) {
            delwin(bash_win);
        }

        clear();
        refresh();

        assistant_win = create_window(rows / 2 - 1, cols - 4, 1, 2, "assistant>");
        bash_win = create_window(rows / 2 - 1, cols - 4, rows / 2 + 1, 2, "bash>");
        current_win = is_assistant_focused ? assistant_win : bash_win;
        set_focus_on_window(current_win);
    }

    WINDOW* create_window(int height, int width, int starty, int startx, const char* label) {
        WINDOW* win = newwin(height, width, starty, startx);
        scrollok(win, TRUE);  // Enable scrolling for the window
        box(win, 0, 0);
        mvwprintw(win, 1, 1, "%s", label);
        wrefresh(win);
        return win;
    }

    void cleanup() {
        if (assistant_win) delwin(assistant_win);
        if (bash_win) delwin(bash_win);
        endwin();
    }

    void set_focus_on_window(WINDOW* win) {
        is_assistant_focused = (win == assistant_win);
        current_win = win;

        wbkgd(assistant_win, is_assistant_focused ? COLOR_PAIR(1) : COLOR_PAIR(2));
        wbkgd(bash_win, is_assistant_focused ? COLOR_PAIR(2) : COLOR_PAIR(1));

        refresh_window(assistant_win);
        refresh_window(bash_win);

        reset_cursor_position();
    }

    void refresh_window(WINDOW* win) {
        touchwin(win);
        wrefresh(win);
    }

    void reset_cursor_position() {
        int y, x;
        getyx(current_win, y, x);
        if (x >= getmaxx(current_win) - 1) {
            x = 1;
            y++;
        }
        scroll_if_needed(y);
        wmove(current_win, y, x);
        wrefresh(current_win);
    }

    void execute_command(const std::string& command) {
        int y, x;
        getyx(current_win, y, x);
        y++;
        scroll_if_needed(y);
        wmove(current_win, y, 1);

        int pipefd[2];
        if (pipe(pipefd) == -1) {
            print_error("Pipe failed");
            return;
        }

        pid_t pid = fork();
        if (pid == -1) {
            print_error("Fork failed");
            return;
        }

        if (pid == 0) {
            execute_child_process(command, pipefd);
        } else {
            read_from_pipe(pipefd, pid);
            print_prompt();
        }
    }

    void scroll_if_needed(int& y) {
        if (y >= getmaxy(current_win) - 1) {
            char command_buffer[1024];  // Local buffer to hold the current line

            for (int i = 2; i < getmaxy(current_win) - 1; i++) {
                mvwinnstr(current_win, i + 1, 1, command_buffer, getmaxx(current_win) - 2);
                mvwprintw(current_win, i, 1, "%s", command_buffer);
            }
            mvwhline(current_win, getmaxy(current_win) - 2, 1, ' ', getmaxx(current_win) - 2);
            y = getmaxy(current_win) - 2;
        }
    }

    void print_error(const char* msg) {
        wprintw(current_win, "%s\n", msg);
        wrefresh(current_win);
    }

    void execute_child_process(const std::string& command, int pipefd[2]) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
        _exit(EXIT_FAILURE);
    }

    void read_from_pipe(int pipefd[2], pid_t pid) {
        close(pipefd[1]);
        char buffer[128];
        ssize_t count;
        while ((count = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[count] = '\0';
            print_buffer(buffer, count);
        }
        close(pipefd[0]);
        waitpid(pid, nullptr, 0);
    }

    void print_buffer(const char* buffer, ssize_t count) {
        int y, x;
        getyx(current_win, y, x);
        int width = getmaxx(current_win);

        for (int i = 0; i < count; ++i) {
            if (x == width - 1) {
                wmove(current_win, ++y, 1);
                scroll_if_needed(y);
                x = 1;
            }
            if (buffer[i] == '\n') {
                y++;
                x = 1;
                scroll_if_needed(y);
                wmove(current_win, y, x);
            } else {
                waddch(current_win, buffer[i]);
                x++;
            }
        }
        wrefresh(current_win);
    }

    void print_prompt() {
        int y, x;
        getyx(current_win, y, x);
        scroll_if_needed(y);
        wmove(current_win, y, 1);
        wprintw(current_win, "%s> ", is_assistant_focused ? "assistant" : "bash");
        wrefresh(current_win);
    }

    void handle_input(int ch) {
        if (ch == CTRL_KEY('i')) {
            set_focus_on_window(is_assistant_focused ? bash_win : assistant_win);
        } else if (ch == '\n') {
            execute_command_if_not_empty();
        } else if (ch == KEY_BACKSPACE || ch == 127) {
            handle_backspace();
        } else {
            handle_character_input(ch);
        }
    }

    void execute_command_if_not_empty() {
        int index = is_assistant_focused ? 0 : 1;
        if (!current_commands[index].empty()) {
            execute_command(current_commands[index]);
            command_history[index].push_back(current_commands[index]);
            current_commands[index].clear();
        }
    }

    void handle_backspace() {
        int index = is_assistant_focused ? 0 : 1;
        if (!current_commands[index].empty()) {
            current_commands[index].pop_back();
            int y, x;
            getyx(current_win, y, x);
            mvwaddch(current_win, y, x - 1, ' '); // Replace character with space
            wmove(current_win, y, x - 1); // Move cursor back
            draw_borders(); // Redraw the borders
            wrefresh(current_win);
        }
    }

    void draw_borders() {
        // Redraw the box around the window
        box(assistant_win, 0, 0);
        box(bash_win, 0, 0);
        wrefresh(assistant_win);
        wrefresh(bash_win);
    }

    void handle_character_input(int ch) {
        int index = is_assistant_focused ? 0 : 1;
        char command_buffer[1024];  // Local buffer to hold the command

        if (current_commands[index].size() < sizeof(command_buffer) - 1) {
            int y, x;
            getyx(current_win, y, x);
            if (x < getmaxx(current_win) - 2) {
                current_commands[index].push_back(ch);
                waddch(current_win, ch);
            } else if (y < getmaxy(current_win) - 1) {
                move_cursor_to_new_line(y, x);
                current_commands[index].push_back(ch);
                waddch(current_win, ch);
            }
            wrefresh(current_win);
        }
    }

    void move_cursor_to_new_line(int& y, int& x) {
        y++;
        scroll_if_needed(y);
        x = 1;
        wmove(current_win, y, x);
    }
};

int main() {
    TerminalMultiplexer multiplexer;
    multiplexer.run();
    return 0;
}