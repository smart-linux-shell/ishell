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

#include <screen.hpp>
#include <utils.hpp>
#include <assistant.hpp>
#include <escape.hpp>

#include <terminal_multiplexer.hpp>

#define MAGENTA_FOREGROUND 1
#define WHITE_FOREGROUND 2
#define WHITE_ON_MAGENTA 3

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
        setenv("TERM", "ishell-m", 1);

        // Execute the shell
        execl(shell, shell, NULL);

        // If execl fails
        perror("execl");
        exit(EXIT_FAILURE);
    }

    close(pty_bash_slave);

    int pty_assistant_master, pty_assistant_slave;
    if (openpty(&pty_assistant_master, &pty_assistant_slave, NULL, NULL, NULL) == -1) {
        perror("openpty: agent");
        exit(EXIT_FAILURE);
    }

    // Fork again
    int assistant_pid = fork();

    if (assistant_pid < 0) {
        perror("fork: assistant_pty");
        exit(EXIT_FAILURE);
    }

    if (assistant_pid == 0) {
        // Run the agent
        close(pty_bash_master);
        close(pty_assistant_master);

        // Duplicate pty slave
        dup2(pty_assistant_slave, STDIN_FILENO);
        dup2(pty_assistant_slave, STDOUT_FILENO);
        dup2(pty_assistant_slave, STDERR_FILENO);

        close(pty_assistant_slave);

        // Set TERM type
        setenv("TERM", "ishell-m", 1);

        // Run agent
        agent();

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

    // Create temporary unsized screens
    screens.push_back(Screen(0, 0, pty_assistant_master, assistant_pid));
    screens.push_back(Screen(0, 0, pty_bash_master, bash_pid));

    init_nc();
}

void TerminalMultiplexer::init_nc() {
    initscr();
    start_color();
    use_default_colors();
    raw();
    nodelay(stdscr, TRUE);

    start_color();
    init_pair(MAGENTA_FOREGROUND, COLOR_MAGENTA, -1); // Magenta foreground
    init_pair(WHITE_FOREGROUND, COLOR_WHITE, -1); // White foreground
    init_pair(WHITE_ON_MAGENTA, COLOR_WHITE, COLOR_MAGENTA); // White foreground, Magenta background
    noecho();

    create_wins_draw();
}

void TerminalMultiplexer::refresh_cursor() {
    if (focus != FOCUS_NULL) {
        if (screens[focus].is_in_manual_scroll()) {
            curs_set(0);
        } else {
            curs_set(1);
        }

        wcursyncup(screens[focus].get_pad());
        screens[focus].refresh_screen();
    }
}

void TerminalMultiplexer::draw_focus() {
    // Change the background color for the entire window
    int cols = getmaxx(middle_divider);

    int assistant_color = WHITE_FOREGROUND;
    int bash_color = WHITE_FOREGROUND;

    if (focus == FOCUS_ASSISTANT) {
        assistant_color = MAGENTA_FOREGROUND;
    } else if (focus == FOCUS_BASH) {
        bash_color = MAGENTA_FOREGROUND;
    }

    wattron(middle_divider, COLOR_PAIR(assistant_color));
    mvwhline(middle_divider, 0, 0, 0, cols / 2);
    wattroff(middle_divider, COLOR_PAIR(assistant_color));

    wattron(middle_divider, COLOR_PAIR(bash_color));
    mvwhline(middle_divider, 0, cols / 2, 0, cols - cols / 2);
    wattroff(middle_divider, COLOR_PAIR(bash_color));

    wrefresh(middle_divider);
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

    int middle_row = (rows - 1) / 2;
    int assistant_lines = middle_row;
    int assistant_cols = cols;

    int bash_lines = rows - middle_row - 2;
    int bash_cols = cols;

    int assistant_y = 0, assistant_x = 0;
    int bash_y = middle_row + 1, bash_x = 0;

    // Create windows for bottom bar and middle divider
    if (bottom_bar != NULL) {
        delwin(bottom_bar);
    }

    if (middle_divider != NULL) {
        delwin(middle_divider);
    }

    bottom_bar = newwin(1, cols, rows - 1, 0);
    middle_divider = newwin(1, cols, middle_row, 0);

    wbkgd(bottom_bar, COLOR_PAIR(WHITE_ON_MAGENTA));
    wprintw(bottom_bar, "ishell");
    wrefresh(bottom_bar);

    if (zoomed_in) {
        if (focus == FOCUS_ASSISTANT) {
            assistant_lines = rows - 1;
            bash_y = -1;
            bash_x = -1;
        } else if (focus == FOCUS_BASH) {
            bash_lines = rows - 1;
            bash_y = 0;
            assistant_y = -1;
            assistant_x = -1;
        }
    }

    // Resize old screens
    std::vector<Screen> new_screens;
    
    // Assistant
    Screen screen = Screen(assistant_lines, assistant_cols, screens[0]);
    screen.set_screen_coords(assistant_y, assistant_x, assistant_y + assistant_lines - 1, assistant_x + assistant_cols - 1);

    new_screens.push_back(screen);

    // Bash
    screen = Screen(bash_lines, bash_cols, screens[1]);
    screen.set_screen_coords(bash_y, bash_x, bash_y + bash_lines - 1, bash_x + bash_cols - 1);

    new_screens.push_back(screen);

    delete_windows();

    screens = new_screens;

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

    for (WINDOW *window : windows) {
        delwin(window);
    }

    windows.clear();
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

    // Make sigfd non-blocking (a potential spurious wake-up might block the app)
    int flags = fcntl(sigfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl: F_GETFL sigfd");
        exit(EXIT_FAILURE);
    }

    if (fcntl(sigfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl: F_SETFL sigfd");
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
                // this happens sometimes
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
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        // Spurious wake-up, ignore
                        continue;
                    }
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
    int bytes_read = 0;

    while (1) {
        std::vector<TerminalChar> chars;
        int n = read_and_escape(fd, chars);

        if (n < 0) {
            if (errno == EIO) {
                // PTY set EIO (-1) for closure for some reason...
                return -1;
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Done reading. Do not return a 0 if spurious
                if (bytes_read == 0) {
                    bytes_read = 1;
                }
                break;
            }

            perror("read");
            exit(EXIT_FAILURE);
        }

        if (n > 0) {
            bytes_read += n;
            for (TerminalChar &tch : chars) {
                screen.handle_char(tch);
            }
        }
    }

    if (bytes_read > 0) {
        screen.refresh_screen();
        refresh_cursor();
    }

    return bytes_read;
}

int TerminalMultiplexer::handle_input() {
    /*
    problem when using wgetch: KEY_RESIZE does not wake up the epoll event.
    solution: use custom SIGWINCH handler

    new problem: once too many unread KEY_RESIZEs accumulate, wgetch breaks. (it will return KEY_RESIZE on each call)
    therefore, read straight from stdin; do not use wgetch.
    */

    std::vector<TerminalChar> chars;
    int n = read_and_escape(STDIN_FILENO, chars);

    if (n < 0) {
        perror("read");
        exit(EXIT_FAILURE);
    }

    for (TerminalChar &tch : chars) {        
        if (tch.ch == 0x02) {
            // Pressed ^B
            waiting_for_command = true;
        } else if (waiting_for_command) {
            waiting_for_command = false;
            if (tch.ch == '\t') {
                switch_focus();
            } else if (tch.ch == 'Z' || tch.ch == 'z') {
                if (!zoomed_in) {
                    zoom_in();
                } else {
                    zoom_out();
                }
            } else if (tch.ch == '[') {
                toggle_manual_scroll();
            }
        } else if (focus != FOCUS_NULL) {
            if (screens[focus].is_in_manual_scroll()) {
                if (tch.ch == E_KEY_CUU) {
                    screens[focus].manual_scroll_up();
                    refresh_cursor();
                } else if (tch.ch == E_KEY_CUD) {
                    screens[focus].manual_scroll_down();
                    refresh_cursor();
                }
            } else {
                for (char ch : tch.sequence) {
                    handle_pty_input(screens[focus].get_pty_master(), ch);
                }
            }
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

void TerminalMultiplexer::toggle_manual_scroll() {
    if (focus != FOCUS_NULL) {
        if (screens[focus].is_in_manual_scroll()) {
            screens[focus].reset_manual_scroll();
            refresh_cursor();
        } else {
            screens[focus].enter_manual_scroll();
            refresh_cursor();
        }
    }
}