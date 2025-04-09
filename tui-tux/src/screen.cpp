#include <ncurses.h>
#include <cstring>

#include <screen.hpp>
#include <utils.hpp>

Screen::Screen() = default;

Screen::Screen(const int lines, const int cols, const int pty_master, const int pid) {
    init(lines, cols, pty_master, pid);
}

Screen::Screen(const int lines, const int cols, Screen &old_screen) {
    init(lines, cols, old_screen);
}

int Screen::get_n_lines() const {
    return n_lines;
}

int Screen::get_n_cols() const {
    return n_cols;
}

void Screen::handle_char(const TerminalChar &tch) {
    // CR
    if (tch.ch == '\r') {
        cursor_return();
    } else if (tch.ch == '\n') {
        newline();
    } else if (tch.ch == KEY_BEL || tch.ch == KEY_SI) {
        // ignore BEL and alt charset
    } else if (tch.ch == KEY_BS) {
        // BKSP
        cursor_back();
    } else if (tch.ch > 255) {
        if (tch.ch == E_KEY_CLEAR) {
            clear();
        } else if (tch.ch == E_KEY_DCH) {
            int del_cnt = 1;
            if (tch.args.size() == 1) {
                del_cnt = tch.args[0];
            }
            erase(del_cnt);
        } else if (tch.ch == E_KEY_EL) {
            erase_to_eol();
        } else if (tch.ch == E_KEY_CUP) {
            int y = 1;
            int x = 1;
            if (tch.args.size() == 2) {
                y = tch.args[0];
                x = tch.args[1];
            }

            int new_y, new_x;
            translate_given_coords(y, x, new_y, new_x);
            move_cursor(new_y, new_x);
        } else if (tch.ch == E_KEY_VPA) {
            int y = 1;
            if (tch.args.size() == 1) {
                y = tch.args[0];
            }

            move_cursor(translate_given_y(y), getcurx(pad));
        } else if (tch.ch == E_KEY_CUB) {
            int x_offs = 1;
            if (tch.args.size() == 1) {
                x_offs = tch.args[0];
            }

            move_cursor(getcury(pad), getcurx(pad) - x_offs);
        } else if (tch.ch == E_KEY_CUF) {
            int x_offs = 1;
            if (tch.args.size() == 1) {
                x_offs = tch.args[0];
            }

            move_cursor(getcury(pad), getcurx(pad) + x_offs);
        } else if (tch.ch == E_KEY_CUU) {
            int y_offs = 1;
            if (tch.args.size() == 1) {
                y_offs = tch.args[0];
            }

            move_cursor(getcury(pad) - y_offs, getcurx(pad));
        } else if (tch.ch == E_KEY_CUD) {
            int y_offs = 1;
            if (tch.args.size() == 1) {
                y_offs = tch.args[0];
            }

            move_cursor(getcury(pad) + y_offs, getcurx(pad));
        } else if (tch.ch == E_KEY_RI) {
            scroll_up();
        } else if (tch.ch == E_KEY_ICH) {
            int num = 1;
            if (tch.args.size() == 1) {
                num = tch.args[0];
            }
            insert_next(num);
        }
    } else if (tch.ch > 0 && tch.ch < 256) {
        write_char(tch.ch);
    }
}

void Screen::write_char(const chtype ch) {
    bool inserting = false;

    int y = getcury(pad);
    int x = getcurx(pad);

    if (pushing_right > 0) {
        pushing_right--;
        inserting = true;
    }

    if (inserting) {
        winsch(pad, ch);

        // Set user placed true at the end of the chain
        for (int i = x; i < n_cols; i++) {
            if (!user_placed[y][i]) {
                user_placed[y][i] = true;
                break;
            }
        }

    }

    if (cursor_wrapped) {
        // Do not overwrite
        chtype orig_ch = mvwinch(pad, y, x);
        waddch(pad, orig_ch);
    }

    y = getcury(pad);
    x = getcurx(pad);
    user_placed[y][x] = true;
    waddch(pad, ch);

    if (!cursor_wrapped && x == n_cols - 1) {
        move_cursor(y, x);
        cursor_wrapped = true;
    } else if (cursor_wrapped) {
        cursor_wrapped = false;
        if (x == 0) {
            // Mark as wrapped
            line_info[y] = LINE_INFO_WRAPPED;
        }
    }

    while (getcury(pad) > pad_start + n_lines - 1) {
        scroll_down();
    }
}

void Screen::cursor_begin() {
    move_cursor(0, 0);
}

void Screen::cursor_return() {
    move_cursor(getcury(pad), 0);
}

void Screen::cursor_back() {
    move_cursor(getcury(pad), getcurx(pad) - 1);
}

void Screen::cursor_forward() {
    move_cursor(getcury(pad), getcurx(pad) + 1);
}

void Screen::cursor_up() {
    move_cursor(getcury(pad) - 1, getcurx(pad));

    while (getcury(pad) < pad_start) {
        scroll_up();
    }
}

int Screen::move_cursor(int y, int x) {
    const int rc = wmove(pad, y, x);
    cursor_wrapped = false;

    return rc;
}

void Screen::clear() {
    user_placed = std::vector<std::vector<bool>>(pad_lines, std::vector<bool>(n_cols, false));
    line_info = std::vector<int>(pad_lines, LINE_INFO_UNTOUCHED);

    wclear(pad);
    pad_start = 0;
}

void Screen::erase(const int del_cnt) {
    for (int i = 0; i < del_cnt; i++) {
        wdelch(pad);

        // Set user placed false at the end of the chain
        const int y = getcury(pad);
        const int x = getcurx(pad);
        bool found = false;

        for (int j = x; j < n_cols - 1 && !found; j++) {
            if (!user_placed[y][j + 1]) {
                found = true;
                user_placed[y][j] = false;
            } 
        }

        if (!found) {
            user_placed[y][n_cols - 1] = false;
        }
    }
}

void Screen::erase_to_eol() {
    wclrtoeol(pad);

    // Set all user placed false
    int y = getcury(pad);
    int x = getcurx(pad);
    for (int i = x; i < n_cols; i++) {
        user_placed[y][i] = false;
    }
}

void Screen::cursor_down() {
    move_cursor(getcury(pad) + 1, getcurx(pad));
    
    while (getcury(pad) > pad_start + n_lines - 1) {
        scroll_down();
    }
}

// Translates coords passed in escape coords to pad coords.
int Screen::translate_given_x(const int x) const {
    if (x < 1) {
        return 0;
    }

    if (x > n_cols) {
        return (n_cols - 1);
    }

    return (x - 1);
}

int Screen::translate_given_y(const int y) const {
    if (y < 1) {
        return pad_start;
    }

    if (y > n_lines) {
        return pad_start + (n_lines - 1);
    }

    return pad_start + y - 1;
}

void Screen::translate_given_coords(int y, int x, int &new_y, int &new_x) const {
    new_x = translate_given_x(x);
    new_y = translate_given_y(y);
}

void Screen::scroll_up() {
    if (pad_start > 0) {
        pad_start--;
        const int y = getcury(pad);
        const int x = getcurx(pad);

        wmove(pad, pad_start, 0);
        erase_to_eol();
        wmove(pad, y, x);
    }
}

void Screen::scroll_down() {
    if (pad_start + n_lines < pad_lines) {
        pad_start++;
    }
}

void Screen::newline() {
    if (const int y = getcury(pad); line_info[y] == LINE_INFO_UNTOUCHED) {
        line_info[y] = LINE_INFO_UNWRAPPED;
    }

    cursor_return();
    cursor_down();
}

WINDOW *Screen::get_pad() const {
    return pad;
}

int Screen::get_pty_master() const {
    return pty_master;
}

int Screen::get_pid() const {
    return pid;
}

int Screen::get_pad_height() const {
    return pad_lines;
}

void Screen::delete_wins() const {
    delwin(pad);
}

// Next num characters will be inserted.
void Screen::insert_next(int num) {
    pushing_right = num;
}

void Screen::refresh_screen() const {
    int start = pad_start;

    if (manual_scrolling_start != -1) {
        start = manual_scrolling_start;
    }

    if (sminy != -1 && sminx != -1 && smaxy != -1 && smaxx != -1) {
        prefresh(pad, start, 0, sminy, sminx, smaxy, smaxx);
    }
}

void Screen::set_screen_coords(int sminy, int sminx, int smaxy, int smaxx) {
    this->sminy = sminy;
    this->sminx = sminx;
    this->smaxy = smaxy;
    this->smaxx = smaxx;
}

void Screen::expand_pad() {
    // Resize
    pad_lines += INITIAL_PAD_HEIGHT;
    wresize(pad, pad_lines, n_cols);
    std::vector<std::vector<bool>> user_placed_ext = std::vector<std::vector<bool>>(INITIAL_PAD_HEIGHT, std::vector<bool>(n_cols, false));
    std::vector<bool> line_info_ext = std::vector<bool>(INITIAL_PAD_HEIGHT, false);

    user_placed.insert(user_placed.end(), user_placed_ext.begin(), user_placed_ext.end());
    line_info.insert(line_info.end(), line_info_ext.begin(), line_info_ext.end());
}

void Screen::init(int new_lines, int new_cols, int new_pty_master, int new_pid) {
    n_lines = new_lines;
    n_cols = new_cols;
    pty_master = new_pty_master;
    pid = new_pid;
    pushing_right = 0;
    manual_scrolling_start = -1;

    pad_lines = INITIAL_PAD_HEIGHT;
    pad = newpad(pad_lines, n_cols);

    user_placed = std::vector<std::vector<bool>>(pad_lines, std::vector<bool>(new_cols, false));
    line_info = std::vector<int>(pad_lines, LINE_INFO_UNTOUCHED);
}

void Screen::init(int new_lines, int new_cols, Screen &old_screen) {
    init(new_lines, new_cols, old_screen.pty_master, old_screen.pid);

    bool first = true;

    int old_y = getcury(old_screen.pad);
    int old_x = getcurx(old_screen.pad);

    int new_y = -1;
    int new_x = -1;

    // Transfer old data
    std::vector<chtype> current;

    for (int i = 0; i < old_screen.pad_lines; i++) {
        // Skip untouched lines
        if (old_screen.line_info[i] == LINE_INFO_UNTOUCHED) {
            continue;
        }

        // Not wrapped to previous line
        if (old_screen.line_info[i] == LINE_INFO_UNWRAPPED) {
            if (first) {
                first = false;
            } else {
                current.clear();
                newline();
            }
        }

        for (int j = 0; j < old_screen.n_cols; j++) {
            if (i == old_y && j == old_x) {
                // Translate cursor position
                new_y = getcury(pad);
                new_x = getcurx(pad);
            }

            chtype ch = mvwinch(old_screen.pad, i, j);
            current.push_back(ch);
            if (old_screen.user_placed[i][j]) {
                // Write everything found so far and reset
                for (chtype ch1 : current) {
                    write_char(ch1);
                }

                current.clear();
            }
        }
    }

    // Reset cursor
    if (new_y != -1 && new_x != -1) {
        wmove(pad, new_y, new_x);
    }
}

bool Screen::is_in_manual_scroll() const {
    return (manual_scrolling_start != -1);
}

void Screen::reset_manual_scroll() {
    manual_scrolling_start = -1;
}

void Screen::enter_manual_scroll() {
    manual_scrolling_start = pad_start;
}

void Screen::manual_scroll_up() {
    if (is_in_manual_scroll() && manual_scrolling_start > 0) {
        manual_scrolling_start--;
    }
}

void Screen::manual_scroll_down() {
    if (is_in_manual_scroll() && manual_scrolling_start < pad_start) {
        manual_scrolling_start++;
    }
}

// Wrappers
int Screen::waddch(WINDOW *window, const chtype ch) {
    // Mark line as touched
    int y = getcury(pad);
    if (line_info[y] == LINE_INFO_UNTOUCHED) {
        line_info[y] = LINE_INFO_UNWRAPPED;
    }

    // Expand if out of bounds
    int rc;

    while (true) {
        rc = ::waddch(window, ch);
        if (rc != ERR) {
            break;
        }
        expand_pad();
    }

    return rc;
}

int Screen::winsch(WINDOW *window, const chtype ch) {
    // Mark line as touched
    int y = getcury(pad);
    if (line_info[y] == LINE_INFO_UNTOUCHED) {
        line_info[y] = LINE_INFO_UNWRAPPED;
    }

    // Expand if out of bounds
    int rc;

    while (true) {
        rc = ::winsch(window, ch);
        if (rc != ERR) {
            break;
        }
        expand_pad();
    }

    return rc;
}

int Screen::wmove(WINDOW *window, int y, int x) {
    // Expand if out of bounds
    int rc;

    while (true) {
        rc = ::wmove(window, y, x);
        if (rc != ERR) {
            break;
        }
        expand_pad();
    }

    return rc;
}

std::string Screen::get_visible_line() {
    char buffer[512];
    memset(buffer, 0, sizeof(buffer));

    int cur_y, cur_x;
    getyx(pad, cur_y, cur_x);

    mvwinnstr(pad, cur_y, 0, buffer, sizeof(buffer) - 1);

    wmove(pad, cur_y, cur_x);

    return std::string(buffer);
}