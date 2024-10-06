#ifndef ISHELL_SCREEN
#define ISHELL_SCREEN

#include <ncurses.h>
#include <utility>
#include <vector>

#include <escape.hpp>

class Screen {
public:
    Screen();
    Screen(int lines, int cols, int pty_master, int pid);
    Screen(int lines, int cols, Screen &old_screen);
    int get_n_lines() const;
    int get_n_cols() const;
    void handle_char(TerminalChar &tch);
    void write_char(chtype ch);
    void cursor_begin();
    void cursor_return();
    void cursor_back();
    void cursor_forward();
    void cursor_up();
    int move_cursor(int y, int x);
    void clear();
    void erase(int del_cnt);
    void erase_to_eol();
    void cursor_down();
    void scroll_down();
    void scroll_up();
    void newline();
    int get_pty_master() const;
    int get_pid() const;
    int get_pad_height() const;
    WINDOW *get_pad() const;
    void delete_wins();
    void insert_next(int num);
    int translate_given_x(int x);
    int translate_given_y(int y);
    void translate_given_coords(int y, int x, int &new_y, int &new_x);
    void refresh_screen();
    void set_screen_coords(int sminy, int sminx, int smaxy, int smaxx);
    void expand_pad();
    bool is_in_manual_scroll();
    void reset_manual_scroll();
    void enter_manual_scroll();
    void manual_scroll_up();
    void manual_scroll_down();

private:
    int n_lines, n_cols;

    int pty_master;
    int pid;

    int pushing_right = 0;
    bool cursor_wrapped = false;

    // Point where pad displaying starts
    int pad_start = 0;

    // Pad manual scrolling (-1 means disabled)
    int manual_scrolling_start = -1;

    int pad_lines;

    int sminx = -1, sminy = -1;
    int smaxx = -1, smaxy = -1;

    WINDOW *pad;

    // Keeps track of characters placed by the user
    std::vector<std::vector<bool>> user_placed;

    // Keeps track of line information. 
    // - LINE_INFO_UNTOUCHED marks a line that has not been written to yet
    // - LINE_INFO_UNWRAPPED marks a standalone line
    // - LINE_INFO_WRAPPED marks that the line is wrapped with the previous
    std::vector<int> line_info;

    void init(int new_lines, int new_cols, int new_pty_master, int new_pid);
    void init(int new_lines, int new_cols, Screen &old_screen);

public:
    // Wrappers
    int waddch(WINDOW *window, const chtype ch);
    int winsch(WINDOW *window, const chtype ch);
    int wmove(WINDOW *window, int y, int x);
};

#endif