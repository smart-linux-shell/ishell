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
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <stdlib.h>

#define ANSI_NULL 0
#define ANSI_IN_ESCAPE 1

#define FOCUS_NONE 0
#define FOCUS_ASSISTANT 1
#define FOCUS_BASH 2

#define MAX_EVENTS 5

class Screen {
public:
    Screen() {}

    Screen(int lines, int cols, WINDOW *window) {
        init(lines, cols, window);
    }

    Screen(int lines, int cols, WINDOW *window, Screen &old_screen) {
        init(lines, cols, window, old_screen);
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

                curx = getcurx(window);
                cury = getcury(window);
            }
        } else {
            // Just write
            waddch(window, ch);
        }

        lines[cury].data[curx] = ch;

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

        for (int i = 1; i < n_lines; i++) {
            lines[i - 1] = lines[i];
            for (int j = 0; j < n_cols; j++) {
                if (lines[i].data[j] != 0) {
                    mvwaddch(window, i - 1, j, lines[i].data[j]);
                } else {
                    break;
                }
            }
        }

        lines[n_lines - 1].data = new char[n_cols];
        lines[n_lines - 1].wrapped = false;
        memset(lines[n_lines - 1].data, 0, n_cols);

        move_cursor(n_lines - 1, 0);
    }

    void scroll_up() {
        wclear(window);
        
        // Move the line data first
        for (int i = n_lines - 1; i > 0; i--) {
            lines[i] = lines[i - 1];
        }

        lines[0].data = new char[n_cols];
        lines[0].wrapped = false;
        memset(lines[0].data, 0, n_cols);

        // Redraw
        wclear(window);

        for (int i = 1; i < n_lines; i++) {
            for (int j = 0; j < n_cols; j++) {
                if (lines[i].data[j] != 0) {
                    mvwaddch(window, i, j, lines[i].data[j]);
                } else {
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

    void init(int new_lines, int new_cols, WINDOW *new_window, Screen &old_screen) {
        init(new_lines, new_cols, new_window);

        int new_y = n_lines - 1;
        std::string line = "";
        bool begun = false;

        for (int i = old_screen.n_lines - 1; i >= 0; i--) {
            for (int j = old_screen.n_cols - 1; j >= 0; j--) {
                if (old_screen.lines[i].data[j] != 0) {
                    begun = true;
                    line = old_screen.lines[i].data[j] + line;
                }
            }

            if (begun && (i == 0 || !old_screen.lines[i - 1].wrapped)) {
                // Finish line
                int line_length = line.length();
                int translated_lines = line_length / n_cols;
                if (translated_lines * n_cols != line_length) {
                    translated_lines += 1;
                }

                int col = 0;
                int y = new_y - translated_lines + 1;

                int c_pos = 0;
                while (y < 0) {
                    y++;
                    c_pos += new_cols;
                }

                for (; c_pos < line_length; c_pos++) {
                    lines[y].data[col] = line[c_pos];
                    col++;
                    if (col == n_cols) {
                        if (y < new_y) {
                            lines[y].wrapped = true;
                        }
                        col = 0;
                        y++;
                    }
                }

                new_y = new_y - translated_lines;
                line = "";

                if (new_y < 0) {
                    // Can't fit anymore...
                    break;
                }
            }
        }

        // Bump text up if empty spaces
        if (new_y >= 0) {
            int to_bump = new_y + 1;
            for (int i = 0; i < new_lines - to_bump; i++) {
                lines[i] = lines[i + to_bump];
            }

            for (int i = new_lines - to_bump; i < new_lines; i++) {
                lines[i].data = new char[new_cols];
                memset(lines[i].data, 0, new_cols);
                lines[i].wrapped = false;
            }
        }

        // Draw text
        wclear(window);
        for (int i = 0; i < n_lines; i++) {
            for (int j = 0; j < n_cols; j++) {
                if (lines[i].data[j] != 0) {
                    mvwaddch(window, i, j, lines[i].data[j]);
                }
            }
        }

        wrefresh(window);
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

    std::regex scroll_up_regex("^M$");

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
    } else if (std::regex_match(seq, scroll_up_regex)) {
        bash_screen.scroll_up();
    }
}

class TerminalMultiplexer {
public:
    TerminalMultiplexer() : assistant_win(nullptr), bash_win(nullptr) {
        init();
    }

    ~TerminalMultiplexer() {
        cleanup();
    }

    void run() {
        run_terminal();
    }

private:
    WINDOW *assistant_win, *bash_win, *outer_assistant_win, *outer_bash_win;
    const char *shell = "/bin/bash";

    int pty_master, pty_slave;
    int focus = FOCUS_NONE;
    
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

            // Set the master as non-blocking
            int rv = fcntl(pty_master, F_GETFL, 0);
            if (rv < 0) {
                perror("fcntl: F_GETFL");
                exit(EXIT_FAILURE);
            }

            rv |= O_NONBLOCK;
            if (fcntl(pty_master, F_SETFL, rv) < 0) {
                perror("fcntl: F_SETFL");
                exit(EXIT_FAILURE);
            }

            init_nc();
        }
    }

    void init_nc() {
        initscr();
        raw();
        nodelay(stdscr, TRUE);

        start_color();
        init_pair(1, COLOR_BLACK, COLOR_WHITE); // Focused window
        init_pair(2, COLOR_WHITE, COLOR_BLACK); // Unfocused window
        noecho();
        keypad(stdscr, TRUE);

        create_wins_draw(NULL);
    }

    void create_wins_draw(Screen *old_screen) {
        int rows, cols;

        getmaxyx(stdscr, rows, cols);

        // Create the windows with proper spacing
        outer_assistant_win = newwin(rows / 2 - 1, cols - 4, 1, 2);
        outer_bash_win = newwin(rows / 2 - 1, cols - 4, rows / 2 + 1, 2);

        assistant_win = subwin(outer_assistant_win, rows / 2 - 3, cols - 6, 2, 3);

        int bash_lines = rows / 2 - 3;
        int bash_cols = cols - 6;

        bash_win = subwin(outer_bash_win, bash_lines, bash_cols, rows / 2 + 2, 3);

        if (old_screen == NULL) {
            bash_screen = Screen(bash_lines, bash_cols, bash_win);
        } else {
            bash_screen = Screen(bash_lines, bash_cols, bash_win, *old_screen);
        }

        // enable these = can no longer bksp in bash.
        //keypad(bash_win, TRUE);
        //keypad(assistant_win, TRUE);

        // Draw borders around the windows
        //box(outer_assistant_win, 0, 0);
        box(outer_assistant_win, 0, 0);
        box(outer_bash_win, 0, 0);

        // Print the prompts aligned correctly
        wprintw(assistant_win, "assistant>");

        // Set the focus on the assistant window if unfocused
        if (focus == FOCUS_NONE) {
            focus = FOCUS_ASSISTANT;
        }

        wbkgd(assistant_win, (focus == FOCUS_ASSISTANT) ? COLOR_PAIR(1) : COLOR_PAIR(2));
        wbkgd(bash_win, (focus == FOCUS_ASSISTANT) ? COLOR_PAIR(2) : COLOR_PAIR(1));
        wrefresh(outer_bash_win);
        wrefresh(outer_assistant_win);
        wrefresh(bash_win);
        wrefresh(assistant_win);
    }

    void delete_windows() {
        delwin(outer_assistant_win);
        delwin(outer_bash_win);
        delwin(assistant_win);
        delwin(bash_win);
    }

    void cleanup() {
        delete_windows();
        endwin();
    }

    void switch_focus() {
        // Toggle the focus state

        if (focus == FOCUS_ASSISTANT) {
            focus = FOCUS_BASH;
        } else {
            focus = FOCUS_ASSISTANT;
        }

        // Change the background color for the entire window
        wbkgd(assistant_win, (focus == FOCUS_ASSISTANT) ? COLOR_PAIR(1) : COLOR_PAIR(2));
        wbkgd(bash_win, (focus == FOCUS_ASSISTANT) ? COLOR_PAIR(2) : COLOR_PAIR(1));

        wrefresh(assistant_win);
        wrefresh(bash_win);
    }

    void send_dims() {
        winsize w;
        memset(&w, 0, sizeof(w));
        w.ws_row = bash_screen.get_n_lines();
        w.ws_col = bash_screen.get_n_cols();
        int rc = ioctl(pty_master, TIOCSWINSZ, &w);
        if (rc < 0) {
            perror("ioctl");
            exit(EXIT_FAILURE);
        }
    }

    void resize() {
        send_dims();
        delete_windows();
        clear();
        refresh();
        create_wins_draw(&bash_screen);
    }

    void run_terminal() {
        fd_set fds;

        send_dims();        

        // Create an epoll instance
        int epoll_fd = epoll_create1(0);
        if (epoll_fd == -1) {
            perror("epoll_create1");
            exit(EXIT_FAILURE);
        }

        // Add stdin to the epoll instance
        struct epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = STDIN_FILENO;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &event) == -1) {
            perror("epoll_ctl: stdin");
            exit(EXIT_FAILURE);
        }

        // Add pty (shell output)
        event.events = EPOLLIN;
        event.data.fd = pty_master;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pty_master, &event) == -1) {
            perror("epoll_ctl: pty_master");
            exit(EXIT_FAILURE);
        }

        struct sigaction sa_old;
        sigset_t mask;

        // Get the existing signal handler for SIGWINCH
        if (sigaction(SIGWINCH, NULL, &sa_old) == -1) {
            perror("sigaction");
            exit(EXIT_FAILURE);
        }

        // Block the SIGWINCH signal
        sigemptyset(&mask);
        sigaddset(&mask, SIGWINCH);
        if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
            perror("sigprocmask");
            exit(EXIT_FAILURE);
        }

        // Add signal SIGWINCH to catch window resizes
        sigemptyset(&mask);
        sigaddset(&mask, SIGWINCH);

        int sigfd = signalfd(-1, &mask, 0);
        if (sigfd < 0) {
            perror("signalfd");
            exit(EXIT_FAILURE);
        }

        event.events = EPOLLIN;
        event.data.fd = sigfd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sigfd, &event) == -1) {
            perror("epoll_ctl: sigfd");
            exit(EXIT_FAILURE);
        }

        struct epoll_event events[MAX_EVENTS];
        bool epolling = true;

        while (epolling) {
            int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

            if (n < 0) {
                if (errno == EINTR) {
                    // :(
                    continue;
                }

                perror("epoll_wait");
                exit(EXIT_FAILURE);
            }

            for (int i = 0; i < n; i++) {
                if (events[i].data.fd == STDIN_FILENO) {
                    // User input
                    int n_in = handle_input();
                    if (n_in == 0) {
                        epolling = false;
                        break;
                    }    
                } else if (events[i].data.fd == pty_master) {
                    // Shell output
                    int n_pty = handle_shell_output(bash_win, pty_master);
                    if (n_pty <= 0) { 
                        epolling = false;
                        break;
                    }
                } else if (events[i].data.fd == sigfd) {
                    // Read the signal
                    struct signalfd_siginfo sigfd_info;
                    ssize_t s = read(sigfd, &sigfd_info, sizeof(struct signalfd_siginfo));

                    if (s != sizeof(struct signalfd_siginfo)) {
                        perror("read signalfd");
                        exit(EXIT_FAILURE);
                    }

                    // If there was an old signal handler, call it
                    if (sa_old.sa_handler != SIG_IGN && sa_old.sa_handler != SIG_DFL) {
                        sa_old.sa_handler(SIGWINCH);
                    }

                    // Resize
                    resize();
                }
            }
        }

        close(epoll_fd);
    }

    int handle_shell_output(WINDOW *window, int fd) {
        static int escape_status = ANSI_NULL;
        static std::string escape_seq = "";

        char buffer[256];
        int n = read(pty_master, buffer, sizeof(buffer));

        if (n < 0) {
            if (errno == EIO) {
                // PTY set EIO (-1) for closure for some reason...
                return -1;
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return 1;
            }

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
                    if (buffer[i] == 0x5B) {
                        escape_seq += buffer[i];
                    } else if (buffer[i] == 0x9C) {
                        escape(escape_seq, bash_screen);
                        escape_status = ANSI_NULL;
                        escape_seq = "";
                    } else if ((buffer[i] >= 0x40 && buffer[i] <= 0x7E) || (buffer[i] == 0x9C)) {
                        escape_seq += buffer[i];
                        escape(escape_seq, bash_screen);
                        escape_status = ANSI_NULL;
                        escape_seq = "";
                    } else {
                        escape_seq += buffer[i];
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

        if (focus == FOCUS_BASH) {
            ch = wgetch(bash_win);
        } else {
            ch = wgetch(assistant_win);
        }

        if (ch == KEY_RESIZE) {
            // Handled by SIG HANDLER
        } else if (ch == 0x11) {
            // Pressed ^Q
            switch_focus();
        } else if (focus == FOCUS_BASH) {
            handle_shell_input(ch);
        } else {
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
};

int main() {
    TerminalMultiplexer tmux;
    tmux.run();
    return 0;
}