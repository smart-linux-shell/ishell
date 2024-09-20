#ifndef ISHELL_SCREEN_RING_BUFFER
#define ISHELL_SCREEN_RING_BUFFER

#define MANUAL_SCROLL_NULL -1

#include <vector>

class ScreenRingBuffer {
public:
    ScreenRingBuffer();
    ScreenRingBuffer(int terminal_lines, int terminal_cols, int max_lines);
    ScreenRingBuffer(int terminal_lines, int terminal_cols, int max_lines, ScreenRingBuffer &old_buffer);
    int add_char(int terminal_y, int terminal_x, char ch);
    const char get_char(int terminal_y, int terminal_x);
    void scroll_down();
    void scroll_up();
    int new_line(int terminal_y);
    int clear();
    bool has_new_line(int terminal_y);
    void push_right(int termianl_y, int terminal_x);

    const bool is_in_manual_scroll();
    void enter_manual_scroll();
    void manual_scroll_up();
    void manual_scroll_down();
    void manual_scroll_reset();

private:
    int terminal_lines, terminal_cols, max_lines;
    int start_line;
    int filled_lines;

    // This is the line in the ring buffer where the current terminal view starts
    int terminal_begin_line;

    // This is the line in the ring buffer used as a begin line for getting chars.
    // Normally set as -1, but when in manual scroll mode it will be a positive value.
    int manual_scroll_begin_line;

    struct Line {
        std::vector<char> data;
        bool new_paragraph;   
    };

    std::vector<Line> lines;
    
    void init(int terminal_lines, int terminal_cols, int max_lines);
    void init(int terminal_lines, int terminal_cols, int max_lines, ScreenRingBuffer &old_buffer);
    void expand_down();
    void expand_up();
    bool is_out_of_bounds(int y);
    bool is_out_of_terminal_bounds(int terminal_y, int terminal_x);
    void push_down(int y);
};

#endif