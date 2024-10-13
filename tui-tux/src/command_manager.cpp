#include <command_manager.hpp>
#include <fstream>
#include <utils.hpp>
#include <iostream>

#define MANUALS_PATH_1 "/etc/ishell/manuals"
#define MANUALS_PATH_2 "manuals"

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

        std::cerr << "Error: Command not found!\n";
    }
}

void CommandManager::run_alias(std::string &alias) {
    std::pair<std::string, std::string> bookmark = bookmark_manager->get_bookmark(alias);
    bookmark_manager->agency_manager->session_history.push_back(bookmark);
    std::cout << bookmark.second << "\n";
}

void CommandManager::clear(std::vector<std::string> &args) {
    if (args.empty()) {
        bookmark_manager->agency_manager->session_history.clear();
        std::cout << "Cleared session history.\n\n";
    }
}

int CommandManager::read_from_file(std::string &filepath, std::string &output) {
    std::ifstream file(filepath);

    if (!file.is_open()) {
        return -1;
    }

    output = "";
    std::string line;
    while (std::getline(file, line)) {
        output += line + "\n";
    }

    file.close();
    return 0;
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
    } else if (args.size() == 1 && args[0] == "--help") {
        // Try both paths
        std::string help_message;
        std::string path = std::string(MANUALS_PATH_1) + std::string("/bookmark.txt");
        int rc = read_from_file(path, help_message);
        if (rc == 0) {
            std::cout << help_message;
            return;
        }

        path = std::string(MANUALS_PATH_2) + std::string("/bookmark.txt");
        rc = read_from_file(path, help_message);
        if (rc == 0) {
            std::cout << help_message;
            return;
        }

        std::cerr << "Error: Could not find manual page 'bookmark.txt'.\n";
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
            std::cerr << "Error: Alias already in use.\n";
            return;
        }

        bookmark_manager->bookmark(index, alias);

        std::cout << "Bookmarked " << alias << " at index " << index << ".\n";
    } else {
        std::cerr << "Error: Invalid bookmark command format. Try bookmark --help.\n";
    }
}



