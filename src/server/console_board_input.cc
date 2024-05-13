#include <algorithm>
#include <cctype>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include "../server/network.hh"
#include "../server/not_implemented_error.hh"
#include "console_board_input.hh"
#include "utils.hh"

string operator*(const string& lhs, size_t rhs) {
    string result{};
    result.reserve(lhs.size() * rhs);
    for (size_t i = 0; i < rhs; ++i) {
        result += lhs;
    }
    return result;
}

bool ConsoleBoardInput::handle_input_server(client_data &client, client_data &client2,
        vector<string> &players_moves, const bool is_player1, Timer& timer, Observers& observers) {
    // Gets the input from the client and processes it
    // Then sends the result to both clients
    if (!_board->myTurn()) {
        throw NotImplementedError("handle_input_server() when not my turn.");
    }


    for (bool fired = false; !fired; update(client, false)) {
        BoardCoordinates coordinates{_board->width(), _board->height()};

        // Asks the client to choose a coord
        send_prnt_token("It's your turn, choose a coordinate to fire at :", client.client_sock);

        timer.start_timer_turn();

        // Wait for the client to play a new move
        while (!client.has_new_move) {
            if(timer.check_timer_game(client)){
                client.has_new_move = false;
                timer.end_timer_turn(client);
                return false;
            }
            // Check if player timed out
            if (timer.check_timer_turn(client)) {
                client.has_new_move = false;
                timer.end_timer_turn(client);
                return false;
            }
        }

        client.has_new_move = false;

        // Get the new move and use it to instantiate a stringstream
        istringstream ss(client.client_actions);
        ss >> coordinates;

        if (!(coordinates.x() < _board->width() && coordinates.y() < _board->height())) {
            send_prnt_token("Position out of range", client.client_sock);
            continue;
        }

        fired = _control->fire(coordinates, client, client2, players_moves,
                            is_player1, observers);
    }
    // If move valid, updates board for the other player

    observers.update_observers();
    update(client2, false);
    timer.end_timer_turn(client);
    return true;
}


void ConsoleBoardInput::update(client_data &client, const bool send) {
    if (send) {
        if (client.played_move[0] != '\0') {
            send_played_move(client);
        }
        bzero(client.played_move, BUFFER_SIZE);
    } else {
        // Tells the client to update the board
        send_message(TOKEN_UPDATE_BOARDS, client.client_sock);
    }
}
