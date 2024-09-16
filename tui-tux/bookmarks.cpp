#include "bookmarks.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <readline/readline.h>
#include <readline/history.h>
#include "assistant_query.hpp"

// <alias:<query, result>>
std::unordered_map<std::string, std::pair<std::string, std::string>> bookmarks;

void load_bookmarks(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::ofstream create_file(filename);
        if (!create_file.is_open()) {
            std::cerr << "Error creating file: " << filename << "\n";
            return;
        }
        create_file << "alias,query,result\n";
        create_file.close();
        return;
    }
    std::string line, alias, query, result;
    std::getline(file, line);
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        if (std::getline(iss, alias, ',') && std::getline(iss, query, ',') && std::getline(iss, result)) {
            bookmarks[alias] = {query, result};
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
    file << "alias,query,result\n";
    for (const auto &entry : bookmarks) {
        file << entry.first << "," << entry.second.first << "," << entry.second.second << "\n";
    }
    file.close();
}

bool is_bookmarking(const std::string &input_str) {
    return input_str.find("bookmark") == 0;
}

bool is_bookmark_flag(const std::string &option) {
    return (option == "-b" || option == "--bookmark");
}

bool is_bookmark(const std::string &alias) {
    return bookmarks.find(alias) != bookmarks.end();
}

std::pair<std::string, std::string> get_bookmark(const std::string &alias) {
    return bookmarks[alias];
}

bool is_list_flag(const std::string &input) {
    return (input == "bookmark -l" || input == "bookmark --list");
}

void bookmark(int index, const std::string &alias, std::vector<std::pair<std::string, std::string>> &session_history) {
    if (index > 0 && index <= history_length) {
        HIST_ENTRY *he = history_get(history_length - index + 1);
        if (he) {
            std::string query(he->line);
            std::string result;

            auto it = std::find_if(session_history.begin(), session_history.end(),
                                   [&query](const std::pair<std::string, std::string> &entry) {
                                       return entry.first == query;
                                   });

            if (it != session_history.end()) {
                result = it->second;
            } else {
                result = execute_query(query, session_history);
            }
            bookmarks[alias] = {query, result};
            std::cout << "Bookmarked query: \"" << query << "\" with alias: \"" << alias << "\"\n";
        }
    } else {
        std::cout << "Invalid history index.\n";
    }
}

void list_bookmarks() {
    if (bookmarks.empty()) {
        std::cout << "No bookmarks available.\n";
        return;
    }
    std::cout << "Bookmarks:\n";
    std::cout << "BOOKMARK" << "\t\t" << "QUERY" << "\n";
    for (const auto &entry : bookmarks) {
        std::cout << entry.first << "\t\t" << entry.second.first << "\n";
    }
}

void handle_bookmark_command(const std::string &input_str, std::vector<std::pair<std::string, std::string>> &session_history) {
    std::istringstream iss(input_str);
    std::string cmd, alias;
    int index = 1;

    if (is_list_flag(input_str)) {
        list_bookmarks();
        return;
    }

    if (!(iss >> cmd >> index >> alias)) {
        iss.clear();
        iss.str(input_str);
        iss >> cmd >> alias;
        index = 1;
    }

    bookmark(index, alias, session_history);
}