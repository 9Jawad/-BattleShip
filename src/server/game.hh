#include <arpa/inet.h>
#include <complex>
#include <cstddef>
#include <iostream>
#include <memory>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utility>
#include "board_general.hh"
#include "network.hh"
#include "utils.hh"
#include "observers.hh"

// Manages a game with 2 players
class Game {

    pair<client_data, client_data> _players;
    vector<string> _player_1_moves, _player_2_moves; // vectors with all moves, even invalid ones.
    vector<string> _players_moves; //vector with valid moves only
    /*
    * 1st token : which player -> '1' or '2'
    * 2nd token : action -> 'B' or 'R' or 'S'
    * 3rd token : coord_x
    * 4th token : coord_y
    * 5th token : cell_type (0 -> 5)
    * */
    Observers _observers;
    string _gamemode;
    Timer _timer_1, _timer_2;
    shared_ptr<Board> _board_player_1, _board_player_2;
    pair<shared_ptr<ConsoleBoardInput>,
              shared_ptr<ConsoleBoardInput>> _displays;

    vector<int> _invited_players;
    vector<int> _invited_observers;

    int _id; // The id is equal to the value of the first client's socket
    bool _playing = false; // The game has started

    enum class WinConditions : int {
        sunk_all_boats = 0,
        time_out = 1,
        no_boats = 2
    };

public:
    Game(){}
    Game(string gamemode, unsigned int timer_turn, unsigned int timer_game, string username, int client_sock) 
        : _gamemode{gamemode}, _timer_1{timer_turn, timer_game}, _timer_2{timer_turn, timer_game} {
        _players.first.name = username;
        _players.first.client_sock = client_sock;
        bzero(_players.first.played_move, BUFFER_SIZE);

        _players.second.name = "Player2";
        // Same socket for both _players at the start to know when to start the game by
        // comparing the value of the two sockets
        _players.second.client_sock = client_sock;
        bzero(_players.second.played_move, BUFFER_SIZE);
        _id = client_sock;
    }

    ~Game() = default;

    bool is_invited(const int invited_user_sock, const string mode = "2"){
        if (mode == "0" || mode == "2"){
            for (auto socket: _invited_players){
                if (socket == invited_user_sock) return true;
            }
        }
        if (mode == "1" || mode == "2"){
            for (auto socket: _invited_observers){
                if (socket == invited_user_sock) return true;
            }
        }
        return false;
    }

    bool add_invited_player(const string mode, const int invited_user_sock, string& error_msg){
        // if already invited or already playing of already obsersving this game
        if (is_invited(invited_user_sock, mode)){
            error_msg = "The client has already been invited with this mode";
            return false;
        }
        else if (is_player(invited_user_sock) || is_observer(invited_user_sock) || game_ready_to_start()){
            error_msg = "The client is already playing/observing this game";
            return false;
        }
        else if (mode == "0"){
            _invited_players.push_back(invited_user_sock);
            return true;
        }
        else if (mode == "1"){
            _invited_observers.push_back(invited_user_sock);
            return true;
        }
        else{
            cerr << "Invalid game mode, can't invite client" << endl;
            return false;
        }
    }

    int get_id() const { return _id; }
    bool playing() const { return _playing; }
    bool is_player(const int player_socket) const { return player_socket == _players.first.client_sock || player_socket == _players.second.client_sock ;}

    void register_move(const int player_socket, string buffer_move){
        int idx = get_index(player_socket);
        // longest move is of length 4 (K12h)
        if (buffer_move.size() > 4 || !game_ready_to_start()) return;
        // We ignore the move if it's not this client's turn
        if (idx == 0 && _board_player_1->myTurn()){
            _players.first.client_actions = buffer_move;
            _player_1_moves.push_back(buffer_move);
            _players.first.has_new_move = true;
        }
        // We ignore the move if it's not this client's turn
        else if (idx == 1 && _board_player_2->myTurn()){
            _players.second.client_actions = buffer_move;
            _player_2_moves.push_back(buffer_move);
            _players.second.has_new_move = true;
        }
        else {
            perror("Player not found\n");
        }
    }
    
    string get_name(const int user) const {
        string name{};
        if (user == 0) {
            name = _players.first.name;
        } else if (user == 1) {
            name = _players.second.name;
        } else {
            name = "PlayerNotFound";
        }
        return name;
    }

    bool game_ready_to_start(){
        // Check if both _players are different
         return _players.first.client_sock != _players.second.client_sock;
    }
    void run_game(vector<int>& participants_socket, vector<string>& players_moves) {
        // Check if both _players are different
        if (!game_ready_to_start()) {
            return;
        }
        send_prnt_token("The game has started !", _players.first.client_sock);
        send_prnt_token("The game has started !", _players.second.client_sock);
        add_game_details_to_moves();
        _playing = true;
        WinConditions win_condition;
        if (tolower(_gamemode[0]) == 'd') {
            _board_player_1 = make_shared<Board>();
            _board_player_2 = make_shared<Board>();
            bool has_boats = initialize_game();
            if (!has_boats) win_condition = WinConditions::no_boats;
            else {
                bool timed_out = true;
                while (!_board_player_1->isFinished() && !_board_player_2->isFinished()) {
                    timed_out = !start_classic_game();
                    if (timed_out) break;
                }
                if (timed_out) {
                    win_condition = WinConditions::time_out;
                } else {
                    win_condition = WinConditions::sunk_all_boats;
                }
            }
        } else {
            cerr << "Gamemode not implemented yet" << endl;
        }

        game_over(win_condition);
        players_moves = _players_moves;
        participants_socket.push_back(_players.first.client_sock);
        participants_socket.push_back(_players.second.client_sock);

        vector<int> observers_socket = _observers.get_sockets();
        for (size_t i=0; i < observers_socket.size(); i++) {
            participants_socket.push_back(observers_socket.at(i));
        }
    }

    bool add_player2(const string username, const int player2_sock) {
        // Dont add a second player if there already is one
        if (game_ready_to_start()){
            return false;
        }
        _players.second.name = username;
        _players.second.client_sock = player2_sock;
        return true;
    }

    bool is_observer(const int observer_socket) const {
        return _observers.is_observer(observer_socket);
    }

    void add_observer(const int observer_sock){
        _observers.add_observer(observer_sock, _players_moves);
    }

    bool is_over(){
        return game_ready_to_start() && !_playing;
    }

    int get_index(int player_socket){
    // Return the player index (0 or 1) depending on the socket
        if (is_player(player_socket)){
            if (player_socket == _players.first.client_sock){return 0;}
            else {return 1;}
        }
        else {return -1;}
    }

private:
    void send_prnt_token_to_everyone(const string message){
        // Player 1
        send_prnt_token(message, _players.first.client_sock);
        // Player 2
        send_prnt_token(message, _players.second.client_sock);
        // Observers
        _observers.send_prnt_token_observers(message);
    }

    bool initialize_game() {
        // Create the display and connect it to the game
        shared_ptr<ConsoleBoardInput> display_player_1 =
            make_shared<ConsoleBoardInput>(_board_player_1, _board_player_1);
        shared_ptr<ConsoleBoardInput> display_player_2 =
            make_shared<ConsoleBoardInput>(_board_player_2, _board_player_2);

        display_player_1->update(_players.first);
        // Board has no undamaged boats
        if (!_board_player_1->place_all_boats(display_player_1, _players.first, _players_moves, true, _timer_1)){
            _board_player_2->check_victory();
            return false;
        }

        display_player_2->update(_players.second);
        if (!_board_player_2->place_all_boats(display_player_2, _players.second, _players_moves, false, _timer_2)){
            _board_player_1->check_victory();
            return false;
        }

        _board_player_1->place_enemy_boats(_board_player_2->get_boats());
        _board_player_2->place_enemy_boats(_board_player_1->get_boats());

        display_player_1->update(_players.first);
        _board_player_2->invert_my_turn();
        _displays = {display_player_1, display_player_2};

        // Notify clients about the end of boat placement
        send_message(TOKEN_BOATS_PLACED, _players.first.client_sock);
        send_message(TOKEN_BOATS_PLACED, _players.second.client_sock);

        return true;
    }

    bool start_classic_game() {
        if (_board_player_1->myTurn()) {
            if (!_displays.first->handle_input_server(_players.first, _players.second, _players_moves, true, _timer_1, _observers)){
                if(!_timer_1.has_timer_game()){
                    send_prnt_token("Game over. Player 1 has ran out of time.", _players.first.client_sock);
                    send_prnt_token("Game over. Player 1 has ran out of time.", _players.second.client_sock);
                    return false;
                }
                _board_player_1->invert_my_turn();
                _board_player_2->invert_my_turn();
            }
            else {
                BoardCoordinates coord = _board_player_1->last_fire_target();
                _board_player_2->receive_fire(coord);
                int last_fired_type =
                    _board_player_2->get_my_side().at(coord.y()).at(coord.x()).type();
                if (last_fired_type != 0b011 && last_fired_type != 0b111) {
                    _board_player_2->invert_my_turn();
                    _displays.second->update(_players.second); // display next player's board
                } else {
                    _displays.first->update(_players.first);
                } // display same player's board
            }
        } else {
            if (!_displays.second->handle_input_server(_players.second, _players.first, _players_moves, false, _timer_2,_observers)){
                if(!_timer_2.has_timer_game()){
                    send_prnt_token("Game over. Player 2 has ran out of time.", _players.first.client_sock);
                    send_prnt_token("Game over. Player 2 has ran out of time.", _players.second.client_sock);
                    return false;
                }
                _board_player_1->invert_my_turn();
                _board_player_2->invert_my_turn();
            }
            else{
                BoardCoordinates coord = _board_player_2->last_fire_target();
                _board_player_1->receive_fire(coord);
                int last_fired_type =
                    _board_player_1->get_my_side().at(coord.y()).at(coord.x()).type();
                if (last_fired_type != 0b011 && last_fired_type != 0b111) {
                    _board_player_1->invert_my_turn();
                    _displays.first->update(_players.first);
                } else {
                    _displays.second->update(_players.second);
                }
            }
        }
        return true;
    }

    void game_over(const WinConditions win_condition){
        _playing = false;
        string winner{};
        if (win_condition != WinConditions::time_out){
            // Send the winner message to both clients
            if (_board_player_1->isVictory()){
                winner = _players.first.name + " wins ! ";
            }
            else if (_board_player_2->isVictory()) {
                winner = _players.second.name + " wins ! ";
            } else {
                send_prnt_token("Error: no player won but game ended",_players.first.client_sock);
                send_prnt_token("Error: no player won but game ended",_players.second.client_sock);

                cerr << "Error: no player won but game ended\n";
                return;
            }
        } else {
            if (_timer_1.get_remaining_time_game() == 0) {
                winner = _players.first.name + " wins ! ";
            }
            else if (_timer_2.get_remaining_time_game() == 0) {
                winner = _players.second.name + " wins ! ";
            }
            else {
                cerr << "Error: no player won but game ended\n";
                return;
            }
        }

        vector<string> result{
            "The enemy fleet has been destroyed.",
            "Time has ran out for the enemy.",
            "Looks like the enemy forgot their fleet at the port."
        };
        int result_index = 0;
        switch (win_condition){
            case WinConditions::sunk_all_boats :
                break;
            case WinConditions::time_out :
                result_index = 1;
                break;
            case WinConditions::no_boats :
                result_index = 2;
                break;
            default:
                cerr << "Error, unknown win condition\n";
                break;
        }
        winner.append(result.at(result_index));

        send_prnt_token_to_everyone(winner);
    }

    bool get_new_move(const int player_socket, string &new_move){
        // Return 'true' and change 'new_move' if the last move in the corresponding vector hasn't been played yet, return 'false' otherwise
        int idx = get_index(player_socket);
        if (idx == 0 && _players.first.has_new_move){
            new_move = _player_1_moves.at(_player_1_moves.size()-1);
            return true;
        }
        else if (idx == 1 && _players.second.has_new_move){
            new_move = _player_2_moves.at(_player_2_moves.size()-1);
            return true;
        }
        return false;
    }

    void add_game_details_to_moves(){
        string game_details_tokens{_players.first.name};
        add_token(game_details_tokens, _players.second.name);
        _players_moves.push_back(game_details_tokens);
    }

};
