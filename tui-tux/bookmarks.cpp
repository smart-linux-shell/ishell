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

bool create_bookmarks_file(const std::string &filename) {
    std::ofstream create_file(filename);
    if (!create_file) {
        std::cerr << "Error creating file: " << filename << "\n";
        return false;
    }
    create_file << "alias,query,result\n";  // add header
    return true;
}

void parse_bookmark_line(const std::string &line) {
    std::istringstream iss(line);
    std::string alias, query, result;
    if (std::getline(iss, alias, ',') && std::getline(iss, query, ',')) {
        if (!std::getline(iss, result)) {
            result = "";
        }
        bookmarks[alias] = {query, result};
    }
}

void load_bookmarks(const std::string &filename) {
    std::ifstream file(filename);
    if (!file) {
        if (!create_bookmarks_file(filename)) {
            return;
        }
        return;  // no bookmarks - no load
    }
    std::string line;
    std::getline(file, line); // skip the header
    while (std::getline(file, line)) {
        parse_bookmark_line(line);
    }
}

void save_bookmarks(const std::string &filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << "\n";
        return;
    }
    file << "alias,query,result\n";
    for (std::unordered_map<std::string, std::pair<std::string, std::string>>::const_iterator it = bookmarks.begin(); it != bookmarks.end(); ++it) {
        file << it->first << "," << it->second.first << "," << it->second.second << "\n";
    }
    file.close();
}

bool is_bookmark_command(const std::string &input_str) {
    return input_str.find("bookmark") == 0;
}

bool is_bookmark_flag(const std::string &option) {
    return (option == "-b" || option == "--bookmark");
}

bool is_remove_flag(const std::string &option) {
    return (option == "-r" || option == "--remove");
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

bool is_help_flag(const std::string &input) {
    return (input == "bookmark -h" || input == "bookmark --help");
}

std::string get_query_from_history(int index) {
    if (index > 0 && index <= history_length) {
        HIST_ENTRY *he = history_get(history_length - index + 1);
        if (he) {
            return std::string(he->line);
        }
    }
    return "";
}

std::string find_result_in_session_history(const std::string &query, std::vector<std::pair<std::string, std::string>> &session_history) {
    std::vector<std::pair<std::string, std::string>>::iterator it = std::find_if(session_history.begin(), session_history.end(),
                           [&query](const std::pair<std::string, std::string> &entry) {
                               return entry.first == query;
                           });
    if (it != session_history.end()) {
        return it->second;
    }
    return "asd";
}

void bookmark(int index, const std::string &alias, std::vector<std::pair<std::string, std::string>> &session_history) {
    // bookmark already exists
    if (bookmarks.find(alias) != bookmarks.end()) {
        std::cout << "Error: Bookmark '" << alias << "' already exists.\n";
        return;
    }

    // find query
    std::string query = get_query_from_history(index);
    if (query.empty()) {
        std::cout << "Error: Invalid history index.\n";
        return;
    }
    // find result
    std::string result = find_result_in_session_history(query, session_history);
    if (result.empty()) {
        result = execute_query(query, session_history);
    }
    // bookmark
    bookmarks[alias] = {query, result};
    std::cout << "Saved the query under the bookmark '" << alias  << "'" << "\n";
}

void remove_bookmark(const std::string &alias) {
    std::unordered_map<std::string, std::pair<std::string, std::string>>::iterator it = bookmarks.find(alias);
    if (it != bookmarks.end()) {
        bookmarks.erase(it);
        std::cout << "Removed bookmark '" << alias << "'.\n";
    } else {
        std::cout << "Error: Bookmark '" << alias << "' not found.\n";
    }
}

void list_bookmarks() {
    std::cout << "BOOKMARK" << "\t\t" << "QUERY" << "\n";
    for (std::unordered_map<std::string, std::pair<std::string, std::string>>::const_iterator it = bookmarks.begin(); it != bookmarks.end(); ++it) {
        std::cout << it->first << "\t\t\t" << it->second.first << "\n";
    }
}

bool try_parse_bookmark_command(const std::string &input_str, std::string &cmd, int &index, std::string &alias) {
    std::istringstream iss(input_str);
    // try to parse "bookmark <index> <alias>"
    if (iss >> cmd >> index >> alias) {
        return true;
    }
    // if parsing fails, try to parse "bookmark <alias>"
    iss.clear();
    iss.str(input_str);
    if (iss >> cmd >> alias) {
        index = 1; // default index if not provided
        return true;
    }
    return false;
}


void handle_bookmark_command(const std::string &input_str, std::vector<std::pair<std::string, std::string>> &session_history) {
    if (is_list_flag(input_str)) {
        list_bookmarks();
        return;
    }

    std::string cmd;
    int index;
    std::string alias;
    if (try_parse_bookmark_command(input_str, cmd, index, alias)) {
        bookmark(index, alias, session_history);
    } else {
        std::cerr << "Invalid bookmark command format.\n";
    }
}
