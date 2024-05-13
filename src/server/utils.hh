#pragma once

#include <algorithm>
#include "../common/common.hh"

struct client_data {
    string name;

    // Message from the client to the server
    string client_actions;

    // Answer of the server to the client
    char played_move [BUFFER_SIZE];

    // Answer of the server to the client
    string tokens = "";
    bool has_new_move = false;
    bool has_thread = false;
    bool is_connected = false;
    int client_sock;
};

bool inline is_number(const string& s){
    return !s.empty() && find_if(s.begin(),s.end(),
           [](unsigned char c) { return !isdigit(c); }) == s.end();
}

bool inline is_invalid_number(const string number, const int lower, const int higher){
    // Return true if the number is not in the interval [lower,higher]
    return !(is_number(number)) || (is_number(number) && (stoi(number) < lower || stoi(number) > higher));
}