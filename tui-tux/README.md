# Prototype of Terminal User Interface for Linux Shell

This directory contains a simple prototype of terminal multiplexer implemented in C++ using the `ncurses` library. The multiplexer, for now, provides two separate terminal windows within a single interface: one for an **assistant** application and another for a **bash** shell. 

### Overview

The terminal multiplexer allows you to run and interact with two terminal sessions simultaneously. The interface consists of two stacked windows, with each window capable of displaying output and receiving input independently. Users can switch focus between the two windows. Terminal resizing is manually handled to maintain the layout.

**Assistant terminal** is made for communication with assistant agency that provides useful information on typed question, using LLM.

**Bash terminal** is a pseudo terminal that supports running commands and interactive applications, just like a regular terminal.

### Demo 

![demo](https://github.com/user-attachments/assets/f69ac166-447c-4c0f-bece-027fee7c84ba)

### Shortcuts

- **Focus Switching**: Easily switch focus between the two terminals using the `Ctrl + Q` shortcut.
- **Exit**: Close the terminal multiplexer by entering exit in the focused terminal or by closing the window.

### Dependencies and Installation

The implementation relies on `ncurses` and standard C++ libraries, ensuring it remains lightweight and portable. 

Terminal can be run using:
```bash
./run.sh # run.sh is in the root directory "ishell"
```

### Future Work

Following enhancements would expand the capabilities of interface:
- dynamically adding/removing terminal windows (multiplexing),
- zooming in/out (maximizing one focused window and minimizing all the others),
- styling.



