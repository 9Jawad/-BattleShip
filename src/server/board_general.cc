#include "board_general.hh"

void Board::parse_action_tokens(client_data& client, client_data& client2, const string x, const string y, const CellType cell_type){
    // Add coordinates to clients' tokens
    add_token(client.tokens, y);
    add_token(client.tokens, x);
    add_token(client2.tokens,y);
    add_token(client2.tokens, x);
    // Parse cellType and add it to clients' tokens
    parse_cell(client.tokens, cell_type);
    parse_cell(client2.tokens, cell_type);
}

void Board::send_fire_observers(const bool is_player1, const string x, const string y, const BoardView::CellType cell_type,
                                Observers& observers, vector<string>& players_moves){
    string observer_token{};
    is_player1 ? observer_token = TOKEN_SEND_FIRE : observer_token = TOKEN_RECV_FIRE;
    add_token(observer_token, y);
    add_token(observer_token, x);
    parse_cell(observer_token, cell_type);
    players_moves.push_back(observer_token);

    observers.send_message_to_observers(observer_token);
}

void Board::send_fire_tokens(client_data& client, client_data& client2, const string x, const string y, const CellType cell_type,
     const bool is_player1, vector<string>& players_moves, Observers& observers){
    parse_action_tokens(client, client2, x, y, cell_type);
    send_message(client.tokens, client.client_sock);
    send_message(client2.tokens, client2.client_sock);
    send_fire_observers(is_player1, x, y, cell_type, observers, players_moves);
}

/**
* @brief Change the cell type of the cell at the given position on the enemy's side upon firing a hit
* @param position : the position of the cell to change
*/
void Board::send_fire(const BoardCoordinates position, client_data& client, client_data& client2,
                     vector<string>& players_moves, const bool is_player1, Observers& observers) {
    Cell fired = _their_hidden_side.at(position.y()).at(position.x());
    client.tokens = TOKEN_SEND_FIRE;
    client2.tokens = TOKEN_RECV_FIRE;
    if (fired.type() == UNDAMAGED) {
        _their_hidden_side.at(position.y()).at(position.x()) = Cell(HIT, fired.shipId());
        int ship_id = optional_to_value(fired.shipId());
        Boat *boat = find_enemy_boat(ship_id);

        bool sunk = check_if_sunk(boat);
        if (sunk){
             vector<pair<int,int>> sunk_boat_positions = boat->get_boat_positions();
             for (size_t i = 0; i < sunk_boat_positions.size(); i++){
                send_fire_tokens(client, client2, to_string(sunk_boat_positions.at(i).first), 
                    to_string(sunk_boat_positions.at(i).second), SUNK, is_player1, players_moves, observers);
                client.tokens = TOKEN_SEND_FIRE;
                client2.tokens = TOKEN_RECV_FIRE;
            
            }
            send_fire_tokens(client, client2, to_string(sunk_boat_positions.at(0).first), 
                    to_string(sunk_boat_positions.at(0).second), SUNK, is_player1, players_moves, observers);

            check_victory();             
        }
        else{
            send_fire_tokens(client, client2, to_string(position.x()), 
                    to_string(position.y()), fired.type(), is_player1, players_moves, observers);
        }
    } else if (fired.type() == WATER) {
        _their_hidden_side.at(position.y()).at(position.x()) = Cell(OCEAN, nullopt);
        _my_turn = false;
        send_fire_tokens(client, client2, to_string(position.x()), 
                    to_string(position.y()), fired.type(), is_player1, players_moves, observers);
    }
} 


/**
 * @brief Change the cell type of the cell at the given position upon receiving a hit from the enemy
 * @param position : the position of the cell to change
*/
void Board::receive_fire(const BoardCoordinates position) {
    Cell fired = _my_side.at(position.y()).at(position.x());

    if (fired.type() == UNDAMAGED) {
        _my_side.at(position.y()).at(position.x()) = Cell(HIT, fired.shipId());
        int ship_id = optional_to_value(fired.shipId());
        Boat *boat = find_boat(ship_id);
        bool sunk = check_if_sunk(boat);
        if (sunk) check_victory();
    } else if (fired.type() == WATER) {
        _my_side.at(position.y()).at(position.x()) = Cell(OCEAN, nullopt);
    }    
}

bool Board::pos_okay(Boat *boat, client_data& client) {
    if (!boat_in_frame(boat, client) || !not_overlapping(boat, client) || !check_neighbours(boat, client))
        return false;
    return true;
}

bool Board::boat_in_frame(Boat *boat, client_data& client){
    pair<int, int> first_pos = boat->get_boat_positions().at(0);
    pair<int, int> last_pos = boat->get_boat_positions().back();
    bool first_ok = first_pos.first >= 0 && first_pos.first < static_cast<int>(width())
                    && first_pos.second >= 0 && first_pos.second < static_cast<int>(height());
    bool last_ok = last_pos.first >= 0 && last_pos.first < static_cast<int>(width())
                   && last_pos.second >= 0 && last_pos.second < static_cast<int>(height());

    if (!first_ok || !last_ok) {
        send_prnt_token("Invalid boat placement, boats cannot go out of frame. Try again.", client.client_sock);
        return false;
    }
    return true;
}

bool Board::not_overlapping(Boat *boat, client_data& client){
    for (auto pos: boat->get_boat_positions()) {
        for (auto old_boat: _boats) {
            for (auto old_pos: old_boat.get_boat_positions()) {
                if (pos == old_pos) {
                    send_prnt_token("Invalid boat placement, boats cannot overlap. Try again.", client.client_sock);
                    return false;
                }
            }
        }
    }
    return true;
}

bool Board::check_neighbours(Boat *boat, client_data& client) {
    for (auto pos : boat->get_boat_positions()) {
        vector<pair<int, int>> neighbours = {
                {pos.first - 1, pos.second - 1},
                {pos.first - 1, pos.second},
                {pos.first - 1, pos.second + 1},
                {pos.first, pos.second - 1},
                {pos.first, pos.second + 1},
                {pos.first + 1, pos.second - 1},
                {pos.first + 1, pos.second},
                {pos.first + 1, pos.second + 1}};

        for (auto neighbour : neighbours) {
            if (neighbour.first >= 0 && neighbour.first < static_cast<int>(width()) && neighbour.second >= 0 && neighbour.second < static_cast<int>(height())) {
                optional<int> current_shipID = _my_side.at(neighbour.second).at(neighbour.first).shipId();
                if (current_shipID != nullopt && optional_to_value(current_shipID) != boat->get_boat_id()) {
                    send_prnt_token("Invalid boat placement, boats cannot touch. Try again.", client.client_sock);
                    return false;
                }
            }
        }
    }
    return true;
}

void Board::place_enemy_boats(vector <Boat> boats) {
    _enemy_boats = boats;
    for (auto enemy_boat: _enemy_boats) {
        for (auto pos: enemy_boat.get_boat_positions()) {
            _their_hidden_side.at(pos.second).at(pos.first) = Cell(UNDAMAGED, enemy_boat.get_boat_id());
        }
    }
}

/**
  * @brief Add a boat to the board
  * @param boat : the vector containing the position of the cells of the boat
  * @param my_side : true if the boat is on the player's side, false if it is on the opponent's side
 */
void Board::add_boat(Boat *boat, const bool my_side) {
    vector <pair<int, int>> boat_pos = boat->get_boat_positions();
    if (my_side) {
        for (auto &pos: boat_pos) {
            _my_side.at(pos.second).at(pos.first) = Cell(UNDAMAGED, boat->get_boat_id());
        }
    } else {
        for (auto &pos: boat_pos) {
            _their_hidden_side.at(pos.second).at(pos.first) = Cell(UNDAMAGED, boat->get_boat_id());
        }
    }
    _boats.push_back(*boat);
}

void Board::send_boat_cell_tokens(client_data& client, const string boat_cell_x, const string boat_cell_y, const string player_turn, vector<string>& players_moves){
    // Register the move in the vector
    string player_move{TOKEN_ADD_BOAT_CELL};
    add_token(player_move, player_turn);
    add_token(player_move, boat_cell_x);
    add_token(player_move, boat_cell_y);
    players_moves.push_back(player_move);

    // Parse the boat cell token and send it to the client
    string token = TOKEN_ADD_BOAT_CELL;
    add_token(token, "1");
    add_token(token, boat_cell_x);
    add_token(token, boat_cell_y);
    send_message(token, client.client_sock);

}

void Board::send_boat_to_client(client_data& client, Boat *boat, vector<string>& players_moves, const bool is_player1){
    // Send every boat cell one by one to the client and repeat the first one to indicate
    // the end of the boat
    vector<pair<int, int>> boats_coord;
    boats_coord = boat->get_boat_positions();

    string player_turn{};
    is_player1 ? player_turn = "1" : player_turn = "2";

    client.tokens = "";
    for (size_t i=0; i < boats_coord.size(); i++) {
        send_boat_cell_tokens(client, to_string(boats_coord.at(i).first), to_string(boats_coord.at(i).second), player_turn, players_moves);
    }
    // Repeats the first coord to tell the client it's the end of this boat
    send_boat_cell_tokens(client, to_string(boats_coord.at(0).first), to_string(boats_coord.at(0).second), player_turn, players_moves);

}

bool Board::place_all_boats(shared_ptr <ConsoleBoardInput> display, client_data& client, vector<string>& players_moves, const bool is_player1, Timer &timer) {
    vector<int> boats_to_place = {2, 3, 3, 4, 5};
    while (!boats_to_place.empty()) {
        bool boat_okay = false;
        Boat *new_boat;
        timer.start_timer_turn();
        while (!boat_okay) {
            new_boat = input_placement(boats_to_place, timer, client);
            // Turn time has ran out
            if (!new_boat){
                timer.end_timer_turn(client);
                break;
            }
            boat_okay = pos_okay(new_boat, client);
        }
        // Boats placement succeeded
        if(boat_okay){
            // Stops turn timer after a valid placement
            timer.end_timer_turn(client);
            // Send a boat to the client
            add_boat(new_boat, true);
            send_boat_to_client(client, new_boat, players_moves, is_player1);
            display->update(client ,false);
        }
        boats_to_place.pop_back();
    }
    // Checks if at least one boat is placed
    if(_boats.size() >= 1) return true;
    return false;
}

Boat *Board::input_placement(const vector<int> boats_to_place, Timer &timer, client_data& client) {
    bool is_okay = false;
    bool orientation;
    string input{}, ori{};
    optional <size_t> x, y;
    int vector_size = boats_to_place.size();

    // Ask the client to enter coordinates
    send_prnt_token("Enter coordinates and orientation (size: "+to_string(boats_to_place.back())+") (e.g. A1h, D4v) : ", client.client_sock);
    while (!is_okay) {
        // Wait for a client to play a new move
        if (!client.has_new_move){
            if(timer.check_timer_turn(client, "Time's up ! You lost a ship :(\n")){
                return nullptr;
            }
            continue;
        }
        // Get the new move
        input = client.client_actions;
        client.has_new_move = false;
        
        // Extract first character for x coordinate
        x = BoardCoordinates::parseX(input.substr(0, 1));
        // Extract characters for y coordinate
        y = BoardCoordinates::parseY(input.substr(1, input.length() - 2));
        // Extract last character for orientation
        ori = input.substr(input.length() - 1);

        bool x_correct = x.has_value() && (x.value() < width());
        bool y_correct = y.has_value() && (y.value() < height());
        bool orientation_correct = (ori == "h" || ori == "v" || ori == "H" || ori == "V");

        if (x_correct && y_correct && orientation_correct) {
            is_okay = true;
        } else {
            // Tell the client it's an invalid input
            send_prnt_token("Invalid input format. Please enter coordinates in the format (e.g. A1h, D4v) (size: "+to_string(boats_to_place.back())+"): \n", client.client_sock);
        }
    }
    orientation = ((ori == "v") || (ori == "V")) ? 1 : 0;
    BoardCoordinates boardCoords(*x, *y);

    return new Boat(boats_to_place.back(), vector_size, orientation,
                    {boardCoords.x(), boardCoords.y()});
}

Boat *Board::find_boat(const int boat_id) {
    for (auto &boat: _boats) {
        if (boat.get_boat_id() == boat_id)return &boat;
    }
    return nullptr;
}

Boat *Board::find_enemy_boat(const int boat_id) {
    for (auto &boat: _enemy_boats) {
        if (boat.get_boat_id() == boat_id)return &boat;
    }
    return nullptr;
}

bool Board::check_if_sunk(Boat *boat) {
    if (boat->get_is_sunk()) return true;
    for (auto pos: boat->get_boat_positions()) {
        if (_their_hidden_side.at(pos.second).at(pos.first).type() != HIT) {
            return false;
        }
    }
    set_sunk_boat(boat);
    return true;
}

void Board::set_sunk_boat(Boat *boat) {
    boat->set_is_sunk();
    for (auto pos: boat->get_boat_positions()) {
        if (!myTurn()) {
            _my_side.at(pos.second).at(pos.first) = Cell(SUNK, boat->get_boat_id());
        } else {
            _their_hidden_side.at(pos.second).at(pos.first) = Cell(SUNK, boat->get_boat_id());
        }
    }
}

/**
  * @brief Check if the player has won
 */
void Board::check_victory() {
    bool victory = true;
    for (size_t i = 0; i < height(); ++i) {
        for (size_t j = 0; j < width(); ++j) {
            if (_their_hidden_side.at(i).at(j).type() == UNDAMAGED) {
                victory = false;
                break;
            }
        }
    }
    if (victory) {
        _is_finished = true;
        _is_victory = true;
    }
}
