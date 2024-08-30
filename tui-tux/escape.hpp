#ifndef ISHELL_ESCAPE
#define ISHELL_ESCAPE

#include <string>
#include "screen.hpp"
void escape(std::string &seq, Screen &bash_screen);

#endif