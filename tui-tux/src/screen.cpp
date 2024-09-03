#include <ncurses.h>

#include "../include/screen_ring_buffer.hpp"

#include "../include/screen.hpp"

Screen::Screen() {}

Screen::Screen(int lines, int cols, WINDOW *window) {
    init(lines, cols, window);
}

Screen::Screen(int lines, int cols, WINDOW *window, Screen &old_screen) {
    init(lines, cols, window, old_screen);
}

int Screen::get_n_lines() {
    return n_lines;
}

int Screen::get_n_cols() {
    return n_cols;
}

void Screen::write_char(char ch) {
    int curx = getcurx(window);
    int cury = getcury(window);

    buffer.add_char(cury, curx, ch);

    show_next_char();
}

int Screen::move_cursor(int y, int x) {
    int rc = wmove(window, y, x);

    if (rc == ERR) {
        return rc;
    }

    cursor_wrapped = false;
    wrefresh(window);

    return rc;
}

int Screen::get_x() {
    return getcurx(window);
}

int Screen::get_y() {
    return getcury(window);
}

void Screen::cursor_begin() {
    move_cursor(0, 0);
}

void Screen::cursor_return() {
    move_cursor(getcury(window), 0);
}

void Screen::cursor_back() {
    move_cursor(getcury(window), getcurx(window) - 1);
}

void Screen::cursor_forward() {
    move_cursor(getcury(window), getcurx(window) + 1);
}

void Screen::cursor_up() {
    int rc = move_cursor(getcury(window) - 1, getcurx(window));

    if (rc == ERR) {
        scroll_up();
    }
}

void Screen::clear() {
    buffer.clear();

    wclear(window);
    cursor_wrapped = false;
    wrefresh(window);
}

void Screen::erase_in_place() {
    int curx = getcurx(window);
    int cury = getcury(window);
    
    buffer.add_char(cury, curx, 0);

    wdelch(window);

    wrefresh(window);
}

void Screen::erase_to_eol() {
    int curx = getcurx(window);
    int cury = getcury(window);

    for (int i = curx; i < n_cols; i++) {
        buffer.add_char(cury, i, 0);
    }

    wclrtoeol(window);

    wrefresh(window);
}

void Screen::cursor_down() {
    int rc = move_cursor(getcury(window) + 1, getcurx(window));
    
    if (rc == ERR) {
        scroll_down();
    }
}

void Screen::scroll_down() {
    wclear(window);

    buffer.scroll_down();

    // Redraw
    show_all_chars();

    move_cursor(getmaxy(window) - 1, 0);
    erase_to_eol();
}

void Screen::scroll_up() {
    wclear(window);
    
    buffer.scroll_up();

    // Redraw
    show_all_chars();

    move_cursor(0, 0);
    erase_to_eol();
}

void Screen::newline() {
    int cury = getcury(window);
    
    buffer.new_line(cury);
    cursor_down();

    wrefresh(window);
}

void Screen::init(int new_lines, int new_cols, WINDOW *new_window) {
    n_lines = new_lines;
    n_cols = new_cols;
    window = new_window;
    buffer = ScreenRingBuffer(new_lines, new_cols, 1024);
}

void Screen::init(int new_lines, int new_cols, WINDOW *new_window, Screen &old_screen) {
    init(new_lines, new_cols, new_window);

    buffer = ScreenRingBuffer(new_lines, new_cols, 1024, old_screen.buffer);

    // Draw text
    show_all_chars();
}

char Screen::show_next_char() {
    int curx = getcurx(window);
    int cury = getcury(window);

    char ch = buffer.get_char(cury, curx);

    char orig_ch = ch;

    if (ch == 0) {
        ch = ' ';
    }

    if (curx == getmaxx(window) - 1) {
        if (!cursor_wrapped) {
            // Write and return to last position, set wrappped
            waddch(window, ch);
            wmove(window, cury, curx);
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

    return orig_ch;
}

void Screen::show_all_chars() {
    wclear(window);
    cursor_wrapped = false;
    
    int farthest_x = 0, farthest_y = 0;

    wmove(window, 0, 0);

    for (int i = 0; i < n_lines; i++) { 
        for (int j = 0; j < n_cols; j++) {
            char ch = buffer.get_char(i, j);

            if (ch != 0) {
                farthest_x = j;
                farthest_y = i;
            } else {
                ch = ' ';
            }

            // Write char
            mvwaddch(window, i, j, ch);
        }
    }

    if (farthest_x == getmaxx(window) - 1) {
        // Wrapped
        cursor_wrapped = true;
    } else {
        // Advance
        farthest_x++;
    }

    wmove(window, farthest_y, farthest_x);
    wrefresh(window);
}