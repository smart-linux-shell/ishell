#include <ncurses.h>
#include <string>
#include <pty.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>

int main() {
    initscr();
    noecho();
    use_default_colors();
    start_color();

    std::string str = "";

    WINDOW *win = newwin(LINES, COLS, 0, 0);
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    //wbkgd(win, COLOR_PAIR(1));

    int slave, master;
    openpty(&master, &slave, NULL, NULL, NULL);

    int pid = fork();
    
    if (pid == 0) {
        close(master);
        dup2(slave, STDOUT_FILENO);
        dup2(slave, STDERR_FILENO);
        dup2(slave, STDIN_FILENO);
        close(slave);
        execl("/bin/bash", "/bin/bash", NULL);
    }

    close(slave);

    int epoll_fd = epoll_create1(0);

    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = EPOLLIN;
    event.data.fd = STDIN_FILENO;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &event);

    memset(&event, 0, sizeof(event));
    event.events = EPOLLIN;
    event.data.fd = master;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, master, &event);

    struct epoll_event events[2];

    bool epolling = true;

    while (epolling) {
        //int ch = getch();
        char buf[4096];
        int n = epoll_wait(epoll_fd, events, 2, -1);

        if (n <= 0) {
            epolling = false;
            break;
        }

        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == STDIN_FILENO) {
                int n1 = read(STDIN_FILENO, buf, 4096);
                write(master, buf, n1);
            } else if (events[i].data.fd == master) {
                int n1 = read(master, buf, 4096);
                if (n1 <= 0) {
                    epolling = false;
                    break;
                }

                for (int i = 0; i < n1; i++) {
                    str += buf[i];
                }

                wclear(win);
                for (char ch1 : str) {
                    waddch(win, ch1);
                }

                wrefresh(win);
            }
        }

        if (!epolling) {
            break;
        }
    }

    endwin();
}
