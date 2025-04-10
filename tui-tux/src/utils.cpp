#include <utils.hpp>

std::vector<std::string> split(std::string &str, char delim, bool ignore_empty) {
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

std::pair<std::string, int> extract_command(const std::string& input) {
    std::string delimiter = "$ ";
    size_t pos = input.find(delimiter);
    if (pos == std::string::npos) {
        delimiter = "> ";
        pos = input.find(delimiter);
        if (pos == std::string::npos) {
            return std::make_pair(input, 0);
        }
        return std::make_pair(input.substr(pos + delimiter.length()), 1);
    }
    return std::make_pair(input.substr(pos + delimiter.length()), 2);
}