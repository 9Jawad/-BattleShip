#pragma once

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "board_coordinates.hh"

/** The board data seen by the display.
 *  This should provide (known) information about both sides of the board, and whose turn
 *  is.
 *
 *  This is an interface: it is a contract given by the display component that determines
 *  the information it wants to have about the board. */
class BoardView {
protected:
    BoardView(const BoardView&)            = default;
    BoardView(BoardView&&)                 = default;
    BoardView& operator=(const BoardView&) = default;
    BoardView& operator=(BoardView&&)      = default;

public:
    /** The type of a board cell */
    enum CellType : uint8_t {
        // Flags:
        IS_SHIP  = 0b001,
        IS_KNOWN = 0b010,                           //< was a target
        IS_SUNK  = 0b100,

        // Non-ship types:
        WATER = 0,                                  //< water (my side) or unknown (assumed water, their side)
        OCEAN = IS_KNOWN,                           //< was empty target

        // Ship states:
        UNDAMAGED = IS_SHIP,                        //< undamaged ship, used for my side
        HIT       = IS_SHIP | IS_KNOWN,             //< hit ship
        SUNK      = IS_SHIP | IS_KNOWN | IS_SUNK,   //< sunk ship
    };

    /** Given two ship states, return the best one */
    static constexpr inline CellType best(const CellType lhs, const CellType rhs) {
        if (!(lhs & IS_SHIP) || !(rhs & IS_SHIP)) {
            cerr << "BoardView::best(" << static_cast<unsigned>(lhs) << ", "
                      << static_cast<unsigned>(rhs) << ")" << endl;
            throw logic_error("BoardView::best called with non-ship types");
        }
        return lhs <= rhs ? lhs : rhs;
    }

    BoardView() = default;

    /** Should I play or wait for the other player to play? */
    [[nodiscard]] virtual bool     myTurn() const                            = 0;
    /** Is the game finished? */
    [[nodiscard]] virtual bool     isFinished() const                        = 0;
    /** Am I the winner? */
    [[nodiscard]] virtual bool     isVictory() const                         = 0;
    /** Valid BoardCoordinates' x are: 0 <= x < width() */
    [[nodiscard]] virtual size_t   width() const                             = 0;
    /** Valid BoardCoordinates' y are: 0 <= y < height() */
    [[nodiscard]] virtual size_t   height() const                            = 0;
    /** Must return the known cell type */
    [[nodiscard]] virtual CellType cellType(bool             my_side,
                                            BoardCoordinates position) const = 0;

    // Make destructor virtual
    virtual ~BoardView() = default;
};
