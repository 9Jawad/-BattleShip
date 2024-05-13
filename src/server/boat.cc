#include "boat.hh"

Boat::Boat(int boat_size,
           int boat_id,
           bool boat_orientation,
           pair<int, int> first_cell) :
        _boat_size(boat_size),
        _boat_id(boat_id),
        _boat_orientation(boat_orientation),
        _first_cell(first_cell) {
    _boat_position = boat_creation();
}

Boat::Boat(int boat_size,
           int boat_id,
           vector<pair<int,int>> _boat_position) :
        _boat_size(boat_size),
        _boat_id(boat_id),
        _boat_position(_boat_position){

}

vector <pair<int, int>> Boat::boat_creation() {
    vector <pair<int, int>> boat_position;
    if (_boat_orientation == 0) {
        for (int i = 0; i < _boat_size; i++) {
            boat_position.push_back(make_pair(_first_cell.first + i, _first_cell.second));
        }
    } else {
        for (int i = 0; i < _boat_size; i++) {
            boat_position.push_back(make_pair(_first_cell.first, _first_cell.second + i));
        }
    }
    return boat_position;
}