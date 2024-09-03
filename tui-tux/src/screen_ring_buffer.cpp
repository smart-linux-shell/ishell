#include <string>
#include <string.h>

#include "../include/screen_ring_buffer.hpp"

ScreenRingBuffer::ScreenRingBuffer() {}

ScreenRingBuffer::ScreenRingBuffer(int terminal_lines, int terminal_cols, int max_lines) {
    init(terminal_lines, terminal_cols, max_lines);
}

ScreenRingBuffer::ScreenRingBuffer(int terminal_lines, int terminal_cols, int max_lines, ScreenRingBuffer &old_buffer) {
    init(terminal_lines, terminal_cols, max_lines, old_buffer);
}

int ScreenRingBuffer::add_char(int terminal_y, int terminal_x, char ch) {
    int y = (terminal_begin_line + terminal_y) % max_lines;
    int x = terminal_x;

    if (is_out_of_terminal_bounds(terminal_y, terminal_x)) {
        return -1;
    }

    while (is_out_of_bounds(y)) {
        // Need more space.
        expand_down();
    }

    lines[y].data[x] = ch;

    return 0;
}

char ScreenRingBuffer::get_char(int terminal_y, int terminal_x) {
    int y = (terminal_begin_line + terminal_y) % max_lines;
    int x = terminal_x;

    if (is_out_of_terminal_bounds(terminal_y, terminal_x)) {
        return -1;
    }

    if (is_out_of_bounds(y)) {
        // Nothing.
        return 0;
    }

    y = y % max_lines;

    return lines[y].data[x];
}

void ScreenRingBuffer::scroll_down() {
    int y = (terminal_begin_line + 1) % max_lines;

    while (is_out_of_bounds(y)) {
        expand_down();
    }

    terminal_begin_line = y;
}

void ScreenRingBuffer::scroll_up() {
    int y = terminal_begin_line - 1;

    if (y < 0) {
        y = y + max_lines;
    }

    while (is_out_of_bounds(y)) {
        expand_up();
    }

    terminal_begin_line = y;
}

int ScreenRingBuffer::new_line(int terminal_y) {
    int y = terminal_begin_line + terminal_y;

    if (is_out_of_terminal_bounds(terminal_y, 0)) {
        return -1;
    }

    if (is_out_of_bounds(y)) {
        // Do nothing
        return 0;
    }

    y = y % max_lines;

    lines[y].new_paragraph = true;

    return 0;
}

int ScreenRingBuffer::clear() {
    start_line = 0;
    filled_lines = 1;
    terminal_begin_line = 0;

    lines = new Line[max_lines];
    for (int i = 0; i < max_lines; i++) {
        lines[i].data = new char[terminal_cols];
        memset(lines[i].data, 0, terminal_cols);
        lines[i].new_paragraph = false;
    }

    return 0;
}

void ScreenRingBuffer::init(int terminal_lines, int terminal_cols, int max_lines) {
    this->terminal_lines = terminal_lines;
    this->terminal_cols = terminal_cols;
    this->max_lines = max_lines;

    clear();
}

void ScreenRingBuffer::init(int terminal_lines, int terminal_cols, int max_lines, ScreenRingBuffer &old_buffer) {
    init(terminal_lines, terminal_cols, max_lines);

    int y = max_lines - 1;

    std::string line = "";

    bool last_line = true;

    // Write from the last line up.
    for (int i = old_buffer.filled_lines - 1; i >= 0; i--) {
        int old_y = (old_buffer.start_line + i) % old_buffer.max_lines;

        // Reconstruct line

        for (int j = old_buffer.terminal_cols - 1; j >= 0; j--) {
            if (old_buffer.lines[old_y].data[j] != 0) {
                line = old_buffer.lines[old_y].data[j] + line;
            }
        }

        if (i == 0 || old_buffer.lines[old_y - 1].new_paragraph) {
            // Line ends here. Store it and start a new one
            int line_length = line.size();

            int n_wrapped_lines = line_length / terminal_cols;
            int remaining_cols = line_length - n_wrapped_lines * terminal_cols;

            if (remaining_cols > 0) {
                n_wrapped_lines++;
            }

            for (int k = 0; k < n_wrapped_lines; k++) {
                int col_max = terminal_cols - 1;
                if (k == 0 && remaining_cols > 0) {
                    col_max = remaining_cols - 1;
                }

                for (int j = col_max; j >= 0; j--) {
                    lines[y].data[j] = line[line_length - 1];
                    line_length--;
                }

                if (k == 0 && !last_line) {
                    lines[y].new_paragraph = true;
                }

                y--;
                if (y < 0) {
                    break;
                }
            }

            if (y < 0) {
                break;
            }

            line = "";
            last_line = false;
        }
    }

    start_line = y + 1;
    filled_lines = max_lines - start_line;

    terminal_begin_line = std::max(start_line, max_lines - terminal_lines - 1);
}

void ScreenRingBuffer::expand_down() {
    if (filled_lines == max_lines) {
        // Push start down
        memset(lines[start_line].data, 0, terminal_cols);
        lines[start_line].new_paragraph = false;
        start_line = (start_line + 1) % max_lines;
    } else {
        // Increase size
        filled_lines++;
    }
}

void ScreenRingBuffer::expand_up() {
    if (filled_lines == max_lines) {
        // Push start up
        memset(lines[start_line].data, 0, terminal_cols);
        lines[start_line].new_paragraph = false;
        start_line--;
        if (start_line == -1) {
            start_line = max_lines - 1;
        }
    } else {
        // Move start line up and increase size
        start_line--;
        if (start_line == -1) {
            start_line = max_lines - 1;
        }

        filled_lines++;
    }
}

bool ScreenRingBuffer::is_out_of_bounds(int y) {
    if (y >= start_line && y - start_line + 1 > filled_lines) {
        return true;
    }

    if (y < start_line && max_lines - start_line + y + 1 > filled_lines) {
        return true;
    }

    return false;
}

bool ScreenRingBuffer::is_out_of_terminal_bounds(int terminal_y, int terminal_x) {
    if (terminal_y < 0 || terminal_y >= terminal_lines || terminal_x < 0 || terminal_x > terminal_cols) {
        return true;
    }

    return false;
}