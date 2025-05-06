#ifndef ISHELL_ESCAPE
#define ISHELL_ESCAPE

#include <string>
#include <vector>

// Check `infocmp linux-m`

#define E_KEY_CLEAR 256

// Erase in place
#define E_KEY_DCH 257

// Erase to EOL 1 char
#define E_KEY_EL 258

// Move cursor
#define E_KEY_CUP 259

// Move cursor vertically
#define E_KEY_VPA 260

// Cursor back
#define E_KEY_CUB 261

// Cursor front
#define E_KEY_CUF 262

// Cursor up
#define E_KEY_CUU 263

// Cursor down
#define E_KEY_CUD 264

// Scroll up
#define E_KEY_RI 265

// Insert character
#define E_KEY_ICH 266

// ────────── OSC 133 (semantic prompt marks) ──────────
#define E_OSC_PROMPT_START  512   // 333;A
#define E_OSC_PROMPT_END    513   // 333;B
#define E_OSC_PRE_EXEC      514   // 333;C
#define E_OSC_CMD_FINISH    515   // 333;D;<exit>

struct TerminalChar {
    int ch;
    std::vector<int> args;
    std::string sequence;
};

TerminalChar escape(const std::string &seq);
int read_and_escape(int fd, std::vector<TerminalChar> &vec);

#endif