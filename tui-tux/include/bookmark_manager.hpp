#ifndef BOOKMARKS_HPP
#define BOOKMARKS_HPP

#include <unordered_map>
#include <string>
#include <vector>
#include <utility>
#include "../nlohmann/json.hpp"

using json = nlohmann::json;

class BookmarkManager {
public:
    BookmarkManager();

    void load_bookmarks(const std::string &filename);
    void save_bookmarks(const std::string &filename);

    bool is_bookmark_command(const std::string &input_str) const;
    bool is_bookmark_flag(const std::string &option) const;
    bool is_remove_flag(const std::string &option) const;
    bool is_bookmark(const std::string &alias) const;
    std::pair<std::string, std::string> get_bookmark(const std::string &alias) const;
    bool is_list_flag(const std::string &input) const;
    bool is_help_flag(const std::string &input) const;

    void list_bookmarks() const;
    void remove_bookmark(const std::string &alias);
    void bookmark(int index, const std::string &alias, std::vector<std::pair<std::string, std::string>> &session_history);
    void handle_bookmark_command(const std::string &input_str, std::vector<std::pair<std::string, std::string>> &session_history);

    std::unordered_map<std::string, std::pair<std::string, std::string>> bookmarks;

    bool create_bookmarks_file(const std::string &filename);
    void parse_bookmark_json(const json &bookmark);
    std::string get_query_from_history(int index);
    std::string find_result_in_session_history(const std::string &query, std::vector<std::pair<std::string, std::string>> &session_history);
    bool try_parse_bookmark_command(const std::string &input_str, std::string &cmd, int &index, std::string &alias);
    void help();
};

#endif // BOOKMARKS_HPP
