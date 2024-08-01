# Investigation of TUI/TUX for Linux

Several libraries were evaluated to find the most suitable framework for developing TUX, focusing on lightweight architecture and design, minimal dependencies.

## Evaluation of TUI/TUX frameworks and libraries

### Evaluation criteria

* **Lightweight** - minimal footprint, suitable for all devices and various hardware using Linux OS
* **Support for adding frame layouts** - similar to _tmux_ or _tile managers_, the library should allow flexible frame layouts
* **Minimal dependencies** - for easier installation and fewer compatibility issues

### Considered libraries

After deep research of the projects and libraries from the [list](https://github.com/rothgar/awesome-tuis), these libraries were considered for further investigation, because of their **lightweight**, **flexibility** and **minimal dependencies**:


| _feature_             | [urwid](https://github.com/urwid/urwid)   | [gocui](https://github.com/jroimartin/gocui) | [bubbletea](https://github.com/charmbracelet/bubbletea)  | [py_cui](https://github.com/jwlodek/py_cui) | [FTXUI](https://github.com/ArthurSonzogni/FTXUI) | [ncurses](https://invisible-island.net/ncurses/announce.html)  |
|-----------------------|-------------------------------------------|----------------------------------------------|----------------------------------------------------------|---------------------------------------------|--------------------------------------------------|----------------------------------------------------------------|
| **widget support**    | 🟢 rich                                    | basic                                        | 🟢 rich                                                   | 🟢 rich                                     | moderate                                         | basic                                                          |
| **layout management** | 🟢 easy                                    | moderate                                     | 🟢 easy                                                   | 🟢 easy                                     | 🟢 easy                                          | hard                                                           |
| **mouse support**     | 🟢 yes                                     | no                                           | 🟢 yes                                                    | 🟢 yes                                      | 🟢 yes                                           | 🟢 yes                                                        |
| **documentation**     | 🟢 extensive                               | moderate                                     | moderate                                                 | moderate                                    | moderate                                         | 🟢 extensive                                                   |
| **ease of use**       | 🟢 high                                    | 🟢 high                                      | 🟢 high                                                   | 🟢 high                                     | 🟢 high                                          | moderate                                                       |
| **extensibility**     | 🟢 high                                    | moderate                                     | 🟢 high                                                   | 🟢 high                                     | 🟢 high                                          | moderate                                                       |

**urwid** was chosen as it is a comprehensive Python library for creating console user interfaces, offering support for complex layouts and widgets with minimal dependencies, making it suitable for lightweight applications on any hardware. Also, it has an active community and extensive documentation.

## Prototype

![demo_tui-tux](https://github.com/user-attachments/assets/18bd7d8c-102a-49bd-8f96-2584c15a95fc)

### Features

* **Multi-frame layout** - support for multiple frames (one for dialog/assistant, others for bash) within the same terminal window.
* **Frame switching** - switching between different frames using _Ctrl+Tab_ and _Ctrl+Shift+Tab_, adding new frames using _Ctrl+T_ and _Ctrl+X_ to remove frame in focus.
* **Command execution** - executing commands in bash frame and displaying results.
* **Theming** - support for customizable themes and focus management.
* **Session** - all commands within one frame are saved during the session.

### To-Do

1. [ ] Error detection and error handling
2. [ ] Keyboard shortcuts
3. [ ] Mouse support
4. [ ] Text completion
5. [ ] Syntax highlighting

### Source Code

Source code for the prototype can be found [here](https://github.com/smart-linux-shell/ishell/blob/2_tui-tux/tui-tux/main.py).
