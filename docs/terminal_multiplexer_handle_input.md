## How `bash` window is working in `terminal_multiplexer.cpp`

- Method `init` initialize two PTY's for agent and bash windows
- Methods from `refresh_cursor` to `resize` (lines 30-37 in `terminal_multiplexer.hpp`) are technical
- Method `run_terminal` creates an epoll instance and fill it with `STDIN`, screens PTY's and fd for window resize. After this it goes to infinite cycle for waiting this fd's.

### Then `STDIN` was called

- Symbols handles by `handle_input()` method for special symbols like `CTRL + B` and if it is just a letter, it writes this to fd of bash screen
- Then method `handle_screen_output(Screen &screen, const int fd)` starts working, because it handles everything that needs to print on a screen(including bash screen)
- Method `handle_screen_output(Screen &screen, const int fd)` also can be called from epolling cycle by bash command output
- This method reads all symbols from given `fd` and sends it to `screen's` method `handle_char(const TerminalChar &tch)` to show it on a screen