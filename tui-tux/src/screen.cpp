#include <ncurses.h>

#include "../include/screen_ring_buffer.hpp"

#include "../include/screen.hpp"

Screen::Screen() {}

Screen::Screen(int lines, int cols, WINDOW *window, WINDOW *outer, int pty_master, int pid) {
    init(lines, cols, window, outer, pty_master, pid);
}

Screen::Screen(int lines, int cols, WINDOW *window, WINDOW *outer, Screen &old_screen) {
    init(lines, cols, window, outer, old_screen);
}

int Screen::get_n_lines() {
    return n_lines;
}

int Screen::get_n_cols() {
    return n_cols;
}

void Screen::write_char(char ch) {
    int curx = sgetx();
    int cury = sgety();

    if (!cursor_wrapped) {
        buffer.add_char(cury, curx, ch);
        if (curx == n_cols - 1) {
            cursor_wrapped = true;
        }
    } else {
        curx = 0;
        cury++;
        if (cury == n_lines) {
            cury--;
            scroll_down();
        }
        buffer.add_char(cury, curx, ch);
        cursor_wrapped = false;
    }

    show_char(cury, curx);
    smove(cury, curx + 1);
}

int Screen::move_cursor(int y, int x) {
    int rc = smove(y, x);

    if (rc == ERR) {
        return rc;
    }

    cursor_wrapped = false;
    srefresh();

    return rc;
}

int Screen::get_x() {
    return sgetx();
}

int Screen::get_y() {
    return sgety();
}

void Screen::cursor_begin() {
    move_cursor(0, 0);
}

void Screen::cursor_return() {
    move_cursor(get_y(), 0);
}

void Screen::cursor_back() {
    move_cursor(get_y(), get_x() - 1);
}

void Screen::cursor_forward() {
    move_cursor(get_y(), get_x() + 1);
}

void Screen::cursor_up() {
    int rc = move_cursor(get_y() - 1, get_x());

    if (rc == ERR) {
        scroll_up();
    }
}

void Screen::clear() {
    buffer.clear();

    sclear();
    cursor_wrapped = false;
    srefresh();
}

void Screen::erase_in_place() {
    int curx = get_x();
    int cury = get_y();
    
    buffer.add_char(cury, curx, 0);

    sdelch();

    srefresh();
}

void Screen::erase_to_eol() {
    int curx = get_x();
    int cury = get_y();

    for (int i = curx; i < n_cols; i++) {
        buffer.add_char(cury, i, 0);
    }

    sclrtoeol();

    srefresh();
}

void Screen::cursor_down() {
    int rc = move_cursor(sgety() + 1, sgetx());
    
    if (rc == ERR) {
        scroll_down();
    }
}

void Screen::scroll_down() {
    // Preserve
    int cury = sgety();
    int curx = sgetx();
    sclear();
    smove(cury, curx);

    buffer.scroll_down();

    // Redraw
    show_all_chars();

    erase_to_eol();
}

void Screen::scroll_up() {
    int cury = sgety();
    int curx = sgetx();
    sclear();
    smove(cury, curx);
    
    buffer.scroll_up();

    // Redraw
    show_all_chars();

    erase_to_eol();
}

void Screen::newline() {
    int cury = sgety();
    
    buffer.new_line(cury);
    cursor_down();

    srefresh();
}

WINDOW *Screen::get_window() {
    return window;
}

int Screen::get_pty_master() {
    return pty_master;
}

int Screen::get_pid() {
    return pid;
}

void Screen::delete_wins() {
    if (window != NULL) {
        delwin(window);
    }

    if (outer != NULL) {
        delwin(outer);
    }
}

void Screen::init(int new_lines, int new_cols, WINDOW *new_window, WINDOW *new_outer, int new_pty_master, int new_pid) {
    n_lines = new_lines;
    n_cols = new_cols;
    window = new_window;
    outer = new_outer;
    buffer = ScreenRingBuffer(new_lines, new_cols, 1024);
    pty_master = new_pty_master;
    pid = new_pid;

    if (window == NULL) {
        fallback_x = 0;
        fallback_y = 0;
    }
}

void Screen::init(int new_lines, int new_cols, WINDOW *new_window, WINDOW *new_outer, Screen &old_screen) {
    init(new_lines, new_cols, new_window, new_outer, old_screen.pty_master, old_screen.pid);

    buffer = ScreenRingBuffer(new_lines, new_cols, 1024, old_screen.buffer);

    // Draw text
    Pair pair = show_all_chars();
    smove(pair.y, pair.x);
}

void Screen::show_char(int y, int x) {
    // Preserve
    int curx = sgetx();
    int cury = sgety();

    char ch = buffer.get_char(y, x);

    if (ch == 0) {
        ch = ' ';
    }

    mvsaddch(y, x, ch);
    smove(cury, curx);
    srefresh();
}

Pair Screen::show_all_chars() {
    // Preserve
    int curx = sgetx();
    int cury = sgety();

    Pair pair;
    pair.x = -1;
    pair.y = -1;

    bool final_newline = false;

    sclear();

    for (int i = 0; i < n_lines; i++) {
        for (int j = 0; j < n_cols; j++) {
            char ch = buffer.get_char(i, j);
            if (ch == 0) {
                ch = ' ';
            } else {
                pair.x = j;
                pair.y = i;
            }

            mvsaddch(i, j, ch);
        }

        if (pair.y == i && buffer.has_new_line(i)) {
            final_newline = true;
        } else {
            final_newline = false;
        }
    }

    if (pair.x == -1 && pair.y == -1) {
        pair.x = 0;
        pair.y = 0;
    } else if (!final_newline) {
        pair.x++;
    } else {
        pair.x = 0;
        pair.y++;
    }

    smove(cury, curx);
    srefresh();

    return pair;
}

// Wrappers
int Screen::srefresh() {
    if (window != NULL) {
        return wrefresh(window);
    }

    return OK;
}

int Screen::sgetx() {
    if (window != NULL) {
        return getcurx(window);
    }

    return fallback_x;
}

int Screen::sgety() {
    if (window != NULL) {
        return getcury(window);
    }

    return fallback_y;
}

int Screen::smove(int y, int x) {
    if (window != NULL) {
        return wmove(window, y, x);
    }

    if (x < 0 || x >= n_cols || y < 0 || y >= n_lines) {
        return ERR;
    }

    fallback_y = y;
    fallback_x = x;

    return OK;
}

int Screen::sclear() {
    if (window != NULL) {
        return wclear(window);
    }

    fallback_y = 0;
    fallback_x = 0;

    return OK;
}

int Screen::mvsaddch(int y, int x, chtype ch) {
    if (window != NULL) {
        return mvwaddch(window, y, x, ch);
    }

    if (x < 0 || x >= n_cols || y < 0 || y >= n_lines) {
        return ERR;
    }

    fallback_x = x;
    fallback_y = y;

    if (fallback_x < n_cols - 1 || fallback_y < n_lines - 1) {
        fallback_x++;

        if (fallback_x == n_cols) {
            fallback_x = 0;
            fallback_y++;
        }
    } else {
        return ERR;
    }

    return OK;
}

int Screen::saddch(chtype ch) {
    if (window != NULL) {
        return waddch(window, ch);
    }

    if (fallback_x < n_cols - 1 || fallback_y < n_lines - 1) {
        fallback_x++;

        if (fallback_x == n_cols) {
            fallback_x = 0;
            fallback_y++;
        }
    } else {
        return ERR;
    }

    return OK;
}

int Screen::sdelch() {
    if (window != NULL) {
        return wdelch(window);
    }

    fallback_x--;
    if (fallback_x < 0) {
        fallback_x = n_cols - 1;
        fallback_y --;
    }

    if (fallback_y < 0) {
        fallback_x = 0;
        fallback_y = 0;
    }

    return OK;
}

int Screen::sclrtoeol() {
    if (window != NULL) {
        return wclrtoeol(window);
    }

    return OK;
}

void Screen::scursyncup() {
    if (window != NULL) {
        wcursyncup(window);
    }
}

int Screen::sbkgd(chtype ch) {
    if (window != NULL) {
        return wbkgd(window, ch);
    }

    return OK;
}

int Screen::sbox_outer(chtype ch1, chtype ch2) {
    if (outer != NULL) {
        return box(outer, ch1, ch2);
    }

    return OK;
}

int Screen::srefresh_outer() {
    if (outer != NULL) {
        return wrefresh(outer);
    }

    return OK;
}
