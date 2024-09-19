#ifndef BOOKMARKS_HPP
#define BOOKMARKS_HPP

#include <unordered_map>
#include <string>
#include <vector>
#include <utility>
#include "../nlohmann/json.hpp"
using json = nlohmann::json;

// <alias:<query, result>>
extern std::unordered_map<std::string, std::pair<std::string, std::string>> bookmarks;

bool create_bookmarks_file(const std::string &filename);
void parse_bookmark_json(const nlohmann::json &bookmark);
void load_bookmarks(const std::string &filename);
void save_bookmarks(const std::string &filename);
bool is_bookmark_command(const std::string &input_str);
bool is_bookmark_flag(const std::string &option);
bool is_remove_flag(const std::string &option);
bool is_bookmark(const std::string &alias);
std::pair<std::string, std::string> get_bookmark(const std::string &alias);
bool is_list_flag(const std::string &input);
bool is_help_flag(const std::string &input);
std::string get_query_from_history(int index);
std::string find_result_in_session_history(const std::string &query, std::vector<std::pair<std::string, std::string>> &session_history);
void bookmark(int index, const std::string &alias, std::vector<std::pair<std::string, std::string>> &session_history);
void remove_bookmark(const std::string &alias);
void list_bookmarks();
void help();
bool try_parse_bookmark_command(const std::string &input_str, std::string &cmd, int &index, std::string &alias);
void handle_bookmark_command(const std::string &input_str, std::vector<std::pair<std::string, std::string>> &session_history);

#endif // BOOKMARKS_HPP
