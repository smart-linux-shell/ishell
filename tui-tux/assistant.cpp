#include <readline/readline.h>
#include <readline/history.h>
#include <stdio.h>
#include <stdlib.h>

#include <fstream>
#include <sstream>
#include <iostream>

#include <unordered_map>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>

#include "agency_request_wrapper.hpp"

// <alias:<command, result>>
std::unordered_map<std::string, std::pair<std::string, std::string>> bookmarks;
// <command, result>
std::vector<std::pair<std::string, std::string>> session_history;

void load_bookmarks(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << "\n";
        return;
    }
    std::string line, alias, command, result;
    std::getline(file, line); // skip the header
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        if (std::getline(iss, alias, ',') && std::getline(iss, command, ',') && std::getline(iss, result)) {
            bookmarks[alias] = {command, result};
        }
    }
    file.close();
}

void save_bookmarks(const std::string &filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << "\n";
        return;
    }
    file << "alias,command,result\n";
    for (const auto &entry : bookmarks) {
        file << entry.first << "," << entry.second.first << "," << entry.second.second << "\n";
    }
    file.close();
}

void assistant() {
    using_history();
    load_bookmarks("bookmarks.csv"); // load bookmarks

    while (1) {
        char *input = readline("assistant> ");

        if (!input) {
            break;
        }

        std::string input_str(input);

        if (input_str.find("bookmark") == 0) {
            std::istringstream iss(input_str);
            std::string cmd, alias;
            int index;
\
            iss >> cmd >> index >> alias;

            if (index > 0 && index <= history_length) {
                HIST_ENTRY *he = history_get(history_length - index + 1);
                if (he) {
                    std::string command(he->line);
                    // TODO save outputs
                    std::string result = ask_agent("http://127.0.0.1:5000/agents/assistant", command);
                    bookmarks[alias] = {command, result};
                    std::cout << "Bookmarked command: \"" << command << "\" with alias: \"" << alias << "\"\n";
                }
            } else {
                std::cout << "Invalid history index.\n";
            }
        }
        // TODO change so that not every query is searched through bookmarks
        else if (bookmarks.find(input_str) != bookmarks.end()) {
            std::string command = bookmarks[input_str].first;
            std::string result = bookmarks[input_str].second;

            std::cout << "Executing bookmarked command \"" << command << "\" with result: " << result << "\n";
        }
        else {
            add_history(input);
            std::string result = ask_agent("http://127.0.0.1:5000/agents/assistant", input_str);
            std::cout << result << "\n";
        }

        free(input);
    }
    save_bookmarks("bookmarks.csv"); // save bookmarks on exit
}
