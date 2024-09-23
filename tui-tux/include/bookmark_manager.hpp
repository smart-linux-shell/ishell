#ifndef BOOKMARKS_HPP
#define BOOKMARKS_HPP

#include <unordered_map>
#include <string>
#include <vector>
#include <utility>
#include "../nlohmann/json.hpp"
#include "agency_manager.hpp"

using json = nlohmann::json;

class BookmarkManager {
private:
    AgencyManager *agency_manager;
public:
    BookmarkManager(AgencyManager *agency_manager);
    virtual ~BookmarkManager();

    virtual void load_bookmarks(const std::string &filename);
    virtual void save_bookmarks(const std::string &filename);

    virtual bool is_bookmark_command(const std::string &input_str) const;
    virtual bool is_bookmark_flag(const std::string &option) const;
    virtual bool is_remove_flag(const std::string &option) const;
    virtual bool is_bookmark(const std::string &alias) const;
    virtual std::pair<std::string, std::string> get_bookmark(const std::string &alias) const;
    virtual bool is_list_flag(const std::string &input) const;
    virtual bool is_help_flag(const std::string &input) const;

    virtual void list_bookmarks() const;
    virtual void remove_bookmark(const std::string &alias);
    virtual void bookmark(int index, const std::string &alias, std::vector<std::pair<std::string, std::string>> &session_history);
    virtual void handle_bookmark_command(const std::string &input_str, std::vector<std::pair<std::string, std::string>> &session_history);

    std::unordered_map<std::string, std::pair<std::string, std::string>> bookmarks;

    virtual bool create_bookmarks_file(const std::string &filename);
    virtual void parse_bookmark_json(const json &bookmark);
    virtual std::string get_query_from_history(int index);
    virtual std::string find_result_in_session_history(const std::string &query, std::vector<std::pair<std::string, std::string>> &session_history);
    virtual bool try_parse_bookmark_command(const std::string &input_str, std::string &cmd, int &index, std::string &alias);
    virtual void help();
};

#endif // BOOKMARKS_HPP
