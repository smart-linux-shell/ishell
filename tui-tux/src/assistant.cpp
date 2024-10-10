#include <readline/readline.h>
#include <readline/history.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <vector>
#include <agency_manager.hpp>
#include <bookmark_manager.hpp>
#include <agency_request_wrapper.hpp>

// <query, result>
std::vector<std::pair<std::string, std::string>> session_history;

void assistant() {
    AgencyRequestWrapper request_wrapper;
    AgencyManager manager(&request_wrapper);
    using_history();
    manager.get_agency_url();


    BookmarkManager bookmark_manager(&manager);

    bookmark_manager.load_bookmarks("local/bookmarks.json");

    while (1) {
        char *input = readline("assistant> ");

        if (!input) {
            break;
        }

        std::string input_str(input);

        if (bookmark_manager.is_bookmark_command(input_str)) {
            bookmark_manager.handle_bookmark_command(input_str, session_history);
        } else {
            std::istringstream iss(input_str);
            std::string alias, option;
            iss >> alias >> option;
            if (bookmark_manager.is_bookmark_flag(option) && bookmark_manager.is_bookmark(alias)) {
                // Use bookmarked
                std::pair<std::string, std::string> bookmark = bookmark_manager.get_bookmark(alias);
                session_history.push_back(bookmark);
                std::cout << bookmark.second << "\n";
            } else if (bookmark_manager.is_remove_flag(option)) {
                // Remove bookmark
                bookmark_manager.remove_bookmark(alias);
            } else if (input_str == "clear") {
                add_history(input);
                session_history.clear();
                std::cout << "Cleared session history.\n\n";
            } else {
                // New query to assistant
                add_history(input);
                std::string result = manager.execute_query(input_str, session_history);
                std::cout << result << "\n";
            }
        }
        free(input);
    }

    bookmark_manager.save_bookmarks("local/bookmarks.json");
}
