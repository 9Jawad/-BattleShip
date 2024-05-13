#pragma once

#include "game.hh"
#include "network.hh"
#include "replays.hh"
#include "menu.hh"
#include <cstddef>
#include <csignal>
#include <cstring>
#include <unistd.h>
#include <thread>
#include <mutex>


#define THREADS_NUMBER 2
#define HANDLE_CONNECTION 0
#define HANDLE_CLIENTS 1


class Server{

    struct GameInvitation{
        int game_index;
        string msg;
        string mode; // 0 or 1
    };

    mutex _games_mutex;
    mutex _clients_mutex;
    socklen_t _addr_size;
    int _server_sock;
    struct sockaddr_in _server_addr, _client_addr;

    vector<client_data> _clients;
    vector<Game> _games;
    Replays _replays;
    shared_ptr<Database> _db;
    Menu _menu;

    // 1 thread for handle_connection() + 1 thread for handle_clients()
    thread _threads[THREADS_NUMBER];
    // 1 thread to handle a game
    vector<thread> _games_threads;
    // 1 listening thread/ client
    vector<thread> _client_threads;


    bool is_valid_action(const char action);

    bool receive_action(const int client_sock, string &action) ;

    bool receive_gamemode(string& gamemode, const string name, const int client_sock);

    bool receive_timer_turn(string& timer_turn, const int client_sock);

    bool receive_timer_game(string& timer_game, const int client_sock);

    void add_game(const string gamemode, const unsigned int timer_turn, const unsigned int timer_game
        ,const string username, const int client_sock, int& game_index);

    bool create_game(const string username, const int client_sock, int& game_index) ;
    
    bool join_game(const string username, const int client_sock, int& game_index, const bool is_player = true);

    bool is_invalid_index(const string index, const bool is_player = true ) ;

    void parse_available_games(string& current_games, const bool is_player);

    bool add_client_to_game(const string username, const int client_sock, const int idx, const bool is_player);

    bool find_game(const int client_sock, int& game_idx);

    void play_game(Game& game);

    void start_replay_menu(const int client_sock);

    bool start_menu(int& game_index, bool& is_player, const string username, const int client_sock);

    void handle_connection() ;

    void handle_clients();

    void find_unattended_client(int& unattended_client_idx);

    void handle_client();

    void start_replay(const int client_sock, const vector<string> &replaying_game);

    bool find_user_socket(const string user, int& client_sock);

    bool find_username(string& user, const int client_sock);

    bool are_friends(const string user1, const string user2){return _db->are_friends(user1,user2);}

    bool in_game(const int user_socket);

    bool invite_client(const string mode, const int sender_sock, const int receiver_sock, string& error_msg);

    vector<GameInvitation> get_invitations(const int client_sock);
    void check_invitations(const int client_sock, int& game_idx, string& mode);
    
    bool receive_input(string& input, const string prompt, const int client_sock, const bool can_back = true);

    
public:
    Server(){
        _db = make_shared<Database>("./src/server/database.sqlite");
        _menu = Menu{_db};
        _games.reserve(10); // Pre-alloacte enough space for 10 instances of 'Game' in the vector
    }

    void start_server();
    void setup_server();
    bool user_connected(const string username);
    void handle_command(string command, const int sender_sock);
    bool recv_client_input(string& input, const int client_sock);
};