#pragma once

#include "board_coordinates.hh"
#include "observers.hh"
#include "utils.hh"

/** Interface to implement to receive events from the display */
class BoardControl {
protected:
    BoardControl(const BoardControl&)            = default;
    BoardControl(BoardControl&&)                 = default;
    BoardControl& operator=(const BoardControl&) = default;
    BoardControl& operator=(BoardControl&&)      = default;

public:
    BoardControl() = default;

    /** Inform that the player chose to fire on this cell.
    * Return true if the action is valid (this cell was not targeted previously). */
    virtual bool fire(const BoardCoordinates, client_data& client, client_data& client2, 
        vector<string>& players_moves, const bool is_player1, Observers& observers) = 0;

    /** Inform that the player quit the game. */
    virtual void quit()                 = 0;

    // Make destructor virtual
    virtual ~BoardControl() = default;
};
