#include <ncurses.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <csignal>
#include <pty.h>
#include <cstdlib>
#include <fcntl.h>
#include <cstring>
#include <cerrno>
#include <string>
#include <stdio.h>

#include <screen.hpp>
#include <utils.hpp>
#include <agent.hpp>
#include <escape.hpp>

#include <terminal_multiplexer.hpp>
#include <session_tracker.hpp>

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

    if (openpty(&pty_bash_master, &pty_bash_slave, nullptr, nullptr, nullptr) == -1) {
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

    int pty_agent_master, pty_agent_slave;
    if (openpty(&pty_agent_master, &pty_agent_slave, nullptr, nullptr, nullptr) == -1) {
        perror("openpty: agent");
        exit(EXIT_FAILURE);
    }

    // Fork again
    int agent_pid = fork();

    if (agent_pid < 0) {
        perror("fork: agent_pty");
        exit(EXIT_FAILURE);
    }

    if (agent_pid == 0) {
        // Run the agent
        close(pty_bash_master);
        close(pty_agent_master);

        // Duplicate pty slave
        dup2(pty_agent_slave, STDIN_FILENO);
        dup2(pty_agent_slave, STDOUT_FILENO);
        dup2(pty_agent_slave, STDERR_FILENO);

        close(pty_agent_slave);

        // Set TERM type
        setenv("TERM", "ishell-m", 1);

        // Run agent
        agent();

        // Exit
        exit(EXIT_SUCCESS);
    }

    close(pty_agent_slave);

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

    rv = fcntl(pty_agent_master, F_GETFL, 0);
    if (rv < 0) {
        perror("fcntl: F_GETFL");
        exit(EXIT_FAILURE);
    }

    rv |= O_NONBLOCK;
    if (fcntl(pty_agent_master, F_SETFL, rv) < 0) {
        perror("fcntl: F_SETFL");
        exit(EXIT_FAILURE);
    }

    // Create temporary unsized screens
    screens.emplace_back(0, 0, pty_agent_master, agent_pid);
    screens.emplace_back(0, 0, pty_bash_master, bash_pid);

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

void TerminalMultiplexer::refresh_cursor() const {
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

void TerminalMultiplexer::draw_focus() const {
    // Change the background color for the entire window
    const int cols = getmaxx(middle_divider);

    int agent_color = WHITE_FOREGROUND;
    int bash_color = WHITE_FOREGROUND;

    if (focus == FOCUS_AGENT) {
        agent_color = MAGENTA_FOREGROUND;
    } else if (focus == FOCUS_BASH) {
        bash_color = MAGENTA_FOREGROUND;
    }

    wattron(middle_divider, COLOR_PAIR(agent_color));
    mvwhline(middle_divider, 0, 0, 0, cols / 2);
    wattroff(middle_divider, COLOR_PAIR(agent_color));

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
        focus = (focus + 1) % static_cast<int>(screens.size());
    }

    draw_focus();
}

void TerminalMultiplexer::create_wins_draw() {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    const int middle_row = (rows - 1) / 2;
    int agent_lines = middle_row;
    const int agent_cols = cols;

    int bash_lines = rows - middle_row - 2;
    const int bash_cols = cols;

    int agent_y = 0, agent_x = 0;
    int bash_y = middle_row + 1, bash_x = 0;

    // Create windows for bottom bar and middle divider
    if (bottom_bar != nullptr) {
        delwin(bottom_bar);
    }

    if (middle_divider != nullptr) {
        delwin(middle_divider);
    }

    bottom_bar = newwin(1, cols, rows - 1, 0);
    middle_divider = newwin(1, cols, middle_row, 0);

    wbkgd(bottom_bar, COLOR_PAIR(WHITE_ON_MAGENTA));
    wprintw(bottom_bar, "ishell");
    wrefresh(bottom_bar);

    if (zoomed_in) {
        if (focus == FOCUS_AGENT) {
            agent_lines = rows - 1;
            bash_y = -1;
            bash_x = -1;
        } else if (focus == FOCUS_BASH) {
            bash_lines = rows - 1;
            bash_y = 0;
            agent_y = -1;
            agent_x = -1;
        }
    }

    // Resize old screens
    std::vector<Screen> new_screens;
    
    // Agent
    Screen screen = Screen(agent_lines, agent_cols, screens[0]);
    screen.set_screen_coords(agent_y, agent_x, agent_y + agent_lines - 1, agent_x + agent_cols - 1);

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
        winsize w{};
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
    const int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    // Add stdin to the epoll instance
    epoll_event event{};
    event.events = EPOLLIN;
    event.data.fd = STDIN_FILENO;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &event) == -1) {
        perror("epoll_ctl: stdin");
        exit(EXIT_FAILURE);
    }

    // Add ptys to epoll instance
    for (const auto & screen : screens) {
        epoll_event event1{};
        event1.events = EPOLLIN;
        event1.data.fd = screen.get_pty_master();
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event1.data.fd, &event1) == -1) {
            perror("epoll_ctl: pty");
            exit(EXIT_FAILURE);
        }
    }

    struct sigaction sa_old{};
    sigset_t mask;

    // Get the existing signal handler for SIGWINCH
    if (sigaction(SIGWINCH, nullptr, &sa_old) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    // Block the SIGWINCH signal
    sigemptyset(&mask);
    sigaddset(&mask, SIGWINCH);
    if (sigprocmask(SIG_BLOCK, &mask, nullptr) == -1) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }

    // Add signal SIGWINCH to catch window resizes
    sigemptyset(&mask);
    sigaddset(&mask, SIGWINCH);

    const int sigfd = signalfd(-1, &mask, 0);
    if (sigfd < 0) {
        perror("signalfd");
        exit(EXIT_FAILURE);
    }

    // Make sigfd non-blocking (a potential spurious wake-up might block the app)
    const int flags = fcntl(sigfd, F_GETFL, 0);
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

    bool epolling = true;

    while (epolling) {
        epoll_event events[MAX_EVENTS];
        const int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

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
                signalfd_siginfo sigfd_info{};

                if (const ssize_t s = read(sigfd, &sigfd_info, sizeof(signalfd_siginfo)); s != sizeof(signalfd_siginfo)) {
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
                        if (const int n_pty = handle_screen_output(screen, events[i].data.fd); n_pty <= 0) {
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

int TerminalMultiplexer::handle_screen_output(Screen &screen, const int fd) const {
    int bytes_read = 0;
    static std::string accumulated_output;

    while (true) {
        std::vector<TerminalChar> chars;
        const int n = read_and_escape(fd, chars);

        if (n < 0) {
            if (errno == EIO) {
                // PTY set EIO (-1) for closure for some reason...
                return -1;
            }

            if (errno == EAGAIN || errno == EWOULDBLOCK) {
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
                accumulated_output += tch.sequence;
                screen.handle_char(tch);

                size_t pos = accumulated_output.find("__ISHELL_EXIT_CODE:");
                if (pos != std::string::npos) {
                    size_t end_pos = accumulated_output.find('\n', pos);
                    if (end_pos != std::string::npos) {
                        int exit_code = std::stoi(
                            accumulated_output.substr(pos + 19, end_pos - (pos + 19))
                        );

                        std::string command_output = accumulated_output.substr(0, pos);
                        SessionTracker::get().finalizeCommand(exit_code, command_output);
                        accumulated_output.erase(0, end_pos + 1);
                    }
                }
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
    const int n = read_and_escape(STDIN_FILENO, chars);

    if (n < 0) {
        perror("read");
        exit(EXIT_FAILURE);
    }

    for (auto const &tch : chars) {
        int const ch = toupper(tch.ch);
        if (ch == 0x02) {
            // Pressed ^B
            waiting_for_command = true;
        } else if (waiting_for_command) {
            waiting_for_command = false;
            if (ch == '\t') {
                switch_focus();
            } else if (ch == 'Z') {
                if (!zoomed_in) {
                    zoom_in();
                } else {
                    zoom_out();
                }
            } else if (ch == '[') {
                toggle_manual_scroll();
            }
        } else if (focus != FOCUS_NULL) {
            if (screens[focus].is_in_manual_scroll()) {
                if (ch == E_KEY_CUU) {
                    screens[focus].manual_scroll_up();
                    refresh_cursor();
                } else if (ch == E_KEY_CUD) {
                    screens[focus].manual_scroll_down();
                    refresh_cursor();
                }
            } else {
                for (const char ch1 : tch.sequence) {
                    handle_pty_input(screens[focus].get_pty_master(), ch1);
                }
            }
        }
    }

    return n;
}

void TerminalMultiplexer::handle_pty_input(const int fd, const char ch) {
    if (ch == '\n') {
        SessionTracker::get().logEvent(SessionTracker::EventType::ShellCommand, current_command);
        current_command.clear();
        const char *exit_code_cmd = "echo \"__ISHELL_EXIT_CODE:$?\"\n";
        write(fd, exit_code_cmd, strlen(exit_code_cmd));
    } else if (ch == 0x7f || ch == 0x08) { // Backspace
        if (!current_command.empty()) {
            current_command.pop_back();
        }
    } else if (isprint(ch)) {
        current_command.push_back(ch);
    }

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