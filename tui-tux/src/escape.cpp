#include <string>
#include <regex>

#include "../include/screen.hpp"

void escape(std::string &seq, Screen &screen) {
    std::regex clear_regex("^\\[J$");
    std::regex home_regex("^\\[H$");
    std::regex cursor_up_regex("^\\[A$");
    std::regex cursor_down_regex("^\\[B$");
    std::regex cursor_forward_regex("^\\[C$");
    std::regex cursor_back_regex("^\\[H$");
    std::regex erase_in_place_regex("^\\[1P$");
    std::regex erase_to_eol_regex("^\\[K$");
    std::regex move_regex("^\\[(\\d+);(\\d+)H$");
    std::regex vertical_regex("^\\[(\\d+)d$");
    
    std::regex back_rel_regex("^\\[(\\d+)D$");
    std::regex front_rel_regex("^\\[(\\d+)C$");
    std::regex up_rel_regex("^\\[(\\d+)A$");
    std::regex down_rel_regex("^\\[(\\d+)B$");

    std::regex scroll_up_regex("^M$");

    std::regex insert_char_regex("^\\[(\\d+)@$");
    std::regex insert_char1_regex("^\\[@$");

    std::smatch matches;

    if (std::regex_match(seq, clear_regex)) {
        screen.clear();
    } else if (std::regex_match(seq, home_regex)) {
        screen.cursor_begin();
    } else if (std::regex_match(seq, cursor_up_regex)) {
        screen.cursor_up();
    } else if (std::regex_match(seq, cursor_down_regex)) {
        screen.cursor_down();
    } else if (std::regex_match(seq, cursor_forward_regex)) {
        screen.cursor_forward();
    } else if (std::regex_match(seq, cursor_back_regex)) {
        screen.cursor_back();
    } else if (std::regex_match(seq, erase_in_place_regex)) {
        screen.erase_in_place();
    } else if (std::regex_match(seq, erase_to_eol_regex)) {
        screen.erase_to_eol();
    } else if (std::regex_match(seq, matches, move_regex)) {
        screen.move_cursor(std::stoi(matches[1].str()) - 1, std::stoi(matches[2].str()) - 1);
    } else if (std::regex_match(seq, matches, vertical_regex)) {
        screen.move_cursor(std::stoi(matches[1].str()) - 1, screen.get_x());
    } else if (std::regex_match(seq, matches, back_rel_regex)) {
        screen.move_cursor(screen.get_y(), screen.get_x() - std::stoi(matches[1].str()));
    } else if (std::regex_match(seq, matches, front_rel_regex)) {
        screen.move_cursor(screen.get_y(), screen.get_x() + std::stoi(matches[1].str()));
    } else if (std::regex_match(seq, matches, up_rel_regex)) {
        screen.move_cursor(screen.get_y() - std::stoi(matches[1].str()), screen.get_x());
    } else if (std::regex_match(seq, matches, down_rel_regex)) {
        screen.move_cursor(screen.get_y() + std::stoi(matches[1].str()), screen.get_x());
    } else if (std::regex_match(seq, scroll_up_regex)) {
        screen.scroll_up();
    } else if (std::regex_match(seq, matches, insert_char_regex)) {
        screen.insert_next(std::stoi(matches[1].str()));
    } else if (std::regex_match(seq, insert_char1_regex)) {
        screen.insert_next(1);
    }
}