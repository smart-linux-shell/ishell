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

#include <stdlib.h>

#define AGENT_SYSTEM 0
#define AGENT_ASSISTANT 1
#define N_AGENTS 2

// <query, result>
std::vector<std::pair<std::string, std::string>> session_history;

// Global variables are unfortunately needed because of readline limitations
AgencyRequestWrapper request_wrapper;
AgencyManager manager(&request_wrapper);
BookmarkManager bookmark_manager(&manager);

bool running = true;
int agent_type = AGENT_ASSISTANT;

void line_handler(char *line) {
    if (!line) {
        running = false;
        return;
    }

    std::string input_str(line);

    if (agent_type == AGENT_ASSISTANT) {
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
                // New query to agent
                add_history(line);
                std::string result = manager.execute_query(input_str, session_history);
                std::cout << result << "\n";
            }
        }
    } else if (agent_type == AGENT_SYSTEM) {
        std::cout << "Unimplemented yet!\n";
    }

    free(line);
}

void agent() {
    // Disable TAB-completion. This was impossible to find
    rl_inhibit_completion = 1;

    using_history();
    manager.get_agency_url();

    bookmark_manager.load_bookmarks("local/bookmarks.json");

    std::string assistant_name = "assistant";
    std::string system_name = "system";

    while (running) {
        std::string agent_name;
        if (agent_type == AGENT_ASSISTANT) {
            agent_name = assistant_name;
        } else if (agent_type == AGENT_SYSTEM) {
            agent_name = system_name;
        }

        std::string prompt = agent_name + "> ";
        rl_callback_handler_install(prompt.c_str(), &line_handler);

        while (running) {
            rl_callback_read_char();

            int pos = rl_point;

            if (pos > 0 && rl_line_buffer[pos - 1] == '\t') {
                // Erase last character, break out, switch agent type
                rl_delete_text(pos - 1, pos);
                rl_point = rl_end;
                rl_redisplay();
                agent_type = (agent_type + 1) % N_AGENTS;
                break;
            }
        }

        std::cout << "\n";
    }

    bookmark_manager.save_bookmarks("local/bookmarks.json");
}
