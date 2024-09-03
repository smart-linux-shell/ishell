#ifndef ISHELL_SCREEN_RING_BUFFER
#define ISHELL_SCREEN_RING_BUFFER

class ScreenRingBuffer {
public:
    ScreenRingBuffer();
    ScreenRingBuffer(int terminal_lines, int terminal_cols, int max_lines);
    ScreenRingBuffer(int terminal_lines, int terminal_cols, int max_lines, ScreenRingBuffer &old_buffer);
    int add_char(int terminal_y, int terminal_x, char ch);
    char get_char(int terminal_y, int terminal_x);
    void scroll_down();
    void scroll_up();
    int new_line(int terminal_y);
    int clear();

private:
    int terminal_lines, terminal_cols, max_lines;
    int start_line;
    int filled_lines;

    int terminal_begin_line;

    struct Line {
        char *data;
        bool new_paragraph;   
    };

    Line *lines;
    
    void init(int terminal_lines, int terminal_cols, int max_lines);
    void init(int terminal_lines, int terminal_cols, int max_lines, ScreenRingBuffer &old_buffer);
    void expand_down();
    void expand_up();
    bool is_out_of_bounds(int y);
    bool is_out_of_terminal_bounds(int terminal_y, int terminal_x);
};

#endif