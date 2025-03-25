#include <command_manager.hpp>
#include <session_tracker.hpp>
#include <fstream>
#include <utils.hpp>
#include <iostream>

#define MANUALS_PATH_1 "/etc/ishell/manuals"
#define MANUALS_PATH_2 "manuals"

CommandManager::CommandManager(BookmarkManager *bookmark_manager) {
    this->bookmark_manager = bookmark_manager;
}

void CommandManager::run_command(std::string &command) {

    SessionTracker::get().logEvent(SessionTracker::EventType::SystemCommand, command);

    if (std::vector<std::string> words = split(command, ' ', true); !words.empty()) {
        const std::vector args(words.begin() + 1, words.end());
        for (auto &[command_name, command_function] : command_map) {
            // pair: (command name, command function accepting args)
            if (command_name == words[0]) {
                (this->*command_function)(args);
                return;
            }
        }

        // if not found, try to run an alias
        if (bookmark_manager->is_bookmark(command)) {
            run_alias(command);
            return;
        }

        std::string error_msg = "Error: Command not found!";
        std::cerr << error_msg << std::endl;
        SessionTracker::get().finalizeCommand(1, error_msg);
    }
}

void CommandManager::run_alias(std::string &alias) {
    const std::pair<std::string, std::string> bookmark = bookmark_manager->get_bookmark(alias);
    bookmark_manager->agency_manager->session_history.push_back(bookmark);
    std::cout << bookmark.second << "\n";
    SessionTracker::get().finalizeCommand(0, "Alias executed: " + alias);
}

void CommandManager::clear(const std::vector<std::string> &args) {
    if (args.empty()) {
        bookmark_manager->agency_manager->session_history.clear();
        std::cout << "Cleared session history.\n\n";
        SessionTracker::get().finalizeCommand(0, "Cleared session history.");
    } else {
        SessionTracker::get().finalizeCommand(1, "Error: clear command does not take arguments.");
    }
}

void CommandManager::agent_switch(const std::vector<std::string> &args) {
    if (args.size() > 1) {
        std::cerr << "Error: Invalid syntax. Usage: switch <agent name>\n";
        SessionTracker::get().finalizeCommand(1, "Error: Invalid syntax. Usage: switch <agent name>");
        return;
    }

    bookmark_manager->agency_manager->set_agent_name(args[0]);
    SessionTracker::get().finalizeCommand(0, "Switched agent to: " + args[0]);
}

int CommandManager::read_from_file(std::string &filepath, std::string &output) {
    std::ifstream file(filepath);

    if (!file.is_open()) {
        SessionTracker::get().finalizeCommand(1, "Error: Unable to open file: " + filepath);
        return -1;
    }

    output = "";
    std::string line;
    while (std::getline(file, line)) {
        output += line + "\n";
    }

    file.close();
    SessionTracker::get().finalizeCommand(0, "Opened file: " + filepath);
    return 0;
}

void CommandManager::bookmark(const std::vector<std::string> &args) {
    if (args.size() > 1 && (args[0] == "-r" || args[0] == "--remove")) {
        auto split_alias = std::vector(args.begin() + 1, args.end());
        if (const std::string alias = join(split_alias, ' '); bookmark_manager->is_bookmark(alias)) {
            bookmark_manager->remove_bookmark(alias);
            SessionTracker::get().finalizeCommand(0, "Bookmark removed: " + alias);
        }

    } else if (args.size() == 1 && (args[0] == "-l" || args[0] == "--list")) {
        bookmark_manager->list_bookmarks();
        SessionTracker::get().finalizeCommand(0, "Bookmark listed: " + args[0]);
    } else if (args.size() == 1 && args[0] == "--help") {
        // Try both paths
        std::string help_message;
        std::string path = std::string(MANUALS_PATH_1) + std::string("/bookmark.txt");
        int rc = read_from_file(path, help_message);
        if (rc == 0) {
            std::cout << help_message;
            SessionTracker::get().finalizeCommand(0, "Help command was executed with path: " + path);
            return;
        }

        path = std::string(MANUALS_PATH_2) + std::string("/bookmark.txt");
        rc = read_from_file(path, help_message);
        if (rc == 0) {
            std::cout << help_message;
             SessionTracker::get().finalizeCommand(0, "Help command was executed with path: " + path);
            return;
        }

        std::cerr << "Error: Could not find manual page 'bookmark.txt'.\n";
        SessionTracker::get().finalizeCommand(1, "Error: Could not find manual page 'bookmark.txt'.");
    } else if (!args.empty()) {
        int index = 1;          // default index if not provided
        int alias_begin_index = 0;
        if (args.size() > 1) {
            try {
                index = std::stoi(args[0]);
                alias_begin_index++;
            } catch ([[maybe_unused]] std::invalid_argument &e) {
                 SessionTracker::get().finalizeCommand(1, "Error: Invalid syntax.");
            }
        }

        auto split_alias = std::vector(args.begin() + alias_begin_index, args.end());
        const std::string alias = join(split_alias, ' ');

        if (bookmark_manager->is_bookmark(alias) || command_map.find(alias) != command_map.end()) {
            std::cerr << "Error: Alias already in use.\n";
            SessionTracker::get().finalizeCommand(1, "Error: Alias '" + alias + "' already in use.");
            return;
        }

        bookmark_manager->bookmark(index, alias);
        SessionTracker::get().finalizeCommand(0, "Bookmark added: " + alias);
    } else {
        std::cerr << "Error: Invalid bookmark command format. Try bookmark --help.\n";
        SessionTracker::get().finalizeCommand(1, "Error: Invalid bookmark command format.");
    }
}
