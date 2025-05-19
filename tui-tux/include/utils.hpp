#ifndef ISHELL_UTILS
#define ISHELL_UTILS

#define FOCUS_NULL (-1)
#define FOCUS_AGENT 0
#define FOCUS_BASH 1

#define MAX_EVENTS 5

#define INITIAL_PAD_HEIGHT 100

#define KEY_BEL 0x07
#define KEY_SI 0x0f
#define KEY_BS 0x08

#define LINE_INFO_UNTOUCHED 0
#define LINE_INFO_UNWRAPPED 1
#define LINE_INFO_WRAPPED 2
#include <string>
#include <vector>

std::vector<std::string> split(const std::string &str, char delim, bool ignore_empty);
std::string join(std::vector<std::string> &words, char delim);
bool is_range_token(const std::string& s);

#define DEFAULT_AGENCY_URL "http://142.93.231.126:5000/agents"
#define DEFAULT_ISHELL_LOCAL_DIR "/etc/ishell"

#endif