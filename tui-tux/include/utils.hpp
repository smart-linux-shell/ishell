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

std::vector<std::string> split(std::string &str, char delim, bool ignore_empty);
std::string join(std::vector<std::string> &words, char delim);
std::pair<std::string, int> extract_command(const std::string& input);
bool is_shell_prompt(const std::string& line);

#define DEFAULT_AGENCY_URL "https://ishell-stage.csai.site/agents"
#define DEFAULT_ISHELL_LOCAL_DIR "/etc/ishell"

#endif