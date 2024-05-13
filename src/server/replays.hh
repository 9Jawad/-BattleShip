#pragma once
#include "not_implemented_error.hh"
#include "../common/common.hh"

class Replays{
    struct Replay{
        vector<int> sockets;
        vector<string> players_moves;
    };

    vector<Replay> _replays;

public:
    void get_replays(const int participant_socket, vector<vector<string>>& accessible_replays){
        for(size_t i=0; i < _replays.size(); i++){
            for(size_t j=0; j < _replays.at(i).sockets.size(); j++){
                if(participant_socket == _replays.at(i).sockets.at(j)){
                    accessible_replays.push_back(_replays.at(i).players_moves);
                    break;
                }
            }
        }
    }

    void add_replay(const vector<int> participants_socket, const vector<string> players_moves){
        _replays.push_back(Replay{.sockets=participants_socket, .players_moves=players_moves});
    }

    bool is_sunk_move(const vector<string> &tokens){
        if (tokens.size() == 0){
            perror("no tokens in replay\n");
            return false;
        }
        return (tokens.at(0) == TOKEN_SEND_FIRE || tokens.at(0) == TOKEN_RECV_FIRE)
                && tokens.at(3) == "4";
    }



};