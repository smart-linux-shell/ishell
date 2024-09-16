#ifndef BOOKMARKS_HPP
#define BOOKMARKS_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <utility>

extern std::unordered_map<std::string, std::pair<std::string, std::string>> bookmarks;

void load_bookmarks(const std::string &filename);
void save_bookmarks(const std::string &filename);
bool is_bookmarking(const std::string &input_str);
bool is_bookmark_flag(const std::string &option);
bool is_bookmark(const std::string &alias);
std::pair<std::string, std::string> get_bookmark(const std::string &alias);
bool is_list_flag(const std::string &input);
void bookmark(int index, const std::string &alias);
void list_bookmarks();
void handle_bookmark_command(const std::string &input_str, std::vector<std::pair<std::string, std::string>> &session_history);

#endif
