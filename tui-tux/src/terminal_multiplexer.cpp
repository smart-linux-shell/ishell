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

#include "../include/screen.hpp"
#include "../include/utils.hpp"
#include "../include/assistant.hpp"
#include "../include/escape.hpp"

#include "../include/terminal_multiplexer.hpp"

TerminalMultiplexer::TerminalMultiplexer() {
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
    int pty_bash_master, pty_bash_slave;

    if (openpty(&pty_bash_master, &pty_bash_slave, NULL, NULL, NULL) == -1) {
        perror("openpty: bash");
        exit(EXIT_FAILURE);
    }

    // Fork the process
    int bash_pid = fork();

    if (bash_pid < 0) {
        perror("fork: bash_pty");
        exit(EXIT_FAILURE);
    } 
    
    if (bash_pid == 0) {
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

    int pty_assistant_master, pty_assistant_slave;
    if (openpty(&pty_assistant_master, &pty_assistant_slave, NULL, NULL, NULL) == -1) {
        perror("openpty: assistant");
        exit(EXIT_FAILURE);
    }

    // Fork again
    int assistant_pid = fork();

    if (assistant_pid < 0) {
        perror("fork: assistant_pty");
        exit(EXIT_FAILURE);
    }

    if (assistant_pid == 0) {
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

    fgetc(stdin);

    // Create temporary unsized screens
    screens.push_back(Screen(0, 0, NULL, NULL, pty_assistant_master, assistant_pid));
    screens.push_back(Screen(0, 0, NULL, NULL, pty_bash_master, bash_pid));

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

    create_wins_draw();
}

void TerminalMultiplexer::refresh_cursor() {
    if (focus != FOCUS_NULL) {
        screens[focus].scursyncup();
        screens[focus].srefresh();
    }
}

void TerminalMultiplexer::draw_focus() {
    // Change the background color for the entire window
    for (Screen &screen : screens) {
        // Set unfocused colours
        screen.sbkgd(COLOR_PAIR(2));
    }

    // Set focused colour
    screens[focus].sbkgd(COLOR_PAIR(1));

    for (Screen &screen : screens) {
        screen.srefresh();
    }

    refresh_cursor();
}

void TerminalMultiplexer::switch_focus() {
    // Toggle the focus state

    if (zoomed_in) {
        // Reject
        return;
    }

    if (focus == FOCUS_NULL) {
        focus = 0;
    } else {
        focus = (focus + 1) % screens.size();
    }

    draw_focus();
}

void TerminalMultiplexer::create_wins_draw() {
    int rows, cols;

    getmaxyx(stdscr, rows, cols);

    // Create the windows with proper spacing
    WINDOW *outer_assistant_win, *outer_bash_win, *assistant_win, *bash_win;
    int assistant_lines = rows / 2 - 3;
    int assistant_cols = cols - 6;

    int bash_lines = rows / 2 - 3;
    int bash_cols = cols - 6;

    if (zoomed_in) {
        if (focus == 0) {
            // First is the assistant window.
            assistant_lines = rows - 3;
            outer_assistant_win = newwin(assistant_lines + 2, assistant_cols + 2, 1, 2);
            outer_bash_win = NULL;
            assistant_win = subwin(outer_assistant_win, assistant_lines, assistant_cols, 2, 3);
            bash_win = NULL;

        } else {
            bash_lines = rows - 3;
            outer_assistant_win = NULL;
            outer_bash_win = newwin(bash_lines + 2, bash_cols + 2, 1, 2);
            assistant_win = NULL;
            bash_win = subwin(outer_bash_win, bash_lines, bash_cols, 2, 3);
        }
    } else {
        outer_assistant_win = newwin(assistant_lines + 2, assistant_cols + 2, 1, 2);
        outer_bash_win = newwin(bash_lines + 2, bash_cols + 2, rows / 2 + 1, 2);
        assistant_win = subwin(outer_assistant_win, assistant_lines, assistant_cols, 2, 3);
        bash_win = subwin(outer_bash_win, bash_lines, bash_cols, rows / 2 + 2, 3);
    }

    // Resize old screens
    std::vector<Screen> new_screens;
    
    // Assistant
    new_screens.push_back(Screen(assistant_lines, assistant_cols, assistant_win, outer_assistant_win, screens[0]));

    // Bash
    new_screens.push_back(Screen(bash_lines, bash_cols, bash_win, outer_bash_win, screens[1]));

    screens = new_screens;

    // Draw borders around the windows
    for (Screen &screen : screens) {
        screen.sbox_outer(0, 0);
        screen.srefresh_outer();
        screen.srefresh();
    }

    if (focus == FOCUS_NULL) {
        switch_focus();
    } else {
        draw_focus();
    }
}

void TerminalMultiplexer::delete_windows() {
    for (Screen &screen : screens) {
        screen.delete_wins();
    }
}

void TerminalMultiplexer::cleanup() {
    delete_windows();
    endwin();
}

void TerminalMultiplexer::send_dims() {
    for (Screen &screen : screens) {
        winsize w;
        memset(&w, 0, sizeof(w));
        w.ws_row = screen.get_n_lines();
        w.ws_col = screen.get_n_cols();
        int rc = ioctl(screen.get_pty_master(), TIOCSWINSZ, &w);
        if (rc < 0) {
            perror("ioctl");
            exit(EXIT_FAILURE);
        }
        rc = kill(screen.get_pid(), SIGWINCH);
        if (rc < 0) {
            perror("kill: SIGWINCH");
            exit(EXIT_FAILURE);
        }
    }
}

void TerminalMultiplexer::resize() {
    delete_windows();
    clear();
    refresh();
    create_wins_draw();
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

    // Add ptys to epoll instance
    for (size_t i = 0; i < screens.size(); i++) {
        struct epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = screens[i].get_pty_master();
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event.data.fd, &event) == -1) {
            perror("epoll_ctl: pty");
            exit(EXIT_FAILURE);
        }
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
            } else {
                // A PTY
                for (Screen &screen : screens) {
                    if (screen.get_pty_master() == events[i].data.fd) {
                        int n_pty = handle_screen_output(screen, events[i].data.fd);
                        if (n_pty <= 0) {
                            epolling = false;
                            break;
                        }

                        break;
                    }
                }
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
        
        if (ch == 0x02) {
            // Pressed ^B
            waiting_for_command = true;
        } else if (waiting_for_command) {
            waiting_for_command = false;
            if (ch == '\t') {
                switch_focus();
            } else if (ch == 'Z' || ch == 'z') {
                if (!zoomed_in) {
                    zoom_in();
                } else {
                    zoom_out();
                }
            } else if (ch == '[') {
                
            }
        } else if (focus != FOCUS_NULL) {
            handle_pty_input(screens[focus].get_pty_master(), ch);
        }
    }

    return n;
}

void TerminalMultiplexer::handle_pty_input(int fd, char ch) {
    write(fd, &ch, 1);
}

void TerminalMultiplexer::zoom_in() {
    zoomed_in = true;
    resize();
}

void TerminalMultiplexer::zoom_out() {
    zoomed_in = false;
    resize();
}