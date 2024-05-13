#pragma once

#include "utils.hh"
#include <arpa/inet.h>
#include "../common/common.hh"


void inline send_played_move(client_data& client) {
    send_to_socket(client.played_move, client.client_sock);
}

void inline recv_from_client(char buffer[BUFFER_SIZE], const int client_sock){
    bzero(buffer, BUFFER_SIZE);
    int longueur, i, ret;
    longueur = BUFFER_SIZE;
    i = 0;
    ret = 0;
    while (i < longueur) {
        ret = recv(client_sock, buffer, longueur - i, 0);
        if (ret <= 0) {
            return;
            if (ret < 0)
                perror("read");
            else
                printf("Déconnexion du serveur.\n");
            return;
        }
        i += ret;
    }
}

void inline send_prnt_token(const string msg, const int client_sock){
    string token = TOKEN_PRNT_MSG;
    const int msg_length = token.length()+3+ msg.length(); // 3 because ~ + 0/1 + ~
    const int substring_length = BUFFER_SIZE-1;

    // Tells the client that the message will be split
    if (msg_length > substring_length){
        add_token(token, "1");
        add_token(token, msg);
        for (int i = 0; i < msg_length; i+= substring_length){
            string substring = token.substr(i,substring_length);
            send_message(substring, client_sock);

            //If last substring to send
            if (i + substring_length >= msg_length){
                send_message(TOKEN_PRNT_MSG, client_sock);
            }
        }
    // If message <= BUFFER_SIZE
    } else{
        add_token(token, "0");
        add_token(token, msg);
        send_message(token, client_sock);
    }
}