#include "markdown_manager.hpp"
#include <iostream>
#include <iomanip>

MarkdownManager::MarkdownManager() = default;

MarkdownManager::~MarkdownManager() = default;

bool MarkdownManager::create_markdowns_file(const std::string& filename) {
    std::ofstream create_file(filename);
    if (!create_file) {
        std::cerr << "Error creating file: " << filename << "\n";
        return false;
    }
    const json empty_markdowns = json::array();
    create_file << empty_markdowns.dump(4);
    return true;
}

void MarkdownManager::parse_markdown_json(const json &markdown) {
    const auto bookmark = markdown.at("bookmark").get<std::string>();
    const auto group = markdown.at("group").get<std::string>();
    const auto protocol = markdown.at("protocol").get<std::string>();
    markdowns[bookmark] = {group, protocol};
}

void MarkdownManager::load_markdowns(const std::string &filename) {
    std::ifstream file(filename);
    if (!file) {
        if (!create_markdowns_file(filename)) {
            return;
        }
        return;
    }
    json markdown_json;
    file >> markdown_json;
    for (const json &markdown : markdown_json) {
        parse_markdown_json(markdown);
    }
}

void MarkdownManager::save_markdowns(const std::string &filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << "\n";
        return;
    }
    json markdown_json = json::array();
    for (const auto &markdown : markdowns) {
        json entry;
        entry["bookmark"] = markdown.first;
        entry["group"] = markdown.second.first;
        entry["protocol"] = markdown.second.second;
        markdown_json.push_back(entry); // Add entry to the array
    }
    file << markdown_json.dump(4);
}

bool MarkdownManager::is_markdown(const std::string& n) const {
    return markdowns.find(n) != markdowns.end();
}

std::pair<std::string, std::string> MarkdownManager::get_markdown(const std::string& n) const {
    auto it = markdowns.find(n);
    return it == markdowns.end() ? std::make_pair(std::string(), std::string()) : it->second;
}

/*---------------------------------------------------------------------------------------*/

void MarkdownManager::markdown(const std::pair<std::string, std::string>& protocol) {
    markdowns[protocol.first] = std::make_pair( "default", protocol.second);
    std::cout << "Markdown added: " << protocol.first << '\n';
}

void MarkdownManager::list_markdowns() const {
    constexpr int group_width = 20;
    constexpr int bookmark_width = 50;
    std::cout << std::left << std::setw(bookmark_width) << "MARKDOWN"
              << std::setw(group_width) << "GROUP" << "\n";
    for (const auto &[fst, snd] : markdowns) {
        std::cout << std::left << std::setw(bookmark_width) << fst
                  << std::setw(group_width) << snd.first
                  << "\n";
    }
}


void MarkdownManager::remove_markdown(const std::string &bookmark) {
    if (const auto it = markdowns.find(bookmark); it != markdowns.end()) {
        markdowns.erase(it);
        std::cout << "Removed markdown '" << bookmark << "'.\n";
    } else {
        std::cerr << "Error: Markdown '" << bookmark << "' not found.\n";

    }
}

void MarkdownManager::add_to_group(const std::string& group,
                                   const std::string& bookmark) {
    auto it = markdowns.find(bookmark);
    if (it == markdowns.end()) {
        std::cerr << "Error: Markdown '" << bookmark << "' not found.\n";
        return;
    }
    it->second.first = group;
    //save_markdowns(get_markdowns_file_path());
    std::cout << "Markdown '" << bookmark << "' moved to '" << group << "'.\n";
}

std::vector<std::string> MarkdownManager::get_by_group(const std::string& group) {
    std::vector<std::string> context;
    for (auto& [_, md] : markdowns){
        if (md.first == group) {
            context.push_back(md.second);
        }
    }
    return context;
}

bool MarkdownManager::save_as_file(const std::string& bookmark) {
    auto it = markdowns.find(bookmark);
    if (it == markdowns.end()) {
        std::cerr << "Error: Markdown '" << bookmark << "' not found.\n";
        return false;
    }
    std::ofstream f(bookmark + ".md");
    if (!f) {
        std::cerr << "Error creating file: " << bookmark << ".md\n";
        return false;
    }
    f << it->second.second << '\n';
    std::cout << "Saved markdown '" << bookmark << "' to " << bookmark << ".md\n";
    return true;
}
