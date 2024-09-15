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

std::string execute_query(const std::string &command) {
    std::string result = ask_agent("http://127.0.0.1:5000/agents/assistant", command);
    session_history.push_back({command, result});
    return result;
}

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

// TODO change outputs - this is just for testing purposes
void bookmark(int index, const std::string &alias) {
    if (index > 0 && index <= history_length) {
        HIST_ENTRY *he = history_get(history_length - index + 1);
        if (he) {
            std::string command(he->line);
            std::string result;

            auto it = std::find_if(session_history.begin(), session_history.end(),
                                   [&command](const std::pair<std::string, std::string> &entry) {
                                       return entry.first == command;
                                   });

            if (it != session_history.end()) {
                result = it->second;
            } else {
				std::string result = execute_query(command);
            }
            bookmarks[alias] = {command, result};
            std::cout << "Bookmarked command: \"" << command << "\" with alias: \"" << alias << "\"\n";
        }
    } else {
        std::cout << "Invalid history index.\n";
    }
}


void handle_bookmark_command(const std::string &input_str) {
    std::istringstream iss(input_str);
    std::string cmd, alias;
    int index = 1;  // Default index to 1 for "bookmark <alias>" case

    // try parsing for "bookmark <index> <alias>"
    iss >> cmd >> index >> alias;

    if (iss.fail()) {
        // case where the input is just "bookmark <alias>" -- bookmark last command
        iss.clear();
        iss.str(input_str);
        iss >> cmd >> alias;
	}
    bookmark(index, alias);
}

bool is_bookmarking(const std::string &input_str) {
    return input_str.find("bookmark") == 0;
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

        if (is_bookmarking(input_str)) {
            handle_bookmark_command(input_str);
        }
		// TODO change outputs - this is just for testing purposes
        else if (bookmarks.find(input_str) != bookmarks.end()) {
            std::string command = bookmarks[input_str].first;
            std::string result = bookmarks[input_str].second;
            std::cout << "Executing bookmarked command \"" << command << "\" with result: " << result << "\n";
        }
        else {
            add_history(input);
			std::string result = execute_query(input);
            std::cout << result << "\n";
        }
        free(input);
    }
    save_bookmarks("bookmarks.csv"); // save bookmarks on exit
}
