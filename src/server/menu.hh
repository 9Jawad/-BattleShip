#pragma once

class Server;

#include "../common/common.hh"
#include "database.hh"
#include <memory>
#include "utils.hh"
#include "network.hh"
#include "not_implemented_error.hh"

class Menu{
    shared_ptr<Database> _db;

    void receive_connection_action(string &action, const int client_sock, Server* server);
    void receive_main_action(string &action, const int client_sock, Server* server);
    bool receive_username(string& username, const int client_sock, const bool can_exist, Server* server);
    bool receive_password(string& password, const int client_sock, Server* server, const string username = "");
    bool signup(string& username, const int client_sock, Server* server);
    bool login(string& username, const int client_sock, Server* server);
    
    bool is_valid_username(const string username, string& error_msg, const bool can_exist, Server* server);
    bool is_valid_password(const string password, const string username, string& error_msg);
    void send_friends_list(const vector<string>& friends, const int client_sock);
    bool parse_friend_requests(const vector<string>& friend_requests, string& message);
    void send_friend_requests_list(const string username, vector<string>& friend_requests, const int client_sock, Server* server);
    void send_friend_request(const string username, const int client_sock, Server* server);

    bool receive_input(string& input, const string prompt, const int client_sock, Server* server, const bool can_back = true);

public:
    Menu(){}
    Menu(shared_ptr<Database> database) : _db{database} {}

    bool is_valid_action(const string action, const int range);
    void start_main_menu(string& username, const int client_sock, Server* server);
    void start_connection_menu(string& username, const int client_sock, Server* server);


};