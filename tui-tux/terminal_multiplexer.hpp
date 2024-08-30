#ifndef ISHELL_TERMINAL_MULTIPLEXER
#define ISHELL_TERMINAL_MULTIPLEXER

#include "screen.hpp"
#include "utils.hpp"

class TerminalMultiplexer {
public:
    TerminalMultiplexer();
    ~TerminalMultiplexer();
    void run();

private:
    WINDOW *assistant_win, *bash_win, *outer_assistant_win, *outer_bash_win;
    const char *shell = "/bin/bash";

    int pty_bash_master, pty_bash_slave;
    int pty_assistant_master, pty_assistant_slave;
    int focus = FOCUS_NONE;
    
    Screen bash_screen, assistant_screen;
    void init();
    void init_nc();
    void refresh_cursor();
    void draw_focus();
    void switch_focus();
    void create_wins_draw(Screen *old_bash_screen, Screen *old_assistant_screen);
    void delete_windows();
    void cleanup();
    void send_dims();
    void resize();
    void run_terminal();
    int handle_screen_output(Screen &screen, int fd);
    int handle_input();
    void handle_pty_input(int fd, char ch);
};

#endif