#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <utility>
#include <thread>
#include <signal.h>
#include <vector>
#include <memory>
#include "grid_display.hh"
#include "board_lite.hh"
#include "network.hh"

#pragma once


static volatile sig_atomic_t stop_threads = 0;

#define SEND 0
#define RECEPTION 1

class Client{
    thread _threads[2];
    int _server_sock;
    int _port = SERVER_PORT;
    char const* _ip = "127.0.0.1";
    GridDisplay* _grid_display;

    void setup_client(){
        // Connect the client to the server
        struct sockaddr_in addr;
        _server_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (_server_sock < 0){
            perror("[-] TCP Socket error");
            exit(1);
        }

        memset(&addr, '\0', sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = _port;
        addr.sin_addr.s_addr = inet_addr(_ip);

        if (connect(_server_sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0){
            perror("Connection to server failed\n");
        };
        printf("[+] Connected to the server \n\n");
    }

    void send_info(){
        // Thread that constantly reads input from user and sends it to the server
        char buffer[BUFFER_SIZE] {'\0'};
        while (!stop_threads){
            get_input(buffer);
            send_to_socket(buffer, _server_sock);
            
        }
    }

    void add_boat(board_status* vec, const int x_coord, const int y_coord){
        // Adds a boat cell to the last boat or create a new boat with a boat cell
        pair<int, int> last_ship_coord{y_coord, x_coord};

        // The new_ship bool indicates that we must create a new ship
        if ((*vec).new_ship){                          
            vector<pair<int, int>> ship;
            ship.push_back(last_ship_coord);
            (*vec).boats.push_back(ship);
            (*vec).new_ship = false;
        }
        // If the first coord isn't repeated then we add the boat cell to the last boat
        else if ((*vec).boats.at((*vec).boats.size()-1).at(0) != last_ship_coord){
            (*vec).boats.at((*vec).boats.size()-1).push_back(last_ship_coord);
        } 
        // This means that the first coord has been repeated thus announcing the end of this boat
        else {
            (*vec).new_ship = true;
        }
    }

    void add_boat_cell(const string player, const string x, const string y){
        if (player == "1"){
            add_boat(_grid_display->get_board_data(), stoi(x),stoi(y));
            if (_grid_display->get_board_data()->new_ship){
                _grid_display->place_last_boat(true);
            }
        }
        else if (player == "2"){
            add_boat(_grid_display->get_enemy_board_data(), stoi(x),stoi(y));
            if (_grid_display->get_enemy_board_data()->new_ship){
                _grid_display->place_last_boat(false);
            }
        }
        else {
            perror("Boat cell send without specifying the player\n");
        }
    }

    void recv_fire(const string x, const string y, const string cell_type){
        char buffer[BUFFER_SIZE] {'\0'};
        int first_x = stoi(x);
        int first_y = stoi(y);
        // If the boat sunk
        if ((*_grid_display).receive_fire(first_x, first_y, cell_type)){
            int coord_x, coord_y;
            recv_from_server(buffer, _server_sock);
            vector<string> tokens;
            string buffer_str{};
            buffer_str = buffer;
            get_tokens(buffer_str, tokens);
            if (tokens.size() < 4){
                perror("Error: not enough tokens given to place the sunk boat\n");
            }
            coord_x = stoi(tokens.at(1));
            coord_y = stoi(tokens.at(2));
            string cell_type = tokens.at(3);
            while (coord_x != first_x || coord_y != first_y){
                if (!(*_grid_display).receive_fire(coord_x, coord_y, tokens.at(3))) {perror("Error Received non SUNK CELL\n");};
                recv_from_server(buffer, _server_sock);
                tokens.clear();
                buffer_str = buffer;
                get_tokens(buffer_str, tokens);
                if (tokens.size() < 4){
                    perror("Error: not enough tokens given to place the sunk boat\n");
                }
                coord_x = stoi(tokens.at(1));
                coord_y = stoi(tokens.at(2));
            }
        }
    }

    void send_fire(const string x, const string y, const string cell_type){
        int coord_x = stoi(x);
        int coord_y = stoi(y);
        (*_grid_display).send_fire(coord_x, coord_y, cell_type);
    }

    void print_msg_token(const vector<string> tokens, bool& just_updated_board){
        char buffer[BUFFER_SIZE]{'\0'};
        string buffer_str{};
        // If message isn't split (>= BUFFER_SIZE)
        if(tokens.at(1) == "0"){
            string message = tokens.at(2);
            if (just_updated_board){
                cout << "\n" << message << endl;
                just_updated_board = false;
            }
            else{
                cout << message << endl;
            }
        }
        else if (tokens.at(1) == "1"){
            string message = tokens.at(2);
            bool looping = true;
            while (looping){
                recv_from_server(buffer, _server_sock);
                buffer_str = buffer;
                if (buffer_str == TOKEN_PRNT_MSG){
                    looping = false;
                    if (just_updated_board){
                        cout << "\n" << message << endl;
                        just_updated_board = false;
                    }
                    else{
                        cout << message << endl;
                    }
                }
                message += buffer_str;
            }
        }
    }
    void reception_info(){
        // Thread that constantly receives data from the server
        char buffer[BUFFER_SIZE]{'\0'};
        bool just_updated_board = true;
        while (!stop_threads){
            recv_from_server(buffer, _server_sock);
            vector<string> tokens;
            string buffer_str{};
            buffer_str = buffer;
            get_tokens(buffer_str, tokens);
            if (strcmp(buffer, "*") == 0){
                stop_threads = 1;
            } else {
                // Add a boat cell
                if (tokens.size() >= 4 && tokens.at(0) == TOKEN_ADD_BOAT_CELL) {
                    add_boat_cell(tokens.at(1), tokens.at(2), tokens.at(3));
                }
                // Finished adding all boats
                else if (tokens.size() >= 1 && tokens.at(0) == TOKEN_BOATS_PLACED) {
                    (*(*_grid_display).get_board_data()).playing = true;
                }
                // Send fire
                else if (tokens.size() >= 4 && tokens.at(0) == TOKEN_SEND_FIRE) {
                    send_fire(tokens.at(1),  tokens.at(2),  tokens.at(3));
                }
                // Receive fire
                else if (tokens.size() >= 4 && tokens.at(0) == TOKEN_RECV_FIRE) {
                    recv_fire(tokens.at(1), tokens.at(2), tokens.at(3));
                }
                // Update the board
                else if (tokens.size() >= 1 && tokens.at(0) == TOKEN_UPDATE_BOARDS){ 
                    (*_grid_display).update();
                    just_updated_board = true;
                }
                // Prints the message received by the server
                else if (tokens.size() >= 3 && tokens.at(0) == TOKEN_PRNT_MSG) {
                    print_msg_token(tokens, just_updated_board);
                }
                else if (tokens.size() >= 1 && tokens.at(0) == TOKEN_GAME_OVER) {
                    (*_grid_display).reset();
                    send_message(TOKEN_GAME_OVER, _server_sock);
                }
                else if (tokens.size() >= 1 && tokens.at(0) == TOKEN_RESET_BOARDS) {
                    (*_grid_display).reset();
                }
                else {
                    printf("Message can not be processed : %s\n", buffer);
                }
            }
        }
    }

public:
    Client(GridDisplay *grid_display) : _grid_display{grid_display}{

        // Connect the client to the server
        setup_client();

    }

    void start_client(){
        _threads[SEND] = thread(&Client::send_info, this);
        _threads[RECEPTION] = thread(&Client::reception_info, this);

        _threads[SEND].join();
        _threads[RECEPTION].join();
    }
};