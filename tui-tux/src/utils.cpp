#include <utils.hpp>

#include <regex>

std::vector<std::string> split(const std::string &str, char delim, bool ignore_empty) {
    std::vector<std::string> words;
    std::string current_str;

    for (const char &character : str) {
        if (character != delim) {
            current_str += character;
        } else {
            if (ignore_empty && current_str.empty()) {
                continue;
            }
            words.push_back(current_str);
            current_str = "";
        }
    }

    if (!current_str.empty()) {
        words.push_back(current_str);
    }

    return words;
}

std::string join(std::vector<std::string> &words, const char delim) {
    std::string str;
    for (const std::string &word : words) {
        str += delim;
        str += word;
    }

    if (!words.empty()) {
        // Remove first character
        auto new_str = std::string(str.begin() + 1, str.end());
        return new_str;
    }

    return str;
}

bool is_range_token(const std::string& s) {
    // "N-M", "-M", "N-"
    static const std::regex re(R"(^(\d*)-(\d*)$)");
    std::smatch m;
    return std::regex_match(s, m, re) && (m[1].length() || m[2].length());
}
