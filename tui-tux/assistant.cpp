#include <readline/readline.h>
#include <readline/history.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iostream>
#include "bookmarks.hpp"
#include "bookmarks.hpp"
#include "assistant_query.hpp"

// <query, result>
std::vector<std::pair<std::string, std::string>> session_history;

void assistant() {
    using_history();
    load_bookmarks("bookmarks.csv");

    while (1) {
        char *input = readline("assistant> ");

        if (!input) {
            break;
        }

        std::string input_str(input);

        if (is_bookmark_command(input_str)) {
            handle_bookmark_command(input_str, session_history);
        } else {
            std::istringstream iss(input_str);
            std::string alias, option;
            iss >> alias >> option;
            if (is_bookmark_flag(option) && is_bookmark(alias)) {
				// use bookmarked
                auto [query, result] = get_bookmark(alias);
                session_history.push_back({query, result});
                std::cout << result << "\n";
            } else {
    			// new query to assistant
                add_history(input);
                std::string result = execute_query(input_str, session_history);
                std::cout << result << "\n";
            }
        }
        free(input);
    }

    save_bookmarks("bookmarks.csv");
}
