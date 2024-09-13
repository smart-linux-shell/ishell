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
    int y = (terminal_begin_line + terminal_y) % max_lines;

    if (is_out_of_terminal_bounds(terminal_y, 0)) {
        return -1;
    }

    if (is_out_of_bounds(y)) {
        // Do nothing
        return 0;
    }

    y = y % max_lines;

    lines[y].new_paragraph = true;

    int next_line = (y + 1) % max_lines;

    if (is_out_of_bounds(next_line)) {
        // Expand for newline
        expand_down();
    }

    return 0;
}

int ScreenRingBuffer::clear() {
    start_line = 0;
    filled_lines = 0;
    terminal_begin_line = 0;

    lines = new Line[max_lines];
    for (int i = 0; i < max_lines; i++) {
        lines[i].data = new char[terminal_cols];
        memset(lines[i].data, 0, terminal_cols);
        lines[i].new_paragraph = false;
    }

    return 0;
}

bool ScreenRingBuffer::has_new_line(int terminal_y) {
    int y = (terminal_begin_line + terminal_y) % max_lines;

    if (is_out_of_terminal_bounds(terminal_y, 0)) {
        return false;
    }

    if (is_out_of_bounds(y)) {
        return false;
    }

    return lines[y].new_paragraph;
}

void ScreenRingBuffer::push_right(int terminal_y, int terminal_x) {
    if (is_out_of_terminal_bounds(terminal_y, terminal_x)) {
        return;
    }

    int y = (terminal_begin_line + terminal_y) % max_lines;
    int x = terminal_x;

    int target_x = x;

    bool looping = true;

    char begin_char = 0;

    while (looping) {
        // Start from the end of the line
        x = terminal_cols - 1;

        char new_begin_char = lines[y].data[x];

        while (x > target_x) {
            // Push right. If empty spaces are found on the line (that can be used to fill), stop here.
            if (lines[y].data[x] == 0) {
                looping = false;
            }

            lines[y].data[x] = lines[y].data[x - 1];

            x--;
        }

        if (begin_char != 0) {
            lines[y].data[target_x] = begin_char;
        }

        begin_char = new_begin_char;
        target_x = 0;

        int next_y = (y + 1) % max_lines;
        
        if (looping) {
            // Edge case: if no empty space on this line, but it is the last line in a paragraph.
            // Create a new line for the remaining chracter, and push all lines down.
            if (lines[y].new_paragraph) {
                push_down(next_y);
                lines[next_y].data[0] = begin_char;
                lines[next_y].new_paragraph = true;
                looping = false;
            } else if (is_out_of_bounds(next_y)) {
                expand_down();
                lines[next_y].data[0] = begin_char;
                looping = false;
            }
        }

        y = (y + 1) % max_lines;
    }

    // Scrolling needed?
    if (!is_out_of_bounds((terminal_begin_line + terminal_lines) % max_lines)) {
        scroll_down();
    }
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
    bool wrote_data = false;

    std::string line = "";

    bool last_line = true;

    // Resizing old buffer to current buffer

    // Write from the last line up.
    for (int i = old_buffer.filled_lines - 1; i >= 0; i--) {
        int old_y = (old_buffer.start_line + i) % old_buffer.max_lines;
        int prev_old_y = old_y - 1;
        if (prev_old_y < 0) {
            prev_old_y += old_buffer.max_lines;
        }

        // Reconstruct line

        for (int j = old_buffer.terminal_cols - 1; j >= 0; j--) {
            if (old_buffer.lines[old_y].data[j] != 0) {
                line = old_buffer.lines[old_y].data[j] + line;
            }
        }

        if (i == 0 || old_buffer.lines[prev_old_y].new_paragraph) {
            // Line ends here. Store it and start a new one
            int line_length = line.size();

            int n_wrapped_lines = line_length / terminal_cols;
            int remaining_cols = line_length - n_wrapped_lines * terminal_cols;

            if (remaining_cols > 0) {
                n_wrapped_lines++;
            }

            if (line_length == 0) {
                // Empty line
                y--;
                wrote_data = true;
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

                if (y < 0) {
                    break;
                }

                y--;
                wrote_data = true;
            }

            if (y < 0) {
                break;
            }

            line = "";
            last_line = false;
        }
    }

    if (wrote_data) {
        // Need to offset the last decrementation of y; only if reached.
        y++;
    }

    start_line = y;
    filled_lines = max_lines - start_line;

    terminal_begin_line = std::max(start_line, max_lines - terminal_lines);
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

// Push down all lines starting from y.
void ScreenRingBuffer::push_down(int y) {
    expand_down();

    int lines_filled_until_y = y - start_line;
    if (lines_filled_until_y < 0) {
        lines_filled_until_y += max_lines;
    }

    int remaining_lines = filled_lines - lines_filled_until_y;

    for (int i = remaining_lines - 1; i >= 1; i--) {
        // From the end to y
        int current_y = (y + i) % max_lines;
        int prev_y = (y + i - 1) % max_lines;

        lines[current_y] = lines[prev_y];
    }

    lines[y].new_paragraph = false;
    lines[y].data = new char[terminal_cols];
}
