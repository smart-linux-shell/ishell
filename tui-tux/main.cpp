#include <ncurses.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h> // Include this header for sleep()
#include <pty.h>
#include <fcntl.h>
#include <signal.h>
#include <regex>

#define CTRL(x) ((x) & 0x1f)

#define ANSI_NULL 0
#define ANSI_IN_ESCAPE 1
#define ANSI_IN_ESCAPE_CSI 2
#define ANSI_IN_ESCAPE_OTHER 3

extern char **environ;

class Screen {
public:
    Screen() {}

    Screen(int lines, int cols, WINDOW *window) {
        init(lines, cols, window);
    }

    int get_n_lines() {
        return n_lines;
    }

    int get_n_cols() {
        return n_cols;
    }

    void write_char(char ch) {
        int curx = getcurx(window);
        int cury = getcury(window);
        lines[cury].data[curx] = ch;
    
        if (curx == getmaxx(window) - 1) {
            if (!cursor_wrapped) {
                // Write and return to last position, set wrappped
                waddch(window, ch);
                wmove(window, cury, curx);
                lines[cury].wrapped = true;
                cursor_wrapped = true;
            } else {
                // Start writing on newline
                cursor_down();
                cursor_return();
                waddch(window, ch);
                cursor_wrapped = false;
            }
        } else {
            // Just write
            waddch(window, ch);
        }

        wrefresh(window);
    }

    int move_cursor(int y, int x) {
        int rc = wmove(window, y, x);

        if (rc == ERR) {
            return rc;
        }

        cursor_wrapped = false;
        wrefresh(window);

        return rc;
    }

    int get_x() {
        return getcurx(window);
    }

    int get_y() {
        return getcury(window);
    }

    void cursor_begin() {
        move_cursor(0, 0);
    }

    void cursor_return() {
        move_cursor(getcury(window), 0);
    }

    void cursor_back() {
        move_cursor(getcury(window), getcurx(window) - 1);
    }

    void cursor_forward() {
        move_cursor(getcury(window), getcurx(window) + 1);
    }

    void cursor_up() {
        int rc = move_cursor(getcury(window) - 1, getcurx(window));

        if (rc == ERR) {
            scroll_up();
        }
    }

    void clear() {
        for (int i = 0; i < n_lines; i++) {
            lines[i].wrapped = false;
            memset(lines[i].data, 0, n_cols);
        }

        wclear(window);
        wrefresh(window);
    }

    void erase_in_place() {
        // TODO: hope to remove this
        int curx = getcurx(window);
        int cury = getcury(window);
        lines[cury].data[curx] = ' ';

        if (curx == getmaxx(window) - 1) {
            lines[cury].wrapped = false;
        }

        wdelch(window);

        wrefresh(window);
    }

    void erase_to_eol() {
        int curx = getcurx(window);
        int cury = getcury(window);

        for (int i = curx; i < n_cols; i++) {
            lines[cury].data[i] = 0;
        }

        wclrtoeol(window);

        wrefresh(window);
    }

    void cursor_down() {
        int rc = move_cursor(getcury(window) + 1, getcurx(window));
        
        if (rc == ERR) {
            scroll_down();
        }
    }

    void scroll_down() {
        wclear(window);
        move_cursor(0, 0);

        for (int i = 1; i < n_lines; i++) {
            lines[i - 1] = lines[i];
            for (int j = 0; j < n_cols; j++) {
                if (lines[i].data[j] != 0) {
                    waddch(window, lines[i].data[j]);
                } else {
                    move_cursor(i, 0);
                    break;
                }
            }
        }

        lines[n_lines - 1].data = new char[n_cols];
        lines[n_lines - 1].wrapped = false;
        memset(lines[n_lines - 1].data, 0, n_cols);

        wrefresh(window);
    }

    void scroll_up() {
        wclear(window);
        
        // Move the line data first
        for (int i = n_lines - 1; i > 1; i--) {
            lines[i] = lines[i - 1];
        }

        lines[0].data = new char[n_cols];
        lines[0].wrapped = false;
        memset(lines[0].data, 0, n_cols);

        // Redraw
        wclear(window);
        move_cursor(1, 0);

        for (int i = 1; i < n_lines; i++) {
            for (int j = 0; j < n_cols; j++) {
                if (lines[i].data[j] != 0) {
                    waddch(window, lines[i].data[j]);
                } else {
                    move_cursor(i + 1, 0);
                    break;
                }
            }
        }

        move_cursor(0, 0);
    }

    void newline() {
        int cury = getcury(window);
        
        lines[cury].wrapped = false;
        cursor_down();

        wrefresh(window);
    }

private:
    int n_lines, n_cols;
    bool cursor_wrapped = false;

    WINDOW *window;

    struct Line {
        char *data;
        bool wrapped;
    };

    Line *lines;

    void init(int new_lines, int new_cols, WINDOW *new_window) {
        n_lines = new_lines;
        n_cols = new_cols;
        window = new_window;
        lines = new Line[n_lines];

        for (int i = 0; i < n_lines; i++) {
            lines[i].data = new char[n_cols];
            memset(lines[i].data, 0, n_cols);
        }
    }
};

void escape(std::string &seq, Screen &bash_screen) {
    std::regex clear_regex("^\\[J$");
    std::regex home_regex("^\\[H$");
    std::regex cursor_up_regex("^\\[A$");
    std::regex cursor_down_regex("^\\[B$");
    std::regex cursor_forward_regex("^\\[C$");
    std::regex cursor_back_regex("^\\[H$");
    std::regex erase_in_place_regex("^\\[1P$");
    std::regex erase_to_eol_regex("^\\[K$");
    std::regex move_regex("^\\[(\\d+);(\\d+)H$");
    std::regex vertical_regex("^\\[(\\d+)d$");
    
    std::regex back_rel_regex("^\\[(\\d+)D$");
    std::regex front_rel_regex("^\\[(\\d+)C$");
    std::regex up_rel_regex("^\\[(\\d+)A$");
    std::regex down_rel_regex("^\\[(\\d+)B$");

    std::smatch matches;

    if (std::regex_match(seq, clear_regex)) {
        bash_screen.clear();
    } else if (std::regex_match(seq, home_regex)) {
        bash_screen.cursor_begin();
    } else if (std::regex_match(seq, cursor_up_regex)) {
        bash_screen.cursor_up();
    } else if (std::regex_match(seq, cursor_down_regex)) {
        bash_screen.cursor_down();
    } else if (std::regex_match(seq, cursor_forward_regex)) {
        bash_screen.cursor_forward();
    } else if (std::regex_match(seq, cursor_back_regex)) {
        bash_screen.cursor_back();
    } else if (std::regex_match(seq, erase_in_place_regex)) {
        bash_screen.erase_in_place();
    } else if (std::regex_match(seq, erase_to_eol_regex)) {
        bash_screen.erase_to_eol();
    } else if (std::regex_match(seq, matches, move_regex)) {
        bash_screen.move_cursor(std::stoi(matches[1].str()) - 1, std::stoi(matches[2].str()) - 1);
    } else if (std::regex_match(seq, matches, vertical_regex)) {
        bash_screen.move_cursor(std::stoi(matches[1].str()) - 1, bash_screen.get_x());
    } else if (std::regex_match(seq, matches, back_rel_regex)) {
        bash_screen.move_cursor(bash_screen.get_y(), bash_screen.get_x() - std::stoi(matches[1].str()));
    } else if (std::regex_match(seq, matches, front_rel_regex)) {
        bash_screen.move_cursor(bash_screen.get_y(), bash_screen.get_x() + std::stoi(matches[1].str()));
    } else if (std::regex_match(seq, matches, up_rel_regex)) {
        bash_screen.move_cursor(bash_screen.get_y() - std::stoi(matches[1].str()), bash_screen.get_x());
    } else if (std::regex_match(seq, matches, down_rel_regex)) {
        bash_screen.move_cursor(bash_screen.get_y() + std::stoi(matches[1].str()), bash_screen.get_x());
    }
}

class TerminalMultiplexer {
public:
    TerminalMultiplexer() : assistant_win(nullptr), bash_win(nullptr), current_win(nullptr) {
        init();
    }

    ~TerminalMultiplexer() {
        cleanup();
    }

    void run() {
        run_terminal();
    }

private:
    WINDOW* assistant_win;
    WINDOW* bash_win;
    WINDOW* current_win;
    char *shell = "/bin/bash";

    int pty_master, pty_slave;
    
    Screen bash_screen;

    void init() {
        // Create a new PTY
        if (openpty(&pty_master, &pty_slave, NULL, NULL, NULL) == -1) {
            perror("openpty");
            exit(EXIT_FAILURE);
        }

        // Fork the process
        int pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process will execute the shell
            close(pty_master);
            setsid();

            // Slave becomes the controlling terminal
            if (ioctl(pty_slave, TIOCSCTTY, NULL) == -1) {
                perror("ioctl TIOCSCTTY");
                exit(EXIT_FAILURE);
            }

            // Duplicate pty slave to standard input/output/error
            dup2(pty_slave, STDIN_FILENO);
            dup2(pty_slave, STDOUT_FILENO);
            dup2(pty_slave, STDERR_FILENO);

            // Close the pty slave as it's now duplicated
            close(pty_slave);

            // Set TERM type
            setenv("TERM", "linux-m", 1);

            // Execute the shell
            execl(shell, shell, NULL);

            // If execl fails
            perror("execl");
            exit(EXIT_FAILURE);
        } else {
            // Parent process will handle the Terminal Emulator
            close(pty_slave);

            init_nc();
        }
    }

    void init_nc() {
        WINDOW *outer_assistant_win, *outer_bash_win;

        int rows, cols;
        getmaxyx(stdscr, rows, cols);

        refresh();

        // Wait for 30 seconds before proceeding
        // sleep(30);
        // clear();
        // refresh();

        initscr();
        raw();
        nodelay(stdscr, TRUE);

        start_color();
        init_pair(1, COLOR_BLACK, COLOR_WHITE); // Focused window
        init_pair(2, COLOR_WHITE, COLOR_BLACK); // Unfocused window
        noecho();
        keypad(stdscr, TRUE);

        getmaxyx(stdscr, rows, cols);


        // Create the windows with proper spacing
        outer_assistant_win = newwin(rows / 2 - 1, cols - 4, 1, 2);
        outer_bash_win = newwin(rows / 2 - 1, cols - 4, rows / 2 + 1, 2);

        assistant_win = subwin(outer_assistant_win, rows / 2 - 3, cols - 6, 2, 3);

        int bash_lines = rows / 2 - 3;
        int bash_cols = cols - 6;

        bash_win = subwin(outer_bash_win, bash_lines, bash_cols, rows / 2 + 2, 3);

        bash_screen = Screen(bash_lines, bash_cols, bash_win);

        // Draw borders around the windows
        //box(outer_assistant_win, 0, 0);
        box(outer_assistant_win, 0, 0);
        box(outer_bash_win, 0, 0);

        // Print the prompts aligned correctly
        wprintw(assistant_win, "assistant>");

        // Set the focus on the assistant window
        current_win = assistant_win;
        wbkgd(assistant_win, COLOR_PAIR(1));
        wbkgd(bash_win, COLOR_PAIR(2));
        wrefresh(outer_bash_win);
        wrefresh(outer_assistant_win);
        wrefresh(bash_win);
        wrefresh(assistant_win);

        // TODO: remove this
        switch_focus(); 
    }

    void cleanup() {
        delwin(assistant_win);
        delwin(bash_win);
        endwin();
    }

    void switch_focus() {
        // Toggle the focus state

        bool is_assistant_focused = (current_win == assistant_win);

        if (is_assistant_focused) {
            current_win = bash_win;
        } else {
            current_win = assistant_win;
        }

        // Change the background color for the entire window
        wbkgd(assistant_win, is_assistant_focused ? COLOR_PAIR(1) : COLOR_PAIR(2));
        wbkgd(bash_win, is_assistant_focused ? COLOR_PAIR(2) : COLOR_PAIR(1));

        // Force a redraw of the entire window to preserve previous content
        touchwin(assistant_win);
        touchwin(bash_win);
        wrefresh(assistant_win);
        wrefresh(bash_win);

        // Clear and reset the prompt in the new focused window
        // command_len = 0;
        // memset(command, 0, sizeof(command));

        // Find the current cursor position
        int y, x;
        getyx(current_win, y, x);
        if (x >= getmaxx(current_win) - 1) {
            x = 0;
            y++;
        }

        // If we're at the bottom, scroll the window
        if (y >= getmaxy(current_win) - 1) {
            wscrl(current_win, 1);
            y = getmaxy(current_win) - 2;
        }

        wmove(current_win, y, x); // Move cursor to the correct position
        wrefresh(current_win);

    }

    // void execute_command(const std::string& command) {
    //     int x, y;
    //     getyx(current_win, y, x);

    //     wmove(current_win, y, 1);
    //     wclrtoeol(current_win);
    //     wrefresh(current_win);

    //     FILE* fp = popen(command.c_str(), "r");
    //     if (fp == nullptr) {
    //         wprintw(current_win, "Failed to run command\n");
    //         wrefresh(current_win);
    //         return;
    //     }

    //     char buffer[128];
    //     while (fgets(buffer, sizeof(buffer), fp) != nullptr) {
    //         wprintw(current_win, "%s", buffer);
    //         getyx(current_win, y, x);
    //         if (y >= getmaxy(current_win) - 2) {
    //             wscrl(current_win, 1);
    //             y = getmaxy(current_win) - 2;
    //         }
    //         wmove(current_win, y, 1);
    //         wrefresh(current_win);
    //     }
    //     pclose(fp);

    //     getyx(current_win, y, x);
    //     y++;
    //     wmove(current_win, y, 1);
    //     wclrtoeol(current_win);

    //     wprintw(current_win, "%s> ", is_assistant_focused ? "assistant" : "bash");
    //     wrefresh(current_win);
    // }

    void run_terminal() {
        int ch;
        fd_set fds;

        winsize w;
        memset(&w, 0, sizeof(w));
        w.ws_row = bash_screen.get_n_lines();
        w.ws_col = bash_screen.get_n_cols();
        int rc = ioctl(pty_master, TIOCSWINSZ, &w);
        if (rc < 0) {
            perror("ioctl");
            exit(EXIT_FAILURE);
        }

        while (1) {
            FD_ZERO(&fds);
            
            // Add stdin (user input)
            FD_SET(STDIN_FILENO, &fds);

            // Add pty (shell output)
            FD_SET(pty_master, &fds);

            select(pty_master + 1, &fds, NULL, NULL, NULL);

            // User input
            if (FD_ISSET(STDIN_FILENO, &fds)) {
                int n = handle_input();
                if (n == 0) {
                    break;
                }
            }

            // Shell output
            if (FD_ISSET(pty_master, &fds)) {
                int n = handle_shell_output(bash_win, pty_master);
                if (n <= 0) { 
                    break;
                }
            }
        }
    }

    int handle_shell_output(WINDOW *window, int fd) {
        static int escape_status = ANSI_NULL;
        static std::string escape_seq = "";

        char buffer[256];
        int n = read(pty_master, buffer, sizeof(buffer));

        if (n < -1) {
            // PTY set EIO (-1) for closure for some reason...
            perror("read");
            exit(EXIT_FAILURE);
        }

        if (n > 0) {
            buffer[n] = '\0';
            
            for (int i = 0; i < n; i++) {
                // CR
                if (escape_status == ANSI_NULL && buffer[i] == 0x0d) {
                    bash_screen.cursor_return();
                    continue;
                }

                if (escape_status == ANSI_NULL && buffer[i] == '\n') {
                    bash_screen.newline();
                    continue;
                }

                // BEL (ignore)
                if (escape_status == ANSI_NULL && buffer[i] == 0x07) {
                    continue;
                }

                // Disable alt charset (ignore)
                if (escape_status == ANSI_NULL && buffer[i] == 0x0f) {
                    continue;
                }

                // BKSP
                if (escape_status == ANSI_NULL && buffer[i] == 0x08) {
                    bash_screen.cursor_back();
                    continue;
                }

                // ESC sequence
                if (buffer[i] == 0x1B) {
                    escape_status = ANSI_IN_ESCAPE;
                    continue;
                }

                if (escape_status == ANSI_IN_ESCAPE) {
                    escape_seq += buffer[i];

                    if (buffer[i] == '[') {
                        escape_status = ANSI_IN_ESCAPE_CSI;
                    } else if (buffer[i] == 0x5C) {
                        escape(escape_seq, bash_screen);
                        escape_status = ANSI_NULL;
                        escape_seq = "";
                    } else {
                        escape_status = ANSI_IN_ESCAPE_OTHER;
                    }

                    continue;
                }

                if (escape_status == ANSI_IN_ESCAPE_CSI) {
                    escape_seq += buffer[i];

                    if (buffer[i] >= 0x40 && buffer[i] <= 0x7E) {
                        escape(escape_seq, bash_screen);
                        escape_status = ANSI_NULL;
                        escape_seq = "";
                    }

                    continue;
                }

                if (escape_status == ANSI_IN_ESCAPE_OTHER) {
                    if (buffer[i] == 0x9C) {
                        escape(escape_seq, bash_screen);
                        escape_status = ANSI_NULL;
                        escape_seq = "";
                    }

                    continue;
                }

                bash_screen.write_char(buffer[i]);
            }

        }

        return n;
    }

    int handle_input() {
        int ch;

        if (current_win == bash_win) {
            ch = wgetch(bash_win);
            handle_shell_input(ch);
        } else {
            ch = wgetch(assistant_win);
            // Pressed ^D
            if (ch == 0x04) {
                return 0;
            }
        }

        return 1;
    }

    void handle_shell_input(int ch) {
        write(pty_master, &ch, 1);
    }

    // void handle_input(int ch) {
    //     if (ch == '\t' && (ch & CTRL('I'))) { // Ctrl + Tab
    //         switch_focus();
    //     } else {
    //         switch (ch) {
    //             case '\n':
    //                 if (command_len > 0) {
    //                     command[command_len] = '\0';
    //                     std::string command_str(command);
    //                     execute_command(command_str);
    //                     command_len = 0;
    //                     memset(command, 0, sizeof(command));
    //                 }
    //                 break;
    //             case KEY_BACKSPACE:
    //             case 127:
    //                 if (command_len > 0) {
    //                     command_len--;
    //                     int x, y;
    //                     getyx(current_win, y, x);
    //                     mvwdelch(current_win, y, x - 1);
    //                 }
    //                 break;
    //             default:
    //                 if (command_len < sizeof(command) - 1) {
    //                     int y, x;
    //                     getyx(current_win, y, x);
    //                     if (x < getmaxx(current_win) - 2 && y > 0 && y < getmaxy(current_win) - 1) {
    //                         command[command_len++] = ch;
    //                         waddch(current_win, ch);
    //                     } else if (x >= getmaxx(current_win) - 2) {
    //                         x = 1;
    //                         y++;
    //                         if (y < getmaxy(current_win) - 1) {  
    //                             wmove(current_win, y, x);
    //                             command[command_len++] = ch;
    //                             waddch(current_win, ch);
    //                         }
    //                     }
    //                 }
    //             break;

    //         }
    //         wrefresh(current_win);
    //     }
    // }
};

int main() {
    TerminalMultiplexer tmux;
    tmux.run();
    return 0;
}