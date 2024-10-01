#include <ncurses.h>
#include <string>

int main() {
    initscr();
    noecho();

    std::string str = "";

    while (1) {
        int ch = getch();
        str += ch;

        

        clear();
        for (char ch : str) {
            addch(ch);
        }

        refresh();
    }

    endwin();
}
