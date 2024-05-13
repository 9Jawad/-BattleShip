#pragma once

#include "utils.hh"
#include "timer.hh"

/** Interface for Board Display */
class BoardInput {
protected:
    BoardInput(const BoardInput&)            = default;
    BoardInput(BoardInput&&)                 = default;
    BoardInput& operator=(const BoardInput&) = default;
    BoardInput& operator=(BoardInput&&)      = default;

public:
    BoardInput() = default;
    /** To call when the display is to be updated. Can be called after an
    * event or at regular intervals. */
    virtual void update(client_data &client, const bool send=true)      = 0;
    /** To call when the BoardControl is ready to receive an input from the view. The exact
    * meaning have to be precised in the concrete classes. */
    virtual bool handle_input_server(client_data &client, client_data &client2,
                                     vector<string> &players_moves, const bool is_player1, Timer& timer, Observers& observers) = 0;
    // Make destructor virtual
    virtual ~BoardInput() = default;
};
