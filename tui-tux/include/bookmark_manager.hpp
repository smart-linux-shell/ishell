#ifndef BOOKMARKS_HPP
#define BOOKMARKS_HPP

#include <unordered_map>
#include <string>
#include <utility>
#include <optional>
#include "../nlohmann/json.hpp"
#include <agency_manager.hpp>

using json = nlohmann::json;

class BookmarkManager {

public:
    AgencyManager *agency_manager{};

    explicit BookmarkManager(AgencyManager *agency_manager);
    virtual ~BookmarkManager();

//    virtual std::string get_bookmarks_file_path();

    std::unordered_map<std::string, std::pair<std::string, std::string>> bookmarks;

    virtual void load_bookmarks(const std::string &filename);
    virtual void save_bookmarks(const std::string &filename);

    virtual bool is_bookmark(const std::string &alias) const;
    virtual std::pair<std::string, std::string> get_bookmark(const std::string &alias) const;

    virtual void list_bookmarks() const;
    virtual void remove_bookmark(const std::string &alias);
    virtual void bookmark(int first, int last, const std::string& alias);

    virtual bool create_bookmarks_file(const std::string &filename);
    virtual void parse_bookmark_json(const json &bookmark);
};

#endif // BOOKMARKS_HPP
