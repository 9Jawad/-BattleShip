#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include "grid_display.hh"

string operator*(const string &lhs, const size_t rhs) {
    string result;
    result.reserve(lhs.size() * rhs);
    for (size_t i = 0; i < rhs; ++i) {
        result += lhs;
    }
    return result;
}

string GridDisplay::create_header() const {
    //                   ╔════════════╗
    //                   ║ Your  Turn ║
    //                   ╚════════════╝

    // 2de line:
    string who = "Your ";
    string turn = "║ " + who + " Turn ║";

    // margin:
    size_t margin_size = length(turn) > _width ? 0 : (_width - length(turn)) / 2;
    string margin(margin_size, ' ');

    // 1st and 3rd line:
    ostringstream oss;
    oss << (string("═") * (length(turn) - 2));
    string line = oss.str();
    oss.str(""); // clear oss

    // Result:
    oss << margin << "╔" << line << "╗\n"
        << margin << turn << '\n'
        << margin << "╚" << line << "╝\n\n";
    return oss.str();
}

string GridDisplay::create_grid_label(const bool my_side) const {
    string your = "Your  Fleet";
    string their = "Their Fleet";
    size_t label_size = max(length(your), length(their));
    size_t margin_size = label_size > _grid_width ? 0 : (_grid_width - label_size) / 2;
    string margin(margin_size, ' ');
    return margin + (my_side ? your : their);
}

vector<string> GridDisplay::create_grid(const bool my_side) const {
    vector<string> grid;
    ostringstream oss("    ", ios_base::ate);

    // letters
    for (size_t i = 0; i < _board->width(); ++i) {
        oss << setw(_letter_width) << BoardCoordinates{i, 0}.xToString() << ' ';
    }
    grid.emplace_back(oss.str());

    // first line
    oss.str("   ┌");
    oss << (((string("─") * _letter_width) + "┬") * (_board->width() - 1));
    oss << "─┐";
    grid.emplace_back(oss.str());

    // body
    for (unsigned i = 0; i < _board->height(); ++i) {
        oss.str("");
        oss << setw(_number_width) << i + 1 << " ";
        for (unsigned j = 0; j < _board->width(); ++j) {
            string border = "│";
            BoardLite::CellType content = _board->cellType(my_side, {j, i});
            oss << border << toString(content);
        }
        oss << "│";
        grid.emplace_back(oss.str());
    } 

    // last line
    oss.str("   └");
    oss << (((string("─") * _letter_width) + "┴") * (_board->width() - 1));
    oss << "─┘";
    grid.emplace_back(oss.str());

    return grid;
}

vector<string> GridDisplay::create_prompt() const {
    vector<string> prompt(_map_key.size() - 2, ""); // Add padding
    prompt.emplace_back(">> SELECT TARGET <<");
    prompt.emplace_back(">> ");
    return prompt;
}

void GridDisplay::print_side_by_side(const vector<string> left, const vector<string> right) {
    size_t left_width = max(_grid_width, ranges::max(left, {}, 
        [](const string &s) noexcept { return length(s);}).size());
    size_t idx{0};
    size_t last_line = max(left.size(), right.size());
    string space(left_width, ' ');
    for (idx = 0; idx < last_line; ++idx) {
        // Left
        if (idx < left.size()) {
            _out << std::left << left.at(idx);
            if (length(left.at(idx)) < left_width) {
                _out << string(left_width - length(left.at(idx)), ' ');
            }
        } else {
            _out << space;
        }
        // Right (and gap)
        if (idx < right.size()) {
            _out << _gap << right.at(idx);
        }
        // New line
        if (idx < last_line - 1) {
            _out << '\n';
        }
    }
}

bool GridDisplay::send_fire(const int x, const int y, const string cell_type) {
    bool sunk = false;
    _board->set_side(false, x, y, cell_type, sunk);
    return sunk;
}

bool GridDisplay::receive_fire(const int x, const int y, const string cell_type) {
    bool sunk = false;
    _board->set_side(true, x, y, cell_type, sunk);
    return sunk;
}
void GridDisplay::print(const vector<string> &lines) {
    for (const string &line : lines) {
        _out << line << '\n';
    }
}

void GridDisplay::update() {
    system("clear");  // Do not use system in other contexts
    _out << create_header();
    print_side_by_side({create_grid_label(true)}, {create_grid_label(false)});
    _out << '\n';
    print_side_by_side(create_grid(true), create_grid(false));
    _out << '\n';
    // if (_board->myTurn()) {
    print_side_by_side(create_map_key(), create_prompt());
    // } else {
    //   print(create_map_key());
    // }
    _out << flush;
}

void GridDisplay::place_last_boat(const bool myside) {
    if (myside)
        _board->place_boat(_board_data_1.boats.at(_board_data_1.boats.size()-1)); 
    else
        _board->place_boat(_board_data_2.boats.at(_board_data_2.boats.size()-1), false); 
}
