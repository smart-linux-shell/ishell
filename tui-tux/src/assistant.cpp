#include <readline/readline.h>
#include <readline/history.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <algorithm>

#include "../include/assistant_query.hpp"
#include "../include/bookmarks.hpp"
#include "../include/agency_request_wrapper.hpp"


// <query, result>
std::vector<std::pair<std::string, std::string>> session_history;

void assistant() {
    using_history();
    get_agency_url();
    load_bookmarks("local/bookmarks.csv");

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
            } else if (is_remove_flag(option)) {
                // remove bookmark
                remove_bookmark(alias);
            } else {
                // new query to assistant
                add_history(input);
                std::string result = execute_query(input_str, session_history);
                std::cout << result << "\n";
            }
        }
        free(input);
    }

    save_bookmarks("local/bookmarks.csv");
}
