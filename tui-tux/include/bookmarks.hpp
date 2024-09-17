#ifndef BOOKMARKS_HPP
#define BOOKMARKS_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <utility>

extern std::unordered_map<std::string, std::pair<std::string, std::string>> bookmarks;

void load_bookmarks(const std::string &filename);
void save_bookmarks(const std::string &filename);
bool is_bookmark_command(const std::string &input_str);
bool is_bookmark_flag(const std::string &option);
bool is_bookmark(const std::string &alias);
void remove_bookmark(const std::string &alias);
bool is_remove_flag(const std::string &option);
std::pair<std::string, std::string> get_bookmark(const std::string &alias);
void handle_bookmark_command(const std::string &input_str, std::vector<std::pair<std::string, std::string>> &session_history);

#endif
