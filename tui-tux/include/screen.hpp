#ifndef ISHELL_SCREEN
#define ISHELL_SCREEN

#include <ncurses.h>

#include "screen_ring_buffer.hpp"

class Screen {
public:
    Screen();
    Screen(int lines, int cols, WINDOW *window, WINDOW *outer, int pty_master);
    Screen(int lines, int cols, WINDOW *window, WINDOW *outer, Screen &old_screen);
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
    WINDOW *get_window();
    int get_pty_master();
    void delete_wins();

private:
    int n_lines, n_cols;
    bool cursor_wrapped = false;
    int pty_master;

    WINDOW *window;
    WINDOW *outer;

    ScreenRingBuffer buffer;

    void init(int new_lines, int new_cols, WINDOW *new_window, WINDOW *outer, int pty_master);
    void init(int new_lines, int new_cols, WINDOW *new_window, WINDOW *outer, Screen &old_screen);
    char show_next_char();
    void show_all_chars();

};

#endif