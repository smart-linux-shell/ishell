#include <utils.hpp>

std::vector<std::string> split(std::string &str, char delim, bool ignore_empty) {
    std::vector<std::string> words;
    std::string current_str;

    for (char &character : str) {
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

std::string join(std::vector<std::string> &words, char delim) {
    std::string str;
    for (std::string &word : words) {
        str += delim;
        str += word;
    }

    if (!words.empty()) {
        // Remove first character
        std::string new_str = std::string(str.begin() + 1, str.end());
        return new_str;
    }

    return str;
}
