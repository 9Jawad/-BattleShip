#pragma once

#include <sys/socket.h>
#include "../common/common.hh"

void inline get_input(char buffer[BUFFER_SIZE]) {
    // Get input from user
    bzero(buffer, BUFFER_SIZE);
    fgets(buffer, BUFFER_SIZE, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
    if (buffer[0] == '\0'){
        buffer[0] = '\n';
    }
}

void inline recv_from_server(char buffer[BUFFER_SIZE], const int server_sock){
    // Receive data from server and update the buffer
    bzero(buffer, BUFFER_SIZE);
    int longueur, i, ret;
    longueur = BUFFER_SIZE;
    i = 0;
    ret = 0;
    while (i < longueur) {
        ret = recv(server_sock, buffer, longueur - i, 0);
        if (ret <= 0) {
            return;
            if (ret < 0)
                perror("Error when receiving data\n");
            else
                printf("Déconnexion du serveur.\n");
            return;
        }
        i += ret;
    }
}
