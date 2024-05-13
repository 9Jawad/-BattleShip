#pragma once

#include <algorithm>
#include <cmath>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>
#include <unistd.h>
#include "board_control.hh"
#include "board_input.hh"
#include "board_view.hh"
#include "not_implemented_error.hh"
#include "utils.hh"
#include "timer.hh"



/** BoardInput using text.
 *
 *  A grid is a side of the board as represented on the screen.
 *  Draw both sides of the board as two grids side by side. 
 */
class ConsoleBoardInput final : public BoardInput {
    /** ConsoleBoardInput's methods must check they support this */
    shared_ptr<BoardView const> const _board;    //< What to print
    shared_ptr<BoardControl> const    _control;  //< Who to inform of user actions

    // Utility methods

    /** Count number of unicode characters in a UTF-8 encoded string (assume linux platform)
    */
    static constexpr size_t length(const string& s) {
        // In UTF-8, continuation bytes begin with 0b10, so, does not count these bytes.
        return static_cast<size_t>(
            ranges::count_if(s, [](char c) noexcept { return (c & '\xC0') != '\x80'; }));
    }

private:
    void update_observers(vector<int> &observers);

public:
    ConsoleBoardInput() = delete;

    /** @param board: what to print, must inherit from BoardView
    *   @param control: who to inform of user actions.
    *
    *   WARNING: ConsoleBoardInput does not support a change in board->height() or
    *   board->width() after construction. 
    */
    ConsoleBoardInput(shared_ptr<BoardView const> board,
                        shared_ptr<BoardControl>    control)
        : _board{move(board)},
          _control{move(control)} {}

    ConsoleBoardInput(const ConsoleBoardInput&)                  = default;
    ConsoleBoardInput(ConsoleBoardInput&&)                       = default;
    ConsoleBoardInput& operator=(const ConsoleBoardInput& other) = delete;
    ConsoleBoardInput& operator=(ConsoleBoardInput&&)            = delete;

    /** Produces a redraw */
    void update(client_data& client, const bool send=true) override;

    /** Parse coordinates provided by user, check boundaries and call
    * BoardControl::fire. */
    bool handle_input_server(client_data &client, client_data &client2,
                             vector<string> &players_moves, const bool is_player1, Timer& timer, Observers& observers) override;

    bool check_timer_turn(const chrono::steady_clock::time_point begin,
                     unsigned int &time_lapse, const Timer timer, client_data &client);

    ~ConsoleBoardInput() override = default;
};
