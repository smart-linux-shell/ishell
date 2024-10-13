#include <readline/readline.h>
#include <readline/history.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>
#include <agency_manager.hpp>
#include <bookmark_manager.hpp>
#include <agency_request_wrapper.hpp>

#include "command_manager.hpp"

#define MODE_SYSTEM 0
#define MODE_AGENT 1
#define N_AGENTS 2

// Global variables are unfortunately needed because of readline limitations
std::vector<std::vector<std::string>> history_storage;      // stores histories for all tabs

AgencyRequestWrapper request_wrapper;
AgencyManager manager(&request_wrapper);
BookmarkManager bookmark_manager(&manager);
CommandManager command_manager(&bookmark_manager);

bool running = true;
int prompt_mode = MODE_AGENT;

void save_history(std::vector<std::string>& history_storage) {
    history_storage.clear();
    for (HIST_ENTRY **entry = history_list(); entry && *entry; ++entry) {
        history_storage.emplace_back((*entry)->line);
    }
}

void load_history(const std::vector<std::string>& history_storage) {
    clear_history();
    for (const std::string& line : history_storage) {
        add_history(line.c_str());
    }
}

void line_handler(char *line) {
    if (!line) {
        running = false;
        return;
    }

    std::string input_str(line);
    add_history(line);

    if (prompt_mode == MODE_AGENT) {
        // New query to agent
        const std::string agency = manager.get_agency_url();
        const std::string result = manager.execute_query(agency + "/" + bookmark_manager.agency_manager->get_agent_name(), input_str);
        std::cout << result << "\n";
    } else if (prompt_mode == MODE_SYSTEM) {
        command_manager.run_command(input_str);
    }

    free(line);
}

void agent() {
    // Enable history storage
    history_storage = std::vector(2, std::vector<std::string>());

    // Disable TAB-completion. This was impossible to find
    rl_inhibit_completion = 1;

    using_history();
    manager.get_agency_url();

    bookmark_manager.load_bookmarks("local/bookmarks.json");

    const std::string system_name = "system";

    while (running) {
        std::string agent_name;
        if (prompt_mode == MODE_AGENT) {
            agent_name = bookmark_manager.agency_manager->get_agent_name();
        } else if (prompt_mode == MODE_SYSTEM) {
            agent_name = system_name;
        }

        std::string prompt = agent_name + "> ";
        rl_callback_handler_install(prompt.c_str(), &line_handler);

        while (running) {
            rl_callback_read_char();

            if (const int pos = rl_point; pos > 0 && rl_line_buffer[pos - 1] == '\t') {
                // Erase last character, break out, switch agent type
                rl_delete_text(pos - 1, pos);
                rl_point = rl_end;
                rl_redisplay();
                save_history(history_storage[prompt_mode]);
                prompt_mode = (prompt_mode + 1) % N_AGENTS;
                load_history(history_storage[prompt_mode]);
                break;
            }
        }

        std::cout << "\n";
    }

    bookmark_manager.save_bookmarks("local/bookmarks.json");
}
