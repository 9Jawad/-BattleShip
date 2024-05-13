#include "menu.hh"
#include "server.hh"

bool Menu::is_valid_action(const string action, const int range){
        if (!is_number(action)){
            return false;
        }
        for (int i = 1; i <= range; i++){
            if (stoi(action) == i)
                return true;
        }
        return false;
}

bool Menu::is_valid_username(const string username, string& error_msg, const bool can_exist, Server* server){
    if ((username.empty() || username.size() > 15 || username[0] == '\n')){
        error_msg = "Invalid input.\nPlease choose a valid username (max 15 characters) : ";
        return false;
    }
    if (can_exist){
        if (!_db->user_exist(username)){
            error_msg = "Invalid input.\nUsername doesn't exist.";
            return false;
        }
        else if (server->user_connected(username)){
            error_msg = "User already connected with this username.";
            return false;
        }
        // if it can exist then it must exist
        return true;
    } 
    if (_db->user_exist(username)){
        error_msg = "Invalid input.\nUsername already exists.";
        return false;
    }
    return true;
}

bool Menu::is_valid_password(const string password, const string username, string& error_msg){
    if (password.empty() || password.size() > 30 || password[0] == '\n'){
        error_msg = "Invalid input.\nPlease choose a valid password (max 30 characters) : ";
        return false;
    }
    if (username != ""){
        // password must be right
        if (_db->is_correct_password(password, username))
            return true;
        error_msg = "Invalid input.\nIncorrect password : ";
        return false;
    } 
    return true;
}

bool Menu::receive_input(string& input, const string prompt, const int client_sock, Server* server, const bool can_back){
    input = "-1";
    send_prnt_token(prompt, client_sock);
    
    if (!can_back){
        bool is_back = true;
        while (is_back){
            server->recv_client_input(input, client_sock);
            if (input == "back"){
                send_prnt_token("You can't back from here", client_sock);
            } else{
                is_back = false;
            }
        }
    } else{
        server->recv_client_input(input, client_sock);
        if (input == "back"){
            return false;
        }
    }
    return true;
}

void Menu::receive_connection_action(string &action, const int client_sock, Server* server){

    string prompt{"Welcome to BATTLESHIP game\n"
                    "   Sign up  :   '1'\n"
                    "   Log in   :   '2'\n"
                    "   Type 'back' to go back to the previous menu"};
    receive_input(action, prompt, client_sock, server);
    action = action[0];
    // If the input is invalid
    while(!is_valid_action(action, 2)){
        // Notify the client with an error message
        receive_input(action, "Incorrect input. Please choose a valid action: '1' or '2': ", client_sock, server);
        action = action[0];
    }
}

bool Menu::receive_username(string& username, const int client_sock, const bool can_exist, Server* server){
    // Ask the client to choose a username
    string prompt{"Please choose a valid username (max 15 characters) : "};
    // Receive the username chosen by the client
    if (!receive_input(username, prompt, client_sock, server)) return false;

    string error_msg;
    // If bad input
    while (!is_valid_username(username, error_msg, can_exist, server)){
        // Send the error message
        if (!receive_input(username, error_msg.append("\nPlease choose a valid username (max 15 characters) : "), client_sock, server)) return false;
    }
    return true;
}

bool Menu::receive_password(string& password, const int client_sock, Server* server, const string username){
    // Ask the client to choose a password
    string prompt{"Please choose a valid password (max 30 characters) : "};
    // Receive the password chosen by the client
    if (!receive_input(password, prompt, client_sock, server)) return false;

    string error_msg;
    // If bad input
    while (!is_valid_password(password, username, error_msg)){   
        if (!receive_input(password, error_msg, client_sock, server)) return false;
    }
    return true;
}

void Menu::receive_main_action(string &action, const int client_sock, Server* server){
    // Send the welcome message
    string prompt{"Please choose an action\n"
                    "   Start playing           :   '1'\n"
                    "   Check friends           :   '2'\n"
                    "   Check friend requests   :   '3'\n"
                    "   Send friend request     :   '4'\n"
                    "   You can also check all available commands with /help\n"};
    // Receive the action (1 -> 5)
    receive_input(action, prompt, client_sock, server, false);
    action = action[0];

    // If the input is invalid
    while(!is_valid_action(action, 4)){
        receive_input(action, "Incorrect input. Please choose a valid action : '1' -> '4': ", client_sock, server, false);
        action = action[0];
    }
}

bool Menu::signup(string& username, const int client_sock, Server* server){
    string password;
    if (!receive_username(username, client_sock, false, server)) return false;
    if (!receive_password(password, client_sock, server)) return false;
    _db->create_user(username, password);
    send_prnt_token("Account created, you're now connected.\nWelcome "+ username + "!", client_sock);
    return true;
}

bool Menu::login(string& username, const int client_sock, Server* server){
    string password;
    if (!receive_username(username, client_sock, true, server)) return false;
    if (!receive_password(password, client_sock, server, username)) return false;
    send_prnt_token("You're now connected.\nWelcome back "+ username + "!", client_sock);
    return true;
}

void Menu::send_friends_list(const vector<string>& friends, const int client_sock){
    string message = "You have " + to_string(friends.size()) + " friends\n";
    for (auto _friend : friends){
        message += ("   " + _friend + "\n");
    }
    send_prnt_token(message, client_sock);
}

bool Menu::parse_friend_requests(const vector<string>& friend_requests, string& message){
    // return false if no friend requests
    if (friend_requests.size() == 0){
        message = "You don't have any friend requests\n";
        return false;
    }
    message = "You have " + to_string(friend_requests.size()) + " friend requests\n"
                        "    Select an index to accept a friend request\n"
                        "    Type 'd' + an index to delete a friend request\n"
                        "    Type 'back' to go back\n";
    for (size_t i = 0; i < friend_requests.size() ; i++){
        message += (" " + to_string(i) + " " + friend_requests.at(i) + "\n");
    }
    return true;
}

void Menu::send_friend_requests_list(const string username, vector<string>& friend_requests, const int client_sock, Server* server){
    string message;
    if (!parse_friend_requests(friend_requests, message)){
        send_prnt_token(message, client_sock);
        return;
    }
    send_prnt_token(message, client_sock);

    string action;
    bool looping = true;
    while (looping){
        server->recv_client_input(action, client_sock);
        // Delete friend request
        if (action[0] == 'd'){
            string index{action[1]};
            if (!is_number(index) ||  (is_number(index) 
                && is_invalid_number(index, 0, friend_requests.size()-1))){
                send_prnt_token("Invalid index", client_sock);
                continue;
            }
            _db->remove_friend_requests(username, friend_requests.at(stoi(index)));
            friend_requests = _db->get_friend_requests(username);
            if (!parse_friend_requests(friend_requests, message)){
                send_prnt_token(message, client_sock);
                looping = false;
            }
        }
        // Accept friend request
        else if (!is_invalid_number(action, 0, friend_requests.size()-1)){
            _db->add_friend(username, friend_requests.at(stoi(action)));
            _db->remove_friend_requests(username, friend_requests.at(stoi(action)));
            friend_requests = _db->get_friend_requests(username);
            if (!parse_friend_requests(friend_requests, message)){
                send_prnt_token(message, client_sock);
                looping = false;
            }
        }
        else if (action == "back"){
            looping = false;
        }
        else{
            parse_friend_requests(friend_requests, message);
            send_prnt_token(message, client_sock);
        }
    }
}

void Menu::send_friend_request(const string username, const int client_sock, Server* server){
    send_prnt_token("Please type your friend's username", client_sock);
    
    string friend_name{};
    server->recv_client_input(friend_name, client_sock);
    if (friend_name == username){
        send_prnt_token("You can't add yourself as a friend.", client_sock);
        return;
    }
    vector<string> users = _db->get_all_users();
    for (auto user: users){
        if (user == friend_name){
            _db->add_friend_request(user, username);
            send_prnt_token("The friend request has been sent to " + user, client_sock);
            return;
        }
    }
    send_prnt_token("The user doesn't exist : " + friend_name, client_sock);
}

void Menu::start_main_menu(string& username, const int client_sock, Server* server){
    string action{};
    bool starting_game = false;
    while (!starting_game){
        receive_main_action(action, client_sock, server);
        if (action[0] == '1'){
            starting_game = true;
        }
        else if (action[0] == '2'){
            vector<string> friends = _db->get_friends(username);
            send_friends_list(friends, client_sock);
        }
        else if (action[0] == '3'){
            vector<string> friend_requests = _db->get_friend_requests(username);
            send_friend_requests_list(username, friend_requests, client_sock, server);
        }
        else if (action[0] == '4'){
            send_friend_request(username, client_sock, server);
        }
    }
}

void Menu::start_connection_menu(string& username, const int client_sock, Server* server){
    string action{};
    bool connected = false;
    while (!connected){
        receive_connection_action(action, client_sock, server);
        if (action[0] == '1'){
            if (!signup(username, client_sock, server)) continue;
            connected = true;
        }
        else if (action[0] == '2'){
            if (!login(username, client_sock, server)) continue;
            connected = true;
        }
    }
}