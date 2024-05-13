#ifndef BOAT_HH
#define BOAT_HH

#include <vector>
#include "board_view.hh"

class Boat  {
    int _boat_size;
    int _boat_id;
    bool _boat_orientation=0; // 0 = horizontal, 1 = vertical
    bool  _is_sunk = false;
    pair<int, int> _first_cell={0,0};
    vector<pair<int,int>> _boat_position;

public : 
    Boat(int boat_size, int boat_id, vector<pair<int,int>> boat_position);
    Boat(int boat_size, int boat_id, bool boat_orientation, pair<int, int> first_cell);

    vector<pair<int,int>> boat_creation();
    void set_is_sunk(){_is_sunk = true;}

    [[nodiscard]] vector<pair<int,int>> get_boat_positions() const {return _boat_position;}
    [[nodiscard]] bool get_is_sunk() {return _is_sunk;}
    [[nodiscard]] int get_boat_id() const {return _boat_id;}
    [[nodiscard]] int get_boat_size()const {return _boat_size;}
    [[nodiscard]] bool get_boat_orientation()const {return _boat_orientation;}
};

#endif //!BOAT_HH
