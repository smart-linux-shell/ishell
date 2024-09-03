#ifndef ISHELL_SCREEN
#define ISHELL_SCREEN

#include <ncurses.h>

#include "../include/screen_ring_buffer.hpp"

class Screen {
public:
    Screen();
    Screen(int lines, int cols, WINDOW *window);
    Screen(int lines, int cols, WINDOW *window, Screen &old_screen);
    int get_n_lines();
    int get_n_cols();
    void write_char(char ch);
    int move_cursor(int y, int x);
    int get_x();
    int get_y();
    void cursor_begin();
    void cursor_return();
    void cursor_back();
    void cursor_forward();
    void cursor_up();
    void clear();
    void erase_in_place();
    void erase_to_eol();
    void cursor_down();
    void scroll_down();
    void scroll_up();
    void newline();

private:
    int n_lines, n_cols;
    bool cursor_wrapped = false;

    WINDOW *window;

    ScreenRingBuffer buffer;

    void init(int new_lines, int new_cols, WINDOW *new_window);
    void init(int new_lines, int new_cols, WINDOW *new_window, Screen &old_screen);
    char show_next_char();
    void show_all_chars();

};

#endif