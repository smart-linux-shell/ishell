#ifndef ISHELL_TERMINAL_MULTIPLEXER
#define ISHELL_TERMINAL_MULTIPLEXER

#include <vector>

#include <screen.hpp>
#include <utils.hpp>

class TerminalMultiplexer {
public:
    TerminalMultiplexer();
    ~TerminalMultiplexer();
    void run();

private:
    const char *shell = "/bin/bash";

    int focus = FOCUS_NULL;
    bool waiting_for_command = false;
    bool zoomed_in = false;

    WINDOW *bottom_bar = nullptr, *middle_divider = nullptr;

    std::vector<Screen> screens;
    std::vector<WINDOW *> windows;

    void init();
    void init_nc();
    void refresh_cursor() const;
    void draw_focus() const;
    void switch_focus();
    void create_wins_draw();
    void delete_windows();
    void cleanup();
    void send_dims();
    void resize();
    void run_terminal();
    int handle_screen_output(Screen &screen, int fd);
    int handle_input();

    static void handle_pty_input(int fd, char ch);
    void zoom_in();
    void zoom_out();
    void toggle_manual_scroll();

    /* ────────── OSC 133 state (bash‑экран) ────────── */
    bool bash_capturing = false;
    bool bash_waiting_for_prompt = true;
    std::string bash_output;
    int last_command_line = -10;
    int first_command_line = -10;
};

#endif