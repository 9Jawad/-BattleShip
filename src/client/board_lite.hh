#pragma once

#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <utility>
#include <vector>
#include "../server/boat.hh"
#include "../common/common.hh"

using namespace std;

// A lite version of BoardGeneral with the only purpose of displaying the board
class BoardLite {
public:
    enum CellType : uint8_t {
        // Flags:
        IS_SHIP  = 0b001,
        IS_KNOWN = 0b010,  //< was a target
        IS_SUNK  = 0b100,

        // Non-ship types:
        WATER = 0,         //< water (my side) or unknown (assumed water, their side)
        OCEAN = IS_KNOWN,  //< was empty target

        // Ship states:
        UNDAMAGED = IS_SHIP,                       //< undamaged ship, used for my side
        HIT       = IS_SHIP | IS_KNOWN,            //< hit ship
        SUNK      = IS_SHIP | IS_KNOWN | IS_SUNK,  //< sunk ship
    };

    /** Given two ship states, return the best one */
    static constexpr inline CellType best(CellType lhs, CellType rhs) {
        if (!(lhs & IS_SHIP) || !(rhs & IS_SHIP)) {
            cerr << "BoardLite::best(" << static_cast<unsigned>(lhs) << ", "
                << static_cast<unsigned>(rhs) << ")" << endl;
            throw logic_error("BoardLite::best called with non-ship types");
        }
        return lhs <= rhs ? lhs : rhs;
    }

    /** The cell type and an optional ship identifier */
    class Cell {
        CellType _type{WATER};

    public:
        /** Default constructor: creates an unkown/Water cell */
        constexpr Cell() = default;

        constexpr Cell(CellType type): _type{type}{}

        [[nodiscard]] constexpr inline CellType

        type() const { return _type; }
    };

private:
    //vector containing my side
    vector <vector<Cell>> _my_side{
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
    };
    //Vector containing cells that we already shooted at
    vector <vector<Cell>> _their_hidden_side{
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
    };

    [[nodiscard]] Cell get(const bool my_side, const BoardCoordinates position) const {
        if (my_side) {
            return _my_side.at(position.y()).at(position.x());
        } else {
            return _their_hidden_side.at(position.y()).at(position.x());
        }
    }

    [[nodiscard]] bool check() const {
        if (_my_side.size() != _their_hidden_side.size()) {
            return false;
        }
        for (size_t i = 0; i < height(); ++i) {
            if (_my_side.at(i).size() != width() || _their_hidden_side.at(i).size() != width()) {
                return false;
            }
        }
        return true;
    }

public:
    BoardLite() {
        if (!check()) {
            cerr << "The hard-coded DummyBoard is not valid" << endl;
            abort();
        }
    }

    BoardLite(const BoardLite &) = delete;

    BoardLite(BoardLite &&) = delete;

    BoardLite &operator=(const BoardLite &) = delete;

    BoardLite &operator=(BoardLite &&) = delete;

    vector <vector<Cell>> get_my_side() { return _my_side; }

    [[nodiscard]] size_t width() const  { return _my_side.at(0).size(); }

    [[nodiscard]] size_t height() const  { return _my_side.size(); }

    [[nodiscard]] CellType cellType(const bool my_side,const BoardCoordinates position) const {
        return get(my_side, position).type();}

    void place_boat(const vector<pair<int, int>> &boat, const bool my_side = true){
        if (my_side){
            for (size_t i = 0; i < boat.size(); i++){
                _my_side.at(boat.at(i).first).at(boat.at(i).second) = Cell{UNDAMAGED};
            }
        }
        else{
            for (size_t i = 0; i < boat.size(); i++){
                _their_hidden_side.at(boat.at(i).first).at(boat.at(i).second) = Cell{UNDAMAGED};
            }
        }
    }

    void set_side(const bool myside,const int x, const int y, const string cell_type, bool &sunk){
        Cell type;
        int cell = stoi(cell_type);
        switch (cell){
            case 0: // WATER
                type = Cell{OCEAN};
                break;
            case 1: // OCEAN
                break;
            case 2: // UNDAMAGED
                type = Cell{HIT};
                break;
            case 3: // HIT
                break;
            case 4: // SUNK
                type = Cell{SUNK};
                sunk = true;
                break;
            default:
                throw NotImplementedError("GridDisplay unknown CellType");
        }
        if (myside) _my_side.at(x).at(y) = type;
        else{ _their_hidden_side.at(x).at(y) = type;}
    }

    void reset(){
        for (size_t i=0; i< _my_side.size(); i++){
            for (size_t j=0; j< _my_side.at(i).size(); j++){
                _my_side.at(i).at(j) = Cell{};
                _their_hidden_side.at(i).at(j) = Cell{};
            }
        }
    }

    ~BoardLite()  = default;
};
