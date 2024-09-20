#include <string>
#include <regex>
#include <unistd.h>
#include <unordered_map>

#include "../include/screen.hpp"
#include "../include/escape.hpp"

#define READ_BUFSIZ 1024

TerminalChar escape(std::string &seq) {
    // Check `infocmp linux-m`

    std::vector<std::pair<std::regex, int>> regexes = {
        {std::regex("^\x1b\\[J$"), E_KEY_CLEAR},
        {std::regex("^\x1b\\[(\\d*)P$"), E_KEY_DCH},
        {std::regex("^\x1b\\[K$"), E_KEY_EL},
        {std::regex("^\x1b\\[(H|\\d+;\\d+H)$"), E_KEY_CUP},
        {std::regex("^\x1b\\[(\\d*)d$"), E_KEY_VPA},
        {std::regex("^\x1b\\[(\\d*)D$"), E_KEY_CUB},
        {std::regex("^\x1b\\[(\\d*)C$"), E_KEY_CUF},
        {std::regex("^\x1b\\[(\\d*)A$"), E_KEY_CUU},
        {std::regex("^\x1b\\[(\\d*)B$"), E_KEY_CUD},
        {std::regex("^\x1bM$"), E_KEY_RI},
        {std::regex("^\x1b\\[(\\d*)@$"), E_KEY_ICH}
    };

    std::smatch matches;

    TerminalChar ret;
    ret.ch = 0;
    ret.sequence = seq;

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
            fd_escape_data[fd].escape_seq = 0x1B;
            continue;
        }

        if (fd_escape_data[fd].in_escape) {
            if (buf[i] != 0x9c) {
                fd_escape_data[fd].escape_seq += buf[i];
            }

            if ((buf[i] >= 0x40 && buf[i] <= 0x7E && buf[i] != '[') || (buf[i] == 0x9C)) {
                vec.push_back(escape(fd_escape_data[fd].escape_seq));
                fd_escape_data[fd].in_escape = false;
                fd_escape_data[fd].escape_seq = "";
            }
        } else {
            TerminalChar tch;
            tch.ch = buf[i];
            tch.sequence = buf[i];
            vec.push_back(tch);
        }
    }

    return n;
}