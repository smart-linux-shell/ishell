#include <string>
#include <regex>
#include <unistd.h>
#include <unordered_map>

#include "../include/screen.hpp"
#include "../include/escape.hpp"

#define READ_BUFSIZ 1024

void escape(std::string &seq, Screen &screen) {
    std::regex clear_regex("^\\[J$");
    std::regex home_regex("^\\[H$");
    std::regex cursor_up_regex("^\\[A$");
    std::regex cursor_down_regex("^\\[B$");
    std::regex cursor_forward_regex("^\\[C$");
    std::regex cursor_back_regex("^\\[H$");
    std::regex erase_in_place_regex("^\\[1P$");
    std::regex erase_to_eol_regex("^\\[K$");
    std::regex move_regex("^\\[(\\d+);(\\d+)H$");
    std::regex vertical_regex("^\\[(\\d+)d$");
    
    std::regex back_rel_regex("^\\[(\\d+)D$");
    std::regex front_rel_regex("^\\[(\\d+)C$");
    std::regex up_rel_regex("^\\[(\\d+)A$");
    std::regex down_rel_regex("^\\[(\\d+)B$");

    std::regex scroll_up_regex("^M$");

    std::regex insert_char_regex("^\\[(\\d+)@$");
    std::regex insert_char1_regex("^\\[@$");

    std::smatch matches;

    if (std::regex_match(seq, clear_regex)) {
        screen.clear();
    } else if (std::regex_match(seq, home_regex)) {
        screen.cursor_begin();
    } else if (std::regex_match(seq, cursor_up_regex)) {
        screen.cursor_up();
    } else if (std::regex_match(seq, cursor_down_regex)) {
        screen.cursor_down();
    } else if (std::regex_match(seq, cursor_forward_regex)) {
        screen.cursor_forward();
    } else if (std::regex_match(seq, cursor_back_regex)) {
        screen.cursor_back();
    } else if (std::regex_match(seq, erase_in_place_regex)) {
        screen.erase_in_place();
    } else if (std::regex_match(seq, erase_to_eol_regex)) {
        screen.erase_to_eol();
    } else if (std::regex_match(seq, matches, move_regex)) {
        screen.move_cursor(std::stoi(matches[1].str()) - 1, std::stoi(matches[2].str()) - 1);
    } else if (std::regex_match(seq, matches, vertical_regex)) {
        screen.move_cursor(std::stoi(matches[1].str()) - 1, screen.get_x());
    } else if (std::regex_match(seq, matches, back_rel_regex)) {
        screen.move_cursor(screen.get_y(), screen.get_x() - std::stoi(matches[1].str()));
    } else if (std::regex_match(seq, matches, front_rel_regex)) {
        screen.move_cursor(screen.get_y(), screen.get_x() + std::stoi(matches[1].str()));
    } else if (std::regex_match(seq, matches, up_rel_regex)) {
        screen.move_cursor(screen.get_y() - std::stoi(matches[1].str()), screen.get_x());
    } else if (std::regex_match(seq, matches, down_rel_regex)) {
        screen.move_cursor(screen.get_y() + std::stoi(matches[1].str()), screen.get_x());
    } else if (std::regex_match(seq, scroll_up_regex)) {
        screen.scroll_up();
    } else if (std::regex_match(seq, matches, insert_char_regex)) {
        screen.insert_next(std::stoi(matches[1].str()));
    } else if (std::regex_match(seq, insert_char1_regex)) {
        screen.insert_next(1);
    }
}

TerminalChar escape(std::string &seq) {
    // Check `infocmp linux-m`

    std::vector<std::pair<std::regex, int>> regexes = {
        {std::regex("^\\[J$"), E_KEY_CLEAR},
        {std::regex("^\\[1P$"), E_KEY_DCH},
        {std::regex("^\\[K$"), E_KEY_EL},
        {std::regex("^\\[(\\d*);(\\d+)H$"), E_KEY_CUP},
        {std::regex("^\\[(\\d*)d$"), E_KEY_VPA},
        {std::regex("^\\[(\\d*)D$"), E_KEY_CUB},
        {std::regex("^\\[(\\d*)C$"), E_KEY_CUF},
        {std::regex("^\\[(\\d*)A$"), E_KEY_CUU},
        {std::regex("^\\[(\\d*)B$"), E_KEY_CUD},
        {std::regex("^M$"), E_KEY_RI},
        {std::regex("^\\[(\\d*)@$"), E_KEY_ICH}
    };

    std::smatch matches;

    TerminalChar ret;
    ret.ch = 0;

    for (std::pair<std::regex, int> &pair : regexes) {
        if (std::regex_match(seq, matches, pair.first)) {
            ret.ch = pair.second;
            for (size_t i = 1; i < matches.size(); i++) {
                int arg;

                try {
                    arg = std::stoi(matches[i].str());
                } catch (const std::invalid_argument &e) {
                    continue;
                } catch (const std::out_of_range &e) {
                    continue;
                }

                ret.args.push_back(arg);
            }

            return ret;
        }
    }

    return ret;
}

int read_and_escape(int fd, std::vector<TerminalChar> &vec) {
    struct FdEscapeData {
        bool in_escape;
        std::string escape_seq;
    };

    static std::unordered_map<int, FdEscapeData> fd_escape_data;

    if (fd_escape_data.find(fd) == fd_escape_data.end()) {
        fd_escape_data[fd].in_escape = false;
        fd_escape_data[fd].escape_seq = "";
    }

    char buf[READ_BUFSIZ];

    int n = read(fd, buf, READ_BUFSIZ);
    if (n <= 0) {
        return n;
    }

    vec = std::vector<TerminalChar>();

    for (int i = 0; i < n; i++) {
        // ESC sequence
        if (buf[i] == 0x1B) {
            fd_escape_data[fd].in_escape = true;
            continue;
        }

        if (fd_escape_data[fd].in_escape) {
            if (buf[i] == 0x5B) {
                fd_escape_data[fd].escape_seq += buf[i];
            } else if (buf[i] == 0x9C) {
                TerminalChar escaped_tch = escape(fd_escape_data[fd].escape_seq);

                if (escaped_tch.ch != 0) {
                    vec.push_back(escaped_tch);
                }

                fd_escape_data[fd].in_escape = false;
                fd_escape_data[fd].escape_seq = "";
            } else if ((buf[i] >= 0x40 && buf[i] <= 0x7E) || (buf[i] == 0x9C)) {
                fd_escape_data[fd].escape_seq += buf[i];
                
                TerminalChar escaped_tch = escape(fd_escape_data[fd].escape_seq);

                if (escaped_tch.ch != 0) {
                    vec.push_back(escaped_tch);
                }

                fd_escape_data[fd].in_escape = false;
                fd_escape_data[fd].escape_seq = "";
            } else {
                fd_escape_data[fd].escape_seq += buf[i];
            }
        } else {
            TerminalChar tch;
            tch.ch = buf[i];
            vec.push_back(tch);
        }
    }

    return n;
}