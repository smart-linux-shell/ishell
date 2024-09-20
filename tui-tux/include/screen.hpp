#ifndef ISHELL_SCREEN
#define ISHELL_SCREEN

#include <ncurses.h>
#include <utility>

#include "screen_ring_buffer.hpp"

class Screen {
public:
    Screen();
    Screen(int lines, int cols, WINDOW *window, WINDOW *outer, int pty_master, int pid);
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
    const WINDOW *get_window();
    const int get_pty_master();
    const int get_pid();
    void delete_wins();
    void insert_next(int num);
    void push_right();

    const bool is_in_manual_scroll();
    void enter_manual_scroll();
    void manual_scroll_up();
    void manual_scroll_down();
    void manual_scroll_reset();

private:
    int n_lines, n_cols;
    bool cursor_wrapped = false;

    int pty_master;
    int pid;

    // When window is NULL, use these
    int fallback_y, fallback_x;

    int pushing_right;

    WINDOW *window;
    WINDOW *outer;

    ScreenRingBuffer buffer;

    void init(int new_lines, int new_cols, WINDOW *new_window, WINDOW *outer, int new_pty_master, int new_pid);
    void init(int new_lines, int new_cols, WINDOW *new_window, WINDOW *outer, Screen &old_screen);
    void show_char(int y, int x);
    std::pair<int, int> show_all_chars();

public:
    // Wrappers
    int srefresh();
    int sgetx();
    int sgety();
    int smove(int y, int x);
    int sclear();
    int mvsaddch(int y, int x, chtype ch);
    int saddch(chtype ch);
    int sdelch();
    int sclrtoeol();
    void scursyncup();
    int sbkgd(chtype ch);
    int sbox_outer(chtype ch1, chtype ch2);
    int srefresh_outer();
};

#endif