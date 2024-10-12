#include <command_manager.hpp>
#include <utils.hpp>
#include <iostream>

CommandManager::CommandManager(BookmarkManager *bookmark_manager) {
    this->bookmark_manager = bookmark_manager;
}

void CommandManager::run_command(std::string &command) {
    std::vector<std::string> words = split(command, ' ', true);

    if (!words.empty()) {
        std::vector args(words.begin() + 1, words.end());
        for (auto &mapped_command : command_map) {
            // pair: (command name, command function accepting args)
            if (mapped_command.first == words[0]) {
                (this->*mapped_command.second)(args);
                return;
            }
        }

        // if not found, try to run an alias
        if (bookmark_manager->is_bookmark(command)) {
            run_alias(command);
            return;
        }

        std::cerr << "Command not found!\n";
    }
}

void CommandManager::run_alias(std::string &alias) {
    std::pair<std::string, std::string> bookmark = bookmark_manager->get_bookmark(alias);
    bookmark_manager->agency_manager->session_history.push_back(bookmark);
    std::cout << bookmark.second << "\n";
}

void CommandManager::clear(std::vector<std::string> &args) {
    bookmark_manager->agency_manager->session_history.clear();
    std::cout << "Cleared session history.\n\n";
}

void CommandManager::bookmark(std::vector<std::string> &args) {
    if (args.size() > 1 && (args[0] == "-r" || args[0] == "--remove")) {
        std::vector<std::string> split_alias = std::vector(args.begin() + 1, args.end());
        std::string alias = join(split_alias, ' ');
        if (bookmark_manager->is_bookmark(alias)) {
            bookmark_manager->remove_bookmark(alias);
        }

    } else if (args.size() == 1 && (args[0] == "-l" || args[0] == "--list")) {
        bookmark_manager->list_bookmarks();
    } else if (!args.empty()) {
        int index = 1;          // default index if not provided
        int alias_begin_index = 0;
        if (args.size() > 1) {
            try {
                index = std::stoi(args[0]);
                alias_begin_index++;
            } catch (std::invalid_argument &e) {}
        }

        std::vector<std::string> split_alias = std::vector(args.begin() + alias_begin_index, args.end());
        std::string alias = join(split_alias, ' ');

        if (bookmark_manager->is_bookmark(alias) || command_map.find(alias) != command_map.end()) {
            std::cerr << "Alias already in use.\n";
            return;
        }

        bookmark_manager->bookmark(index, alias);

        std::cout << "Bookmarked " << alias << " at index " << index << ".\n";
    } else {
        std::cerr << "Wrong command syntax.\n";
    }
}



