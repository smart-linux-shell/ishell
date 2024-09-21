#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <unordered_map>
#include <readline/readline.h>
#include <readline/history.h>
#include "../nlohmann/json.hpp"
#include "../include/bookmark_manager.hpp"
#include "../include/agency_manager.hpp"

using json = nlohmann::json;

BookmarkManager::BookmarkManager() {
}

bool BookmarkManager::create_bookmarks_file(const std::string &filename) {
    std::ofstream create_file(filename);
    if (!create_file) {
        std::cerr << "Error creating file: " << filename << "\n";
        return false;
    }
    json empty_bookmarks = json::array();
    create_file << empty_bookmarks.dump(4);
    return true;
}

void BookmarkManager::parse_bookmark_json(const json &bookmark) {
    std::string alias = bookmark.at("alias").get<std::string>();
    std::string query = bookmark.at("query").get<std::string>();
    std::string result = bookmark.at("result").get<std::string>();
    bookmarks[alias] = {query, result};
}

void BookmarkManager::load_bookmarks(const std::string &filename) {
    std::ifstream file(filename);
    if (!file) {
        if (!create_bookmarks_file(filename)) {
            return;
        }
        return;  // No bookmarks to load if the file is newly created
    }
    json bookmark_json;
    file >> bookmark_json;
    for (const json &bookmark : bookmark_json) {
        parse_bookmark_json(bookmark);
    }
}

void BookmarkManager::save_bookmarks(const std::string &filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << "\n";
        return;
    }
    json bookmark_json = json::array();
    for (const auto &bookmark : bookmarks) {
        json entry;
        entry["alias"] = bookmark.first;
        entry["query"] = bookmark.second.first;
        entry["result"] = bookmark.second.second;
        bookmark_json.push_back(entry); // Add entry to the array
    }
    file << bookmark_json.dump(4);
}

bool BookmarkManager::is_bookmark_command(const std::string &input_str) const {
    return input_str.find("bookmark") == 0;
}

bool BookmarkManager::is_bookmark_flag(const std::string &option) const {
    return (option == "-b" || option == "--bookmark");
}

bool BookmarkManager::is_remove_flag(const std::string &option) const {
    return (option == "-r" || option == "--remove");
}

bool BookmarkManager::is_bookmark(const std::string &alias) const {
    return bookmarks.find(alias) != bookmarks.end();
}

std::pair<std::string, std::string> BookmarkManager::get_bookmark(const std::string &alias) const {
    std::unordered_map<std::string, std::pair<std::string, std::string>>::const_iterator it = bookmarks.find(alias);
    if (it != bookmarks.end()) {
        return it->second; // Return the value if found
    }
    return {"", ""}; // Return an empty pair if not found
}

bool BookmarkManager::is_list_flag(const std::string &input) const {
    return (input == "bookmark -l" || input == "bookmark --list");
}

bool BookmarkManager::is_help_flag(const std::string &input) const {
    return (input == "bookmark -h" || input == "bookmark --help");
}

std::string BookmarkManager::get_query_from_history(int index) {
    if (index > 0 && index <= history_length) {
        HIST_ENTRY *he = history_get(history_length - index + 1);
        if (he) {
            return std::string(he->line);
        }
    }
    return "";
}

std::string BookmarkManager::find_result_in_session_history(const std::string &query, std::vector<std::pair<std::string, std::string>> &session_history) {
    std::vector<std::pair<std::string, std::string>>::iterator it = std::find_if(session_history.begin(), session_history.end(),
                           [&query](const std::pair<std::string, std::string> &entry) {
                               return entry.first == query;
                           });
    if (it != session_history.end()) {
        return it->second;
    }
    return "";
}

void BookmarkManager::bookmark(int index, const std::string &alias, std::vector<std::pair<std::string, std::string>> &session_history) {
    if (bookmarks.find(alias) != bookmarks.end()) {
        std::cout << "Error: Bookmark '" << alias << "' already exists.\n";
        return;
    }
    std::string query = get_query_from_history(index);
    if (query.empty()) {
        std::cout << "Error: Invalid history index.\n";
        return;
    }
    AgencyManager agency_manager;
    std::string result = agency_manager.execute_query(query, session_history);
    if (result.empty()) {
        std::cout << "Error executing query.\n";
        return;
    }
    bookmarks[alias] = {query, result};
    std::cout << "Saved the query under the bookmark '" << alias  << "'" << "\n";
}

void BookmarkManager::remove_bookmark(const std::string &alias) {
    std::unordered_map<std::string, std::pair<std::string, std::string>>::iterator it = bookmarks.find(alias);
    if (it != bookmarks.end()) {
        bookmarks.erase(it);
        std::cout << "Removed bookmark '" << alias << "'.\n";
    } else {
        std::cout << "Error: Bookmark '" << alias << "' not found.\n";
    }
}

void BookmarkManager::list_bookmarks() const {
    const int alias_width = 20;
    const int query_width = 50;
    std::cout << std::left << std::setw(alias_width) << "BOOKMARK"
              << std::setw(query_width) << "QUERY" << "\n";
    for (std::unordered_map<std::string, std::pair<std::string, std::string>>::const_iterator it = bookmarks.begin(); it != bookmarks.end(); ++it) {
        std::cout << std::left << std::setw(alias_width) << it->first
                  << std::setw(query_width) << it->second.first
                  << "\n";
    }
}

void BookmarkManager::help() {
    std::ifstream file("manuals/bookmark.txt");
    if (!file.is_open()) {
        std::cerr << "Error: Could not find the documentation.\n";
        return;
    }
    std::string line;
    while (std::getline(file, line)) {
        std::cout << line << "\n";
    }
    file.close();
}

bool BookmarkManager::try_parse_bookmark_command(const std::string &input_str, std::string &cmd, int &index, std::string &alias) {
    std::istringstream iss(input_str);
    // Try to parse "bookmark <index> <alias>"
    if (iss >> cmd >> index >> alias) {
        return true;
    }
    // If parsing fails, try to parse "bookmark <alias>"
    iss.clear();
    iss.str(input_str);
    if (iss >> cmd >> alias) {
        index = 1; // Default index if not provided
        return true;
    }
    return false;
}

void BookmarkManager::handle_bookmark_command(const std::string &input_str, std::vector<std::pair<std::string, std::string>> &session_history) {
    if (is_list_flag(input_str)) {
        list_bookmarks();
        return;
    }
    else if (is_help_flag(input_str)) {
        help();
        return;
    }

    std::string cmd;
    int index;
    std::string alias;
    if (try_parse_bookmark_command(input_str, cmd, index, alias)) {
        bookmark(index, alias, session_history);
    } else {
        std::cerr << "Invalid bookmark command format.\n";
        help();
    }
}
