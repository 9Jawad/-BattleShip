#pragma once

#include <algorithm>
#include <cmath>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>
#include "board_lite.hh"
#include "../server/not_implemented_error.hh"

struct board_status{
    vector<vector<pair<int, int>>> boats;
    bool new_ship = true;
    bool playing = false;
};

/** GridDisplay using text.
 *
 * A grid is a side of the board as represented on the screen.
 * Draw both sides of the board as two grids side by side. */
class GridDisplay{
    ostream&               _out;      //< Where to print
    shared_ptr<BoardLite>  _board;    //< What to print

    uint8_t _letter_width;  //< Number of character in a column name
    uint8_t _number_width;  //< Number of character in a row name

    string const   _gap;         //< The string used as grid separator
    size_t    _grid_width;  //< The number of character in a line of a grid
    size_t    _width;    //< The number of character in a line with two grids and a gap
    vector<string> _map_key;  //< The map key, each string is a line without the ending '\n'
    board_status _board_data_1, _board_data_2;

    // Utility methods

    /** Count number of unicode characters in a UTF-8 encoded string (assume linux platform)
     */
    static constexpr size_t length(const string& s) {
        // In UTF-8, continuation bytes begin with 0b10, so, does not count these bytes.
        return static_cast<size_t>(
            ranges::count_if(s, [](char c) noexcept { return (c & '\xC0') != '\x80'; }));
    }

    // Pre-print methods

    /** Single character to represent a type of cell on the board */
    static string toString(const BoardLite::CellType type) { 
        switch (type) {
            case BoardLite::WATER:
                return " ";
            case BoardLite::OCEAN:
                return "╳";
            case BoardLite::UNDAMAGED:
                return "█";
            case BoardLite::HIT:
                return "▒";
            case BoardLite::SUNK:
                return "░";
            default:
                throw NotImplementedError("GridDisplay unknown CellType");
        }
    }

    /** Create a header indicating who should play */
    [[nodiscard]] string create_header() const;

    /** Create a grid label: Your / Their Fleet */
    [[nodiscard]] string create_grid_label(const bool my_side) const;

    /** Create grid for a given side, each string is a line without ending '\n' */
    [[nodiscard]] vector<string> create_grid(const bool my_side) const;

    /** Create map key, each string is a line without ending '\n'
     * Used by the constructor to init _map_key.
     * Outside constructor, use the attribute instead. */
    [[nodiscard]] static vector<string> create_map_key() {
        vector<string> map_key;
        map_key.emplace_back(" > " + toString(BoardLite::OCEAN) + " Ocean          <");
        map_key.emplace_back(" > " + toString(BoardLite::UNDAMAGED) + " Undamaged ship <");
        map_key.emplace_back(" > " + toString(BoardLite::HIT) + " Hit ship       <");
        map_key.emplace_back(" > " + toString(BoardLite::SUNK) + " Sunk ship      <");
        return map_key;
    }

    /** Create coordinates prompt, each string is a line without ending '\n'.
     * Empty lines are added in the beginning of the prompt so it can be printed next to the
     * map key. */
    [[nodiscard]] vector<string> create_prompt() const;

    // Print methods

    /** Print a vector of lines adding '\n' at the end of each line */
    void print(const vector<string>& lines);

    /** Print both parts side by side with _gap in between.
     * left and right are vector of lines without ending '\n'.
     * Both parts are fully printed, even if one part is shorter than the other.
     * If the left-hand part has lines of varying sizes, padding is added to ensure that the
     * right-hand part is properly aligned. Moreover, padding is also added so that the left
     * part is at least as wide as a grid.
     * The last line is printed without final '\n'. */
    void print_side_by_side(const vector<string> left, const vector<string> right);

public:
    GridDisplay() : _out{cout}, _letter_width{0},_number_width{0},_grid_width{0}, _width{0} {}

    /** @param out: where to print
     *  @param board: what to print, must inherit from BoardLite
     *
     *  WARNING: GridDisplay does not support a change in board->height() or
     *  board->width() after construction. */
    GridDisplay(ostream& out, shared_ptr<BoardLite> board)
            : _out{out},
              _board{move(board)},
              _letter_width{static_cast<uint8_t>(length(
                      BoardCoordinates(_board->width() - 1, _board->height() - 1).xToString()))},
              _number_width{static_cast<uint8_t>(length(
                      BoardCoordinates(_board->width() - 1, _board->height() - 1).yToString()))},
              _gap{"   "},
              _grid_width{_number_width + 1 + (1 + _letter_width) * _board->width() + 1},
              _width{_grid_width * 2 + _gap.size()},
              _map_key{create_map_key()} {}

    // Produces a redraw
    void update() ;

    

    // Get information about boats
    vector<vector<pair<int, int>>>& get_boats(){return _board_data_1.boats;}
    board_status* get_board_data(){return &_board_data_1;}

    vector<vector<pair<int, int>>>& get_enemy_boats(){return _board_data_2.boats;}
    board_status* get_enemy_board_data(){return &_board_data_2;}

    // Place boats on board
    void place_last_boat(const bool myside = true);

    // Update the boards' cells
    bool receive_fire(const int x, const int y, const string cell_type);
    bool send_fire(const int x, const int y, const string cell_type);

    void reset(){
        _board->reset();
    }
};
