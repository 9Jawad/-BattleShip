#pragma once

#include <string>
#include <cstring>
#include <iostream>
#include <vector>
#include <sys/socket.h>


using namespace std;

#define SERVER_PORT 8080
#define BUFFER_SIZE 200

static const string TOKEN_DELIMETER = "~";
static const string TOKEN_ADD_BOAT_CELL = "20";
static const string TOKEN_BOATS_PLACED = "21";
static const string TOKEN_SEND_FIRE = "22";
static const string TOKEN_RECV_FIRE = "23";
static const string TOKEN_UPDATE_BOARDS = "24";
static const string TOKEN_RESET_BOARDS = "28";
static const string TOKEN_GAME_OVER = "29";
static const string TOKEN_PRNT_MSG = "40";


void inline str_to_char(const string message, char buffer[BUFFER_SIZE]){
    bzero(buffer, BUFFER_SIZE);
    if (strlen(message.c_str()) <= BUFFER_SIZE - 1){
        strncpy(buffer, message.c_str(), BUFFER_SIZE - 1);
        // cout << "Message traduit : " << buffer << endl;
        buffer[BUFFER_SIZE - 1] = '\0';
    } else { 
        cout << "taille " << message.length();
        cout << "\n" << message << endl;
        perror("Chaine de caractère trop longue pour le buffer\n"); 
        // cerr << "messsage size : " << strlen(message.c_str()) << " buffer size : " << BUFFER_SIZE << endl;
    }
}

void inline send_to_socket(const char buffer[BUFFER_SIZE], const int client_sock){
    int longueur = BUFFER_SIZE;
    ssize_t bytes_sent = 0;
    while (bytes_sent < longueur) {
        ssize_t result = send(client_sock, buffer + bytes_sent, longueur - bytes_sent, 0);
        if (result < 0) {
            perror("error when sending data\n");
            break;
        }
        bytes_sent += result;
    }
}

void inline send_message(const string msg, const int socket, const int buffer_size = BUFFER_SIZE) {
    char buffer[buffer_size] {'\0'};
    str_to_char(msg, buffer);
    send_to_socket(buffer, socket);
}

void inline get_tokens(string line, vector<string> &tokens, const string delimiter = TOKEN_DELIMETER){
/*  gets each token in the line separated by the delimiter and put them in the vector "tokens"
*/
    size_t delim_pos = 0;
    bool looping = true;
    while (looping){
        delim_pos = line.find(delimiter);
        string token = line.substr(0, delim_pos); // get the token

        if (delim_pos == string::npos){    // no delimiter found == last elem in line & stop looping
            looping = false;
            char lastChar = token.back();
            if(lastChar == '\n' || lastChar == '\r')
                token.pop_back(); // delete lastChar of token
        }
        tokens.push_back(token);
        line.erase(0, delim_pos + delimiter.length()); // delete token + delimiter after extracting it
    }
}

void inline add_token(string &tokens, const string new_token){
    if (tokens.size() > 0) {
        tokens.append(TOKEN_DELIMETER);
    }
    tokens.append(new_token);

}