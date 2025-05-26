#ifndef MARKDOWN_MANAGER_HPP
#define MARKDOWN_MANAGER_HPP

#include <unordered_map>
#include <string>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <sys/stat.h>
#include "../nlohmann/json.hpp"
#include "bookmark_manager.hpp"
#include "agency_manager.hpp"

using json = nlohmann::json;

class MarkdownManager {

public:
    explicit MarkdownManager();
    virtual ~MarkdownManager();

    virtual bool create_markdowns_file(const std::string &filename);
    virtual void parse_markdown_json(const json &markdown);

    virtual void load_markdowns(const std::string &filename);
    virtual void save_markdowns(const std::string &filename);

    virtual bool is_markdown(const std::string &bookmark) const;
    virtual std::pair<std::string, std::string> get_markdown(const std::string &bookmark) const;

    virtual void markdown(const std::pair<std::string, std::string>& protocol);          // create markdown from bookmark with "default" group
    virtual void list_markdowns() const;                                                  // list markdowns "markdown -l"
    virtual void remove_markdown(const std::string &bookmark);                           // remove markdown "markdown -r"
    virtual void add_to_group(const std::string &group, const std::string &bookmark);    // change markdown's group
    virtual std::vector<std::string> get_by_group(const std::string &group);             // give LLM context of this group
    virtual bool save_as_file(const std::string& bookmark);                              // save markdown as <bookmark>.md

	 std::unordered_map<std::string, std::pair<std::string, std::string>> markdowns;     // <bookmark> [<group, protocol>]
};

#endif //MARKDOWN_MANAGER_HPP
