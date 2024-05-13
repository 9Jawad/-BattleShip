
#include "server.hh"

bool Server::is_valid_action(const char action){
        return action == '1' || action == '2' || action == '3' || action == '4';
    }

bool Server::receive_action(const int client_sock, string &action) {
    // Send the welcome message
    string prompt{"Welcome to BATTLESHIP game\n"
                    "Please choose an action;\n"
                    "   Create game        :   '1'\n"
                    "   Join game          :   '2'\n"
                    "   Check invitations  :   '3'\n"
                    "   Watch game         :   '4'\n"
                    "   Replay game        :   '5'\n"};

    // Receive the action (1 -> 5)
    if (!receive_input(action, prompt, client_sock)) return false;
    action = action[0];
    // If the input is invalid
    while(!_menu.is_valid_action(action, 5)){
        if (!receive_input(action, "Incorrect input. Please choose a valid action: '1' -> '5': ", client_sock)) return false;
    }

    action = action[0];
    return true;
}


bool Server::receive_gamemode(string& gamemode, const string name, const int client_sock){
    string message = "Creating ";
    message.append(name);
    message.append("'s game...\nPlease choose a gamemode 'd' or 'c' : ");

    // Receive the gamemode chosen by the client
    if (!receive_input(gamemode, message, client_sock)) return false;

    // If bad input
    while (gamemode.size() == 0 || (gamemode.size() > 0 && (tolower(gamemode[0]) != 'd' && tolower(gamemode[0]) != 'c'))){
        // Receive the new gamemode input
        if (!receive_input(gamemode, "Invalid gamemode, Please choose 'd' or 'c' : ", client_sock)) return false;
    }
    return true;
}

bool Server::receive_timer_turn(string& timer_turn, const int client_sock){
    // Receive the timer_turn chosen by the client
    if (!receive_input(timer_turn, "Please set a maximum time limit for each player's turn (15-60 seconds) : ", client_sock)) return false;

    // If bad input
    while (is_invalid_number(timer_turn, 15, 60)){
        // Receive the new timer_turn input
        if (!receive_input(timer_turn, "Invalid input.\nPlease set a maximum time limit for each player's turn (15-60 seconds) : ", client_sock)) return false;
    }
    return true;
}

bool Server::receive_timer_game(string& timer_game, const int client_sock){
    // Receive the timer_game chosen by the client
    if (!receive_input(timer_game, "Please set a maximum time limit for the game (650-1200 seconds) : ", client_sock)) return false;

    // If bad input
    while (is_invalid_number(timer_game, 650, 1200)){
        
        // Receive the new timer_game input
        if (!receive_input(timer_game, "Invalid input.\nPlease set a maximum time limit for the game (650-1200 seconds) : ", client_sock)) return false;
    }
    return true;
}

void Server::add_game(const string gamemode, const unsigned int timer_turn, const unsigned int timer_game, 
    const string username, const int client_sock, int& game_index){
    _games_mutex.lock();
    // Add the new game to the vector
    for (size_t i=0; i< _games.size() ; i++){
        if (_games.at(i).is_over()){
            _games.at(i) = Game{gamemode,timer_turn,timer_game,username,client_sock};
            game_index = i;
            _games_mutex.unlock();
            return;
        }
    }
    Game game {gamemode,timer_turn,timer_game,username,client_sock};
    _games.push_back(move(game));
    game_index = _games.size() - 1;
    _games_mutex.unlock();
}

bool Server::create_game(const string username, const int client_sock, int& game_index) {
    // Create an instance of Game with the gamemode specified by the client
    string gamemode{}, name{}, timer_turn{}, timer_game{};

    if (!receive_gamemode(gamemode, name, client_sock)) return false;
    if (!receive_timer_turn(timer_turn, client_sock)) return false;
    if (!receive_timer_game(timer_game, client_sock)) return false;

    add_game(gamemode, static_cast<unsigned int>(stoul(timer_turn)),static_cast<unsigned int>(stoul(timer_game)),
             username, client_sock, game_index);
    return true;
}

bool Server::is_invalid_index(const string index, const bool is_player) {
    if (is_player){
        return !(is_number(index)) || (is_number(index) && (stoi(index) < 0 ||
            stoi(index) >= static_cast<int>(_games.size()) || _games.at(stoi(index)).playing()) || _games.at(stoi(index)).is_over());
    }
    return is_invalid_number(index, 0, static_cast<int>(_games.size()-1)) || _games.at(stoi(index)).is_over();
}

void Server::parse_available_games(string& current_games, const bool is_player){
    char buffer[BUFFER_SIZE] {'\0'};
    current_games = "Choose a game to join: \n";
    _games_mutex.lock();
    for (size_t i = 0; i < _games.size(); i++){
        if (_games.at(i).is_over()) continue;
        if (!is_player){
            if (_games.at(i).playing()) {
                sprintf(buffer, "%lu : %s's game. - Ongoing\n", i, _games.at(i).get_name(0).c_str());
            }
            else{
                sprintf(buffer, "%lu : %s's game.\n", i, _games.at(i).get_name(0).c_str());
            }
            current_games.append(buffer);
            bzero(buffer, sizeof(buffer));
        }
        // Parse a string with the indexes and ids of all available games
        else if (!_games.at(i).playing()){
            sprintf(buffer, "%lu : %s's game.\n",i, _games.at(i).get_name(0).c_str());
            current_games.append(buffer);
            bzero(buffer, sizeof(buffer));
        }

    }
    _games_mutex.unlock();
}

bool Server::add_client_to_game(const string username, const int client_sock, const int idx, const bool is_player){
    if (is_player){
        _games_mutex.lock();
        // Add the player to the game
        if (!_games.at(idx).add_player2(username, client_sock)){
            _games_mutex.unlock();
            return false;
        };
        _games_mutex.unlock();
        return true;
    }
    else{
        _games_mutex.lock();
        // Add the observer to the game
        _games.at(idx).add_observer(client_sock);
        _games_mutex.unlock();
        return true;
    }
}

bool Server::join_game(const string username, const int client_sock, int& game_index, const bool is_player){
    // Make a client join an existing game
    string error_msg = "Incorrect input. Please choose a correct index : ";
    string index{};

    string current_games{};
    parse_available_games(current_games, is_player);

    if (!receive_input(index, current_games, client_sock)) return false;

    while(1) {
        _games_mutex.lock();
        bool invalid;
        is_player ? invalid = is_invalid_index(index) : invalid = is_invalid_index(index, false);
        _games_mutex.unlock();
        // If the index is invalid
        if (invalid){
            // Send all available games to the client
            parse_available_games(current_games, is_player);
            error_msg = "Incorrect input. Please choose a correct index.\n";
            current_games = error_msg.append(current_games);

            // Receive the game index
            if (!receive_input(index, current_games, client_sock)) return false;
        }
        // if valid index
        else {
            break;
        }
    }
    game_index = stoi(index);
    if (!add_client_to_game(username, client_sock, game_index, is_player)){
        cerr << "Could not add the client to the game" << endl;
        game_index = -1;
    }
    return true;
}

bool Server::find_game(const int client_sock, int& game_idx){
    _games_mutex.lock();
    // Find the game with the player/observer in it
    for (size_t i = 0; i < _games.size(); i++){
        if (_games.at(i).is_over()) continue;
        if (_games.at(i).is_player(client_sock) || _games.at(i).is_observer(client_sock)){
            game_idx = i;
            _games_mutex.unlock();
            return true;
        }
    }
    _games_mutex.unlock();
    return false;
}

void Server::play_game(Game& game){
    // Thread that runs 1 game
    vector<int> participants_socket;
    vector<string> players_moves;
    game.run_game(participants_socket, players_moves);
    _replays.add_replay(participants_socket, players_moves);
    usleep(20000);
    for(auto socket : participants_socket){
        send_message(TOKEN_GAME_OVER, socket);
    }
}

void Server::start_replay(const int client_sock, const vector<string> &replaying_game){
    string message = "Welcome in replay mode\n"
                        "'n' : next step\n"
                        "'q' : quit\n"
                        "'?' : print commands\n";
    send_prnt_token(message, client_sock);
    string action{};
    // start at 1 because first elem is equal to game details
    size_t replay_index = 1;
    vector<string> replaying_tokens;
    while(1){
        if (replay_index >= replaying_game.size()){
            send_prnt_token("This is the end of the replay", client_sock);
            break;
        }
        recv_client_input(action, client_sock);
        if (action == "n"){
            replaying_tokens.clear();
            get_tokens(replaying_game.at(replay_index), replaying_tokens);
            // if a boat sunk
            if (_replays.is_sunk_move(replaying_tokens)){
                replaying_tokens.clear();
                for (; replay_index < replaying_game.size(); replay_index++){
                    get_tokens(replaying_game.at(replay_index), replaying_tokens);
                    if (!_replays.is_sunk_move(replaying_tokens)){
                        replaying_tokens.clear();
                        break;
                    }
                    send_message(replaying_game.at(replay_index), client_sock);
                    replaying_tokens.clear();
                }
                send_message(TOKEN_UPDATE_BOARDS, client_sock);
            }
            else if (replay_index == 1){
                vector<string> tokens;
                for (;replay_index < replaying_game.size(); replay_index++){
                    get_tokens(replaying_game.at(replay_index), tokens);
                    if (tokens.at(0) != TOKEN_ADD_BOAT_CELL){
                        break;
                    }
                    send_message(replaying_game.at(replay_index), client_sock);
                    tokens.clear();
                }
                send_message(TOKEN_UPDATE_BOARDS, client_sock);
            } else{
                send_message(replaying_game.at(replay_index), client_sock);
                send_message(TOKEN_UPDATE_BOARDS, client_sock);
                replay_index++;
            }
        }
        else if (action == "q"){
            break;
        }
        else{
            send_prnt_token(message, client_sock);
        }
    }
}

void Server::start_replay_menu(const int client_sock){
    vector<vector<string>> accessible_replays;
    _replays.get_replays(client_sock, accessible_replays);
    if (accessible_replays.size() < 1){
        send_prnt_token("You don't have any game to replay", client_sock);
        return;
    }
    ostringstream oss;
    oss << "You have " << accessible_replays.size() << " available replays to watch.\nPlease choose an index:\n";
    string message = oss.str();

    vector<string> game_details_tokens;
    for (size_t i = 0; i < accessible_replays.size(); i++){
        get_tokens(accessible_replays.at(i).at(0), game_details_tokens);
        if (game_details_tokens.size() < 2){
            perror("game details tokens number inferior to 2");
            continue;
        }
        message += "   " + to_string(i) +  " : "+ game_details_tokens.at(0) + " VS " + game_details_tokens.at(1);
        game_details_tokens.clear();
    }

    string index{};
    receive_input(index, message, client_sock, false);

    while(is_invalid_number(index, 0, accessible_replays.size()-1)){
        receive_input(index, message, client_sock, false);
    }

    start_replay(client_sock, accessible_replays.at(stoi(index)));
    send_message(TOKEN_RESET_BOARDS, client_sock);
}

vector<Server::GameInvitation> Server::get_invitations(const int client_sock){
    vector<Server::GameInvitation> invited_games;
    _games_mutex.lock();
    for (size_t i=0; i< _games.size() ; i++){
        // If invited as player
        if (_games.at(i).is_invited(client_sock, "0")){
            invited_games.push_back(Server::GameInvitation{static_cast<int>(i), _games.at(i).get_name(0) + "'s game. as : player", "0"});
        }
        // If invited as observer
        if (_games.at(i).is_invited(client_sock, "1")){
            invited_games.push_back(Server::GameInvitation{static_cast<int>(i), _games.at(i).get_name(0) + "'s game. as : observer", "1"});
        }
    }
    _games_mutex.unlock();
    return invited_games;
}

void Server::check_invitations(const int client_sock, int& game_idx, string& mode){
    vector<Server::GameInvitation> invited_games = get_invitations(client_sock);

    if (invited_games.size() == 0){
        send_prnt_token("You have 0 invitations", client_sock);
        return;
    }

    string message{"Here are your invitations\n"};
    for (size_t i = 0; i < invited_games.size(); i++){
        message += " " + to_string(i) +" : " + invited_games.at(i).msg + "\n";
    }
    message.pop_back();
    send_prnt_token(message, client_sock);

    string index;
    recv_client_input(index, client_sock);
    if (!is_invalid_number(index, 0, invited_games.size())){
        game_idx = invited_games.at(stoi(index)).game_index;
        mode = invited_games.at(stoi(index)).mode;
    }
    else{
        send_prnt_token("Invalid index. Sending you back to the menu", client_sock);
    }

}


bool Server::start_menu(int& game_index, bool& is_player, const string username, const int client_sock){
    string action{};
    bool associated_to_a_game = false;
    game_index = -1;
    while (!associated_to_a_game){
        if (!receive_action(client_sock, action)) return false;
        if (action[0] == '1'){
            if (!create_game(username, client_sock, game_index)) continue;

            if (game_index == -1){
                send_prnt_token("Failed to create a game", client_sock);
                continue;
            }

            send_prnt_token("Game created, waiting for a second player...", client_sock);
            associated_to_a_game = true;
        }
        else if (action[0] == '2'){
            game_index = -1;
            if (!join_game(username, client_sock, game_index)) continue;

            if (game_index == -1){
                send_prnt_token("Failed to join a game", client_sock);
                continue;
            }

            // Creates a thread to handle the game
            thread new_game_thread([this, game_index](){
                play_game(_games.at(game_index));
            });
            _games_threads.push_back(move(new_game_thread));
            associated_to_a_game = true;
        }
        else if (action[0] == '3'){
            game_index = -1;
            string mode;
            check_invitations(client_sock, game_index, mode);
            if (game_index != -1){
                // If player mode
                if (mode == "0"){
                    if (!add_client_to_game(username, client_sock, game_index, true)){
                        send_prnt_token("Could not join the game through invitation", client_sock);
                        continue;
                    }
                    // Creates a thread to handle the game
                    thread new_game_thread([this, game_index](){
                        play_game(_games.at(game_index));
                    });
                    _games_threads.push_back(move(new_game_thread));
                    associated_to_a_game = true;
                }
                // If observer mode
                else if (mode == "1"){
                    if (!add_client_to_game(username, client_sock, game_index, false)){
                        send_prnt_token("Could not join the game through invitation", client_sock);
                        continue;
                    }
                    associated_to_a_game = true;
                }
            }
        }
        else if (action[0] == '4'){
            if (!join_game(username, client_sock, game_index, false)) continue;
            send_prnt_token("Game joined as observer !", client_sock);
            associated_to_a_game = true;
        }
        else if (action[0] == '5'){
            start_replay_menu(client_sock);
        }
    }

    if (game_index == -1){
        cerr << "Error: game index not found\n";
        exit(1);
    }

    is_player = _games.at(game_index).is_player(client_sock);
    return true;
}

void Server::handle_connection() {
    // Wait for a client connection and add it to the vector when he is connected
    _addr_size = sizeof(_client_addr);
    while(1) {
        int client_sock = accept(_server_sock, reinterpret_cast<struct sockaddr *>(&_client_addr), &_addr_size);
        if (client_sock < 0) {
            perror("Error when accepting client\n");
            break;
        }
        printf("[+] New client \n\n");
        client_data new_client;
        new_client.client_sock = client_sock;
        _clients_mutex.lock();
        _clients.push_back(new_client);
        _clients_mutex.unlock();
    }
}
void Server::handle_clients(){
    // Thread that creates and affect 1 thread to 1 client
    size_t idx = 0;
    while(1){
        // Constantly check if a new client has been added to the vector
        if (_clients.size() > idx){
            // thread new_client_thread(handle_client, ref(server));
            thread new_client_thread(&Server::handle_client, this);
            _client_threads.push_back(move(new_client_thread));
            idx++;
        }
        usleep(30000);
    }
    for (size_t i = 0; i < _games_threads.size(); i++){
        _games_threads[i].join();
    }
    for (size_t i = 0; i < _client_threads.size(); i++){
        _client_threads[i].join();
    }
}

void Server::find_unattended_client(int& unattended_client_idx){
    _clients_mutex.lock();
    // Find the socket of a client who hasn't been given a thread yet
    for (size_t i = 0; i < _clients.size(); i++){
        if (!_clients.at(i).has_thread){
            _clients.at(i).has_thread = true;
            unattended_client_idx = i;
            break;
        }
    }
    _clients_mutex.unlock();
}

void Server::handle_client(){
    // Thread dedicated to handle 1 client
    int client_sock, client_idx;
    find_unattended_client(client_idx);
    client_sock = _clients.at(client_idx).client_sock;

    int game_index = -1;
    bool is_player;

    // user connects to the menu
    string username;
    _menu.start_connection_menu(username, client_sock, this);
    _clients.at(client_idx).name = username;
    _clients.at(client_idx).is_connected = true;

    while (true){
        _menu.start_main_menu(username, client_sock, this);
        if (!start_menu(game_index, is_player, username, client_sock)) continue;

        string buffer_str{};
        // Main listening loop which constantly receives data from 1 client and processes it
        while (1){
            // Receive a move
            recv_client_input(buffer_str, client_sock);
            if (buffer_str == TOKEN_GAME_OVER){
                break;
            }
            else if (_games.at(game_index).is_over()){
                cerr << "Le joueur n'est associé à aucune partie\n";
                usleep(20000);
                continue;
            }
            else if (is_player){
                _games_mutex.lock();
                // Register the move
                _games.at(game_index).register_move(client_sock, buffer_str);
                _games_mutex.unlock();
            }
        }
    }
}

bool Server::user_connected(const string username){
    _clients_mutex.lock();
    for (size_t i = 0; i < _clients.size(); i++){
        if (username == _clients.at(i).name){
            _clients_mutex.unlock();
            return _clients.at(i).is_connected;
        }
    }
    _clients_mutex.unlock();
    return false;
}

void Server::setup_server(){
    int port = SERVER_PORT;
    _server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_sock < 0){
        perror("[-] TCP Socket error");
        exit(1);
    }
    memset(&_server_addr, '\0', sizeof(_server_addr));

    int opt = 1;
    setsockopt(_server_sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    _server_addr.sin_family = AF_INET;
    _server_addr.sin_port = port;
    _server_addr.sin_addr.s_addr = INADDR_ANY;

    int b = bind(_server_sock, reinterpret_cast<struct sockaddr*>(&_server_addr), sizeof(_server_addr));
    if (b < 0){
        perror("[-] Bind error");
        exit(1);
    }
    listen(_server_sock, 100);
    printf("Server ready, waiting for players...\n\n");
}
void Server::start_server(){
    _threads[HANDLE_CONNECTION] = thread(&Server::handle_connection, this);
    _threads[HANDLE_CLIENTS] = thread(&Server::handle_clients, this);

    _threads[HANDLE_CONNECTION].join();
    _threads[HANDLE_CLIENTS].join();
}

bool Server::find_user_socket(const string user, int& client_sock){
    _clients_mutex.lock();
    for (size_t i = 0; i < _clients.size(); i++){
        if (_clients.at(i).name == user){
            if (_clients.at(i).is_connected){
                client_sock = _clients.at(i).client_sock;
                _clients_mutex.unlock();
                return true;
            }
            _clients_mutex.unlock();
            return false;
        }
    }
    _clients_mutex.unlock();
    return false;
}

bool Server::find_username(string& user, const int client_sock){
    _clients_mutex.lock();
    for (size_t i = 0; i < _clients.size(); i++){
        if (_clients.at(i).client_sock == client_sock){
            if (_clients.at(i).is_connected){
                user = _clients.at(i).name;
                _clients_mutex.unlock();
                return true;
            }
            _clients_mutex.unlock();
            return false;
        }
    }
    _clients_mutex.unlock();
    return false;
}

bool Server::in_game(const int user_socket){
    _games_mutex.lock();
    for (size_t i=0; i< _games.size() ; i++){
        if (_games.at(i).is_player(user_socket) || _games.at(i).is_observer(user_socket)){
            _games_mutex.unlock();
            return true;
        }
    }
    _games_mutex.unlock();
    return false;
}

bool Server::invite_client(const string mode, const int sender_sock, const int receiver_sock, string& error_msg){
    if (mode != "0" && mode != "1"){
        cerr << "Invalid game mode, can't invite client" << endl;
        return false;
    }
    _games_mutex.lock();
    for (size_t i=0; i< _games.size() ; i++){
        // if client is the creator of the game
        if (_games.at(i).get_index(sender_sock) == 0){
            // If player has already been invited
            if (!_games.at(i).add_invited_player(mode, receiver_sock, error_msg)){
                _games_mutex.unlock();
                return false;
            }
            _games_mutex.unlock();
            return true;
        }
    }
    _games_mutex.unlock();
    error_msg = "You must have created a game to invite another player";
    return false;
}


void Server::handle_command(string command, const int sender_sock){

    string sender{};
    // If user not connected
    if (!find_username(sender, sender_sock)){
        send_prnt_token("You must be connected to use a command.", sender_sock);
        return;
    }

    vector<string> cmd_tokens;
    command.erase(0, 1);
    get_tokens(command, cmd_tokens, " ");
    // update the player's board
    if (cmd_tokens.size() >= 1 && cmd_tokens.at(0) == "u"){
        if (in_game(sender_sock)) send_message(TOKEN_UPDATE_BOARDS, sender_sock);
        else send_prnt_token("You must be in game to use this command", sender_sock);
    }
    else if (cmd_tokens.size() >= 1 && cmd_tokens.at(0) == "help"){
        string all_commands{"Here are all the possible commands\n"
                            "   /help                    : print all commands\n"
                            "   /u                       : update your board\n"
                            "   /mp 'friend' 'message'   : send a message to your friend\n"
                            "   /invite 'friend' 'mode'  : invite a friend in your gamme as a player"
                            " (mode='0') or as an observer (mode='1')\n"};
        send_prnt_token(all_commands, sender_sock);
    }
    else if ((cmd_tokens.size() >= 3 && (cmd_tokens.at(0) == "mp" || cmd_tokens.at(0) == "invite"))){
        string receiver{cmd_tokens.at(1)};
        int receiver_sock = -1;
        // If receiver found
        if (find_user_socket(receiver, receiver_sock)){
            // If receiver not friend
            if (!_db->are_friends(sender, receiver)){
                send_prnt_token("You're not friend with this person yet.\n", sender_sock);
            }
            else{
                if (cmd_tokens.at(0) == "mp"){
                    string message{sender + ": "};
                    for (size_t i = 2; i < cmd_tokens.size(); i++){
                        message += cmd_tokens.at(i) + " ";
                    }
                    message.pop_back();
                    send_prnt_token(message, receiver_sock);
                } 
                else if (cmd_tokens.at(0) == "invite"){
                    if (cmd_tokens.at(2) != "0" && cmd_tokens.at(2) != "1"){
                        send_prnt_token("Invalid game mode", sender_sock);
                        return;
                    }

                    string mode{};
                    cmd_tokens.at(2) == "0" ? mode = "player" : mode = "observer";
                    string error_msg{};
                    if (invite_client(cmd_tokens.at(2), sender_sock, receiver_sock, error_msg)){
                        send_prnt_token("You have been invited as " + mode + " to " + sender +
                            "'s game.\n", receiver_sock);
                        send_prnt_token("Your friend "+receiver+" has been invited as "+mode+"!\n", sender_sock);
                    } else{
                        send_prnt_token(error_msg, sender_sock);
                    }

                }
            }
        }
        else{
           send_prnt_token("The user you're targeting doesn't exist or isn't connected\n", sender_sock);
        }
    }
}

bool Server::recv_client_input(string& input, const int client_sock){
    char buffer[BUFFER_SIZE] {'\0'};
    string cmd;
    // recv input from client until it's not a command
    recv_from_client(buffer, client_sock);
    while (buffer[0] == '/'){
        cmd = buffer;
        handle_command(cmd, client_sock);
        recv_from_client(buffer, client_sock);
    }
    input = buffer;
    if (input == "back") return false;
    return true;
}

bool Server::receive_input(string& input, const string prompt, const int client_sock, const bool can_back){
    input = "-1";
    send_prnt_token(prompt, client_sock);
    
    if (!can_back){
        bool is_back = true;
        while (is_back){
            recv_client_input(input, client_sock);
            if (input == "back"){
                send_prnt_token("You can't back from here", client_sock);
            } else{
                is_back = false;
            }
        }
    } else{
        recv_client_input(input, client_sock);
        if (input == "back"){
            return false;
        }
    }
    return true;
}