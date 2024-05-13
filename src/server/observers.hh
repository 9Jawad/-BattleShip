#pragma once
#include "not_implemented_error.hh"
#include "network.hh"
#include "../common/common.hh"

class Observers{
    struct Observer{
        int socket;
        bool active = true;
    };
    vector<Observer> _observers;


public:
    vector<int> get_sockets(){
        vector<int> sockets;
        for(size_t i = 0; i < _observers.size(); i++){
            sockets.push_back(_observers.at(i).socket);
        }
        return sockets;
    }

    bool is_observer(const int observer_socket) const {
        for (size_t i = 0 ; i < _observers.size() ; i++){
            if (_observers.at(i).socket == observer_socket) return true;
        }
        return false;
    }

    void add_observer(const int observer_sock, const vector<string> players_moves){

        for (auto observer : _observers){
            // Dont add an observer if he already is present
            if (observer.socket == observer_sock) return;
        }

        _observers.push_back(Observer{.socket=observer_sock});
        cerr << "Adding new observer\n";

        size_t start_idx = 0;
        vector<string> tokens;
        // start at 1 because first elem is equal to game details
        for (size_t i = 1; i < players_moves.size(); i++){
            get_tokens(players_moves.at(i), tokens);
            // If boat placement over
            if (tokens.at(0) != TOKEN_ADD_BOAT_CELL){
                cerr << "boat placement over " << tokens.at(0) << endl;
                break;
            }
            tokens.clear();
            start_idx++;
        }
        // Only send actions after the boats placement
        for (size_t i = start_idx; i < players_moves.size(); i++){
            send_message_to_observers(players_moves.at(i));
        }

        send_message(TOKEN_UPDATE_BOARDS, observer_sock);
    }

    void send_message_to_observers(const string message){
        for (size_t i = 0; i < _observers.size(); i++){
            if(_observers.at(i).active)
                send_message(message, _observers.at(i).socket);
        }
    }
    void send_prnt_token_observers(const string message){
        for (size_t i = 0; i < _observers.size(); i++){
            if(_observers.at(i).active)
                send_prnt_token(message, _observers.at(i).socket);
        }
    }
    void update_observers() {
        send_message_to_observers(TOKEN_UPDATE_BOARDS);
    }


};