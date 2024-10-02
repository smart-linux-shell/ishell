#include <ncurses.h>

int main() {
    initscr();
    noecho();

    WINDOW *win = newwin(LINES, COLS, 1, 0);

    scrollok(win, TRUE);

    while (1) {
        char ch = fgetc(stdin);

        if (ch == 'a') {
            scroll(win);
            wrefresh(win);
        } else {
            waddch(win, ch);
            wrefresh(win);
        }
    }

    endwin();

    return 0;
}