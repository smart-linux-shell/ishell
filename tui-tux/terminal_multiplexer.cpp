#include <ncurses.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <pty.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <string>

#include "screen.hpp"
#include "utils.hpp"
#include "assistant.hpp"
#include "escape.hpp"

#include "terminal_multiplexer.hpp"

TerminalMultiplexer::TerminalMultiplexer() : assistant_win(nullptr), bash_win(nullptr) {
    init();
}

TerminalMultiplexer::~TerminalMultiplexer() {
    cleanup();
}

void TerminalMultiplexer::run() {
    run_terminal();
}

void TerminalMultiplexer::init() {
    // Create a new PTY
    if (openpty(&pty_bash_master, &pty_bash_slave, NULL, NULL, NULL) == -1) {
        perror("openpty: bash");
        exit(EXIT_FAILURE);
    }

    // Fork the process
    int pid = fork();

    if (pid < 0) {
        perror("fork: bash_pty");
        exit(EXIT_FAILURE);
    } 
    
    if (pid == 0) {
        // Child process will execute the shell
        close(pty_bash_master);
        setsid();

        // Slave becomes the controlling terminal
        if (ioctl(pty_bash_slave, TIOCSCTTY, NULL) == -1) {
            perror("ioctl TIOCSCTTY");
            exit(EXIT_FAILURE);
        }

        // Duplicate pty slave to standard input/output/error
        dup2(pty_bash_slave, STDIN_FILENO);
        dup2(pty_bash_slave, STDOUT_FILENO);
        dup2(pty_bash_slave, STDERR_FILENO);

        // Close the pty slave as it's now duplicated
        close(pty_bash_slave);

        // Set TERM type
        setenv("TERM", "linux-m", 1);

        // Execute the shell
        execl(shell, shell, NULL);

        // If execl fails
        perror("execl");
        exit(EXIT_FAILURE);
    }

    close(pty_bash_slave);

    if (openpty(&pty_assistant_master, &pty_assistant_slave, NULL, NULL, NULL) == -1) {
        perror("openpty: assistant");
        exit(EXIT_FAILURE);
    }

    // Fork again
    pid = fork();

    if (pid < 0) {
        perror("fork: assistant_pty");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Run the assistant
        close(pty_bash_master);
        close(pty_assistant_master);

        // Duplicate pty slave
        dup2(pty_assistant_slave, STDIN_FILENO);
        dup2(pty_assistant_slave, STDOUT_FILENO);
        dup2(pty_assistant_slave, STDERR_FILENO);

        close(pty_assistant_slave);

        // Run assistant
        assistant();

        // Exit
        exit(EXIT_SUCCESS);
    }

    close(pty_assistant_slave);

    // Parent process will handle the Terminal Emulator
    // Set the masters as non-blocking
    int rv = fcntl(pty_bash_master, F_GETFL, 0);
    if (rv < 0) {
        perror("fcntl: F_GETFL");
        exit(EXIT_FAILURE);
    }

    rv |= O_NONBLOCK;
    if (fcntl(pty_bash_master, F_SETFL, rv) < 0) {
        perror("fcntl: F_SETFL");
        exit(EXIT_FAILURE);
    }

    rv = fcntl(pty_assistant_master, F_GETFL, 0);
    if (rv < 0) {
        perror("fcntl: F_GETFL");
        exit(EXIT_FAILURE);
    }

    rv |= O_NONBLOCK;
    if (fcntl(pty_assistant_master, F_SETFL, rv) < 0) {
        perror("fcntl: F_SETFL");
        exit(EXIT_FAILURE);
    }

    init_nc();
}

void TerminalMultiplexer::init_nc() {
    initscr();
    raw();
    nodelay(stdscr, TRUE);

    start_color();
    init_pair(1, COLOR_BLACK, COLOR_WHITE); // Focused window
    init_pair(2, COLOR_WHITE, COLOR_BLACK); // Unfocused window
    noecho();

    create_wins_draw(NULL, NULL);
}

void TerminalMultiplexer::refresh_cursor() {
    if (focus == FOCUS_ASSISTANT) {
        wcursyncup(assistant_win);
        wrefresh(assistant_win);
    } else if (focus == FOCUS_BASH) {
        wcursyncup(bash_win);
        wrefresh(bash_win);
    }
}

void TerminalMultiplexer::draw_focus() {
    // Change the background color for the entire window
    wbkgd(assistant_win, (focus == FOCUS_ASSISTANT) ? COLOR_PAIR(1) : COLOR_PAIR(2));
    wbkgd(bash_win, (focus == FOCUS_ASSISTANT) ? COLOR_PAIR(2) : COLOR_PAIR(1));

    wrefresh(assistant_win);
    wrefresh(bash_win);

    refresh_cursor();
}

void TerminalMultiplexer::switch_focus() {
    // Toggle the focus state

    if (focus == FOCUS_NONE) {
        focus = FOCUS_ASSISTANT;
    } else if (focus == FOCUS_ASSISTANT) {
        focus = FOCUS_BASH;
    } else {
        focus = FOCUS_ASSISTANT;
    }

    draw_focus();
}

void TerminalMultiplexer::create_wins_draw(Screen *old_bash_screen, Screen *old_assistant_screen) {
    int rows, cols;

    getmaxyx(stdscr, rows, cols);

    // Create the windows with proper spacing
    outer_assistant_win = newwin(rows / 2 - 1, cols - 4, 1, 2);
    outer_bash_win = newwin(rows / 2 - 1, cols - 4, rows / 2 + 1, 2);

    int assistant_lines = rows / 2 - 3;
    int assistant_cols = cols - 6;

    assistant_win = subwin(outer_assistant_win, rows / 2 - 3, cols - 6, 2, 3);

    int bash_lines = rows / 2 - 3;
    int bash_cols = cols - 6;

    bash_win = subwin(outer_bash_win, bash_lines, bash_cols, rows / 2 + 2, 3);

    if (old_bash_screen == NULL) {
        bash_screen = Screen(bash_lines, bash_cols, bash_win);
    } else {
        bash_screen = Screen(bash_lines, bash_cols, bash_win, *old_bash_screen);
    }

    if (old_assistant_screen == NULL) {
        assistant_screen = Screen(assistant_lines, assistant_cols, assistant_win);
    } else {
        assistant_screen = Screen(assistant_lines, assistant_cols, assistant_win, *old_assistant_screen);
    }

    // enable these = can no longer bksp in bash.
    //keypad(bash_win, TRUE);
    //keypad(assistant_win, TRUE);

    // Draw borders around the windows
    box(outer_assistant_win, 0, 0);
    box(outer_bash_win, 0, 0);

    wrefresh(outer_bash_win);
    wrefresh(outer_assistant_win);

    if (focus == FOCUS_NONE) {
        switch_focus();
    } else {
        draw_focus();
    }
}

void TerminalMultiplexer::delete_windows() {
    delwin(outer_assistant_win);
    delwin(outer_bash_win);
    delwin(assistant_win);
    delwin(bash_win);
}

void TerminalMultiplexer::cleanup() {
    delete_windows();
    endwin();
}

void TerminalMultiplexer::send_dims() {
    winsize w;
    memset(&w, 0, sizeof(w));
    w.ws_row = bash_screen.get_n_lines();
    w.ws_col = bash_screen.get_n_cols();
    int rc = ioctl(pty_bash_master, TIOCSWINSZ, &w);
    if (rc < 0) {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
}

void TerminalMultiplexer::resize() {
    delete_windows();
    clear();
    refresh();
    create_wins_draw(&bash_screen, &assistant_screen);
    send_dims();
}

void TerminalMultiplexer::run_terminal() {
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

    // Add bash pty (shell output)
    event.events = EPOLLIN;
    event.data.fd = pty_bash_master;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pty_bash_master, &event) == -1) {
        perror("epoll_ctl: pty_bash_master");
        exit(EXIT_FAILURE);
    }

    // Add assistant pty (assistant output)
    event.events = EPOLLIN;
    event.data.fd = pty_assistant_master;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pty_assistant_master, &event) == -1) {
        perror("epoll_ctl: pty_assistant_master");
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
            } else if (events[i].data.fd == pty_bash_master) {
                // Shell output
                int n_pty = handle_screen_output(bash_screen, pty_bash_master);
                if (n_pty <= 0) { 
                    epolling = false;
                    break;
                }
            } else if (events[i].data.fd == pty_assistant_master) {
                // Assistant output
                int n_pty = handle_screen_output(assistant_screen, pty_assistant_master);
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

int TerminalMultiplexer::handle_screen_output(Screen &screen, int fd) {
    static int escape_status = ANSI_NULL;
    static std::string escape_seq = "";

    char buffer[256];
    int n = read(fd, buffer, sizeof(buffer));

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
                screen.cursor_return();
                continue;
            }

            if (escape_status == ANSI_NULL && buffer[i] == '\n') {
                screen.newline();
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
                screen.cursor_back();
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
                    escape(escape_seq, screen);
                    escape_status = ANSI_NULL;
                    escape_seq = "";
                } else if ((buffer[i] >= 0x40 && buffer[i] <= 0x7E) || (buffer[i] == 0x9C)) {
                    escape_seq += buffer[i];
                    escape(escape_seq, screen);
                    escape_status = ANSI_NULL;
                    escape_seq = "";
                } else {
                    escape_seq += buffer[i];
                }

                continue;
            }

            screen.write_char(buffer[i]);
        }

    }

    refresh_cursor();

    return n;
}

int TerminalMultiplexer::handle_input() {
    /*
    problem when using wgetch: KEY_RESIZE does not wake up the epoll event.
    solution: use custom SIGWINCH handler

    new problem: once too many unread KEY_RESIZEs accumulate, wgetch breaks. (it will return KEY_RESIZE on each call)
    therefore, read straight from stdin; do not use wgetch.
    */

    char buf[256];
    int n = read(STDIN_FILENO, buf, sizeof(buf));

    if (n < 0) {
        perror("read");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < n; i++) {
        char ch = buf[i];
        
        if (ch == 0x11) {
            // Pressed ^Q
            switch_focus();
        } else if (focus == FOCUS_BASH) {
            handle_pty_input(pty_bash_master, ch);
        } else if (focus == FOCUS_ASSISTANT) {
            handle_pty_input(pty_assistant_master, ch);
        }
    }

    return n;
}

void TerminalMultiplexer::handle_pty_input(int fd, char ch) {
    if (ch == 'j') {
        int ch2 = KEY_UP;
        write(fd, &ch2, 1);
    } else {
        write(fd, &ch, 1);
    }
}