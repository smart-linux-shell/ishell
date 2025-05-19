#include <command_manager.hpp>
#include <fstream>
#include <utils.hpp>
#include <iostream>
#include <algorithm>

#define MANUALS_PATH_1 "/etc/ishell/manuals"
#define MANUALS_PATH_2 "manuals"

CommandManager::CommandManager(BookmarkManager *bookmark_manager, MarkdownManager *markdown_manager) {
    this->bookmark_manager = bookmark_manager;
    this->markdown_manager = markdown_manager;
}

void CommandManager::run_command(std::string &command) {
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

        std::cerr << "Error: Command not found!\n";
    }
}

void CommandManager::run_alias(std::string &alias) {
    const std::pair<std::string, std::string> bookmark = bookmark_manager->get_bookmark(alias);
    // bookmark_manager->agency_manager->session_history.push_back(bookmark);
    std::cout << bookmark.second << "\n";
}

void CommandManager::clear(const std::vector<std::string> &args) {
    if (args.empty()) {
    //    bookmark_manager->agency_manager->session_history.clear();
        std::cout << "Cleared session history.\n\n";
    }
}

void CommandManager::agent_switch(const std::vector<std::string> &args) {
    if (args.size() > 1) {
        std::cerr << "Error: Invalid syntax. Usage: switch <agent name>\n";
        return;
    }

    bookmark_manager->agency_manager->set_agent_name(args[0]);
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

void CommandManager::bookmark(const std::vector<std::string> &args) {
    if (args.size() > 1 && (args[0] == "-r" || args[0] == "--remove")) {
        auto split_alias = std::vector(args.begin() + 1, args.end());
        if (const std::string alias = join(split_alias, ' '); bookmark_manager->is_bookmark(alias)) {
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
    } else if (args.size() > 1 && is_range_token(args[0])) {
        std::vector<std::string> index;
        int first = 0;
        int last = 9999;
        int alias_begin_index = 0;
        if (args.size() > 1) {
            const std::string &range = args[0];
            const auto dash = range.find('-');

            if (dash != std::string::npos) {
                alias_begin_index = 1;

                std::string left  = range.substr(0, dash);
                std::string right = range.substr(dash + 1);

                try {
                    if (!left.empty())
                        first = std::stoul(left);

                    if (!right.empty())
                        last = std::stoul(right);
                } catch (const std::exception &) {
                    std::cerr << "Error: invalid range format\n";
                    return;
                }
            }
        }

        auto split_alias = std::vector(args.begin() + alias_begin_index, args.end());
        const std::string alias = join(split_alias, ' ');

        if (bookmark_manager->is_bookmark(alias) || command_map.find(alias) != command_map.end()) {
            std::cerr << "Error: Alias already in use.\n";
            return;
        }

        bookmark_manager->bookmark(first, last, alias);
    } else {
        std::cerr << "Error: Invalid bookmark command format. Try bookmark --help.\n";
    }
}

void CommandManager::markdown(const std::vector<std::string> &args){
    if(args.size() > 1 && (args[0] == "-c" || args[0] == "--create")){
		auto split_alias = std::vector(args.begin() + 1, args.end());
		const std::string alias = join(split_alias, ' ');
		if(!bookmark_manager->is_bookmark(alias)){
			std::cerr << "Error: Invalid markdown command format. bookmark: " + alias + " not found.\n";
			return;
		}

		const std::pair<std::string, std::string> bookmark = bookmark_manager->get_bookmark(alias);
		const std::string llm_query = R"DELIM(
			Analyze the provided history of my conversation with you and the commands I entered into the Linux terminal to solve my problem.

			Based on the information provided, create a full Markdown (.md) document containing:

			1. A clear description of the problem I was trying to solve
			2. A step-by-step solution with detailed explanations of each step
			3. All necessary commands with comments on what they do
			4. Warnings about possible risks or side effects (if applicable)
			5. Alternative approaches to the solution (if any)
			6. Recommendations for preventing similar problems in the future

			The document format should be as readable as possible, with the correct heading structure, tables, and code formatting.
			)DELIM" "\n" + bookmark.second;

        std::string summary_md = bookmark_manager->agency_manager->execute_query(bookmark_manager->agency_manager->get_agency_url() + "/assistant", llm_query, false);
        markdown_manager->markdown(std::make_pair(alias, summary_md));
    } else if(args.size() > 0 && (args[0] == "-l" || args[0] == "--list")){
        markdown_manager->list_markdowns();
    } else if(args.size() > 1 && (args[0] == "-r" || args[0] == "--remove")){
        auto split_alias = std::vector(args.begin() + 1, args.end());
        if (const std::string alias = join(split_alias, ' '); markdown_manager->is_markdown(alias)) {
            markdown_manager->remove_markdown(alias);
        }
    } else if(args.size() > 1 && (args[0] == "-s" || args[0] == "--save")){
        auto split_alias = std::vector(args.begin() + 1, args.end());
        if (const std::string alias = join(split_alias, ' '); markdown_manager->is_markdown(alias)) {
            markdown_manager->save_as_file(alias);
        }
    } else if(args.size() > 1 && (args[0] == "-a" || args[0] == "--add")){
        auto group_start = std::find(args.begin(), args.end(), std::string("-g"));
        if(group_start == args.end()){
            group_start = std::find(args.begin(), args.end(), std::string("--group"));
            if(group_start == args.end()){
                std::cerr << "Error: Invalid markdown command format. Group to add not found.\n";
                return;
            }
        }
        auto split_alias = std::vector(args.begin() + 1, group_start);
        auto split_group = std::vector(group_start + 1, args.end());
        const std::string alias = join(split_alias, ' ');
        if (!markdown_manager->is_markdown(alias)) {
            std::cerr << "Error: Invalid markdown command format. Markdown for " + alias + " not found.\n";
            return;
        }
        markdown_manager->add_to_group(join(split_group, ' '), alias);
    } else if(args.size() > 1 && args[0] == "-gpt"){
        auto split_group = std::vector(args.begin() + 1, args.end());
        std::vector<std::string> markdown_list = markdown_manager->get_by_group(join(split_group, ' '));
        bookmark_manager->agency_manager->request_wrapper->context = markdown_list;
        std::cout << "group: " + join(split_group, ' ') + " was adeed to agent context\n";
    } else if(args.size() > 0){
        auto split_alias = std::vector(args.begin(), args.end());
        const std::string alias = join(split_alias, ' ');
        if(!markdown_manager->is_markdown(alias)){
            std::cerr << "Error: Invalid markdown command format. Markdown " + alias + " not found.\n";
            return;
        }
        auto md = markdown_manager->get_markdown(alias);
        std::cout << md.second << "\n";
    } else{
        std::cerr << "Error: Invalid markdown command format.\n";
    }
}
