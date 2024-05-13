#pragma once

#include <chrono>
#include "network.hh"
#include "not_implemented_error.hh"

class Timer{
    unsigned int _timer_turn, _timer_game; // How long a turn/game lasts in seconds
    unsigned int _remaining_time_game;
    chrono::steady_clock::time_point _begin_turn, _begin_game; // Starting point of the timer
    bool _timer_turn_started = false;
    unsigned int _time_lapse = 0; // Keeps track of the elapsed time ( Only for displaying purposes )


public:
    Timer(unsigned int timer_turn = 30, unsigned int timer_game = 120) :
        _timer_turn{timer_turn} , _timer_game{timer_game}, _remaining_time_game{timer_game} {}

    unsigned int get_timer_turn() const {return _timer_turn;}

    unsigned int get_turn_elapsed_time() const {
        // Return elapsed time during turn in seconds
        chrono::steady_clock::time_point end = chrono::steady_clock::now();
        return chrono::duration_cast<chrono::milliseconds>(end - _begin_turn).count() / 1000;
    }

    void start_timer_turn(){
        // Starts turn timer if it has not started yet
        if(!_timer_turn_started){
            _begin_turn = chrono::steady_clock::now();
            _time_lapse = 0;
            _timer_turn_started = true;
        }
    }

    void end_timer_turn(const client_data client){
        // Ends turn timer, deducts it from remaining time and sends it to the client
        _time_lapse = 0;
        _timer_turn_started = false;
        unsigned int elapsed_time = get_turn_elapsed_time();
        elapsed_time > _timer_turn ? deduct_turn_time(_timer_turn) : deduct_turn_time(elapsed_time);
        send_remaining_time(client);
    }

    bool check_timer_turn(const client_data client, const string time_out_msg = "Time's up! Your turn is over.\n") {
        const unsigned int interval = 15;
        // Return true when _timer_turn has passed (timed out)
        unsigned int elapsed_time = get_turn_elapsed_time();
        // Sends to the client the elapsed time in intervals
        if (elapsed_time / interval > _time_lapse) {
            string remaining_time = "Elapsed time: " + to_string(elapsed_time) + "/" +
                    to_string(_timer_turn)+ " seconds\n" ;
            send_prnt_token(remaining_time, client.client_sock);
            _time_lapse++;
        }
        if (elapsed_time > _timer_turn) {
            send_prnt_token(time_out_msg, client.client_sock);
            return true;
        }
        return false;
    }

    bool check_timer_game(const client_data client) {
        // Return true when _timer_game has passed (timed out)
        if (get_turn_elapsed_time() > _remaining_time_game) {
            send_prnt_token("Your time's up! Game over.\n", client.client_sock);
            return true;
        }
        return false;
    }

    bool has_timer_game(){
        return _remaining_time_game > 0;
    }

    void send_remaining_time(const client_data client) const {
        unsigned int minutes = _remaining_time_game / 60;
        unsigned int seconds = _remaining_time_game % 60;
        char message[BUFFER_SIZE]{'\0'};
        sprintf(message, "\nYou have %u'%u\" left for this game.\n", minutes, seconds);
        string str_message {message};
        send_prnt_token(str_message,client.client_sock);
    }

    unsigned int get_remaining_time_game(){
        return _remaining_time_game;
    }
    void deduct_turn_time(const unsigned int turn_time){
        // Deducts the time the turn lasted from the remaining time of the game
        if (static_cast<int>(_remaining_time_game) - static_cast<int>(turn_time) < 0) {
            _remaining_time_game = 0;
        } else {
            _remaining_time_game -= turn_time;
        }
    }

};