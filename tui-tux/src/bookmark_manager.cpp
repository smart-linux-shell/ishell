#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <unordered_map>
#include <readline/readline.h>
#include <readline/history.h>
#include "../nlohmann/json.hpp"
#include <bookmark_manager.hpp>
#include <agency_manager.hpp>

using json = nlohmann::json;

BookmarkManager::BookmarkManager(AgencyManager* agencyManager) {
  agency_manager = agencyManager;
}

BookmarkManager::~BookmarkManager() {}

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

bool BookmarkManager::is_bookmark(const std::string &alias) const {
    return bookmarks.find(alias) != bookmarks.end();
}

std::pair<std::string, std::string> BookmarkManager::get_bookmark(const std::string &alias) const {
    auto it = bookmarks.find(alias);
    if (it != bookmarks.end()) {
        return it->second; // Return the value if found
    }
    return {"", ""}; // Return an empty pair if not found
}

void BookmarkManager::bookmark(int index, const std::string &alias) {
   // bookmark already exists
    if (bookmarks.find(alias) != bookmarks.end()) {
        std::cerr << "Error: Bookmark '" << alias << "' already exists.\n";
        return;
    }

    // find <query, result> pair
    int len = agency_manager->session_history.size();
    if (index <= 0 || index > len) {
        std::cerr << "Error: Invalid history index.\n";
        return;
    }

    auto pair = agency_manager->session_history[len - index];

    // bookmark
    bookmarks[alias] = pair;
    std::cout << "Saved the query under the bookmark '" << alias  << "'" << "\n";
}

void BookmarkManager::remove_bookmark(const std::string &alias) {
    std::unordered_map<std::string, std::pair<std::string, std::string>>::iterator it = bookmarks.find(alias);
    if (it != bookmarks.end()) {
        bookmarks.erase(it);
        std::cout << "Removed bookmark '" << alias << "'.\n";
    } else {
        std::cerr << "Error: Bookmark '" << alias << "' not found.\n";

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