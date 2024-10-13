#ifndef COMMAND_MANAGER_HPP
#define COMMAND_MANAGER_HPP

#include <string>

#include "bookmark_manager.hpp"

using json = nlohmann::json;

class CommandManager {
public:
    virtual ~CommandManager() = default;

    explicit CommandManager(BookmarkManager *bookmark_manager);

    void run_command(std::string &command);
    virtual void run_alias(std::string &alias);
    void clear(const std::vector<std::string> &args);
    void bookmark(const std::vector<std::string> &args);
    virtual int read_from_file(std::string &filepath, std::string &output);

    BookmarkManager *bookmark_manager;

    std::unordered_map<std::string, void (CommandManager::*)(const std::vector<std::string>&)> command_map = {
        {"bookmark", &CommandManager::bookmark},
        {"clear", &CommandManager::clear}
    };
};

#endif //COMMAND_MANAGER_HPP
