#pragma once
#include <cstring>
#include <string>
#include <unordered_map>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <utility>
#include <vector>
#include "board_control.hh"
#include "board_input.hh"
#include "console_board_input.hh"
#include "board_view.hh"
#include "network.hh"
#include "boat.hh"
#include "utils.hh"
#include "observers.hh"

using namespace std;

/** A mock game board used to test the display.
 *
 * The DummyBoard use 2D vectors of DummyBoard::Cell to store hardcoded sides of the
 * board. This is convenient for a mock game board, but maybe not for a real game.
 *
 * Since this is a temporary class, the code quality is low. */
class Board : public BoardView, public BoardControl {
    /** The cell type and an optional ship identifier */
    class Cell {
        CellType _type{WATER};
        optional<int> _ship_id{nullopt};

public:
        /** Default constructor: creates an unkown/Water cell */
        constexpr Cell() = default;

        constexpr Cell(CellType type, optional<int> ship_id)
                : _type{type}, _ship_id{forward < optional < int >> (ship_id)} {
            if (type & IS_SHIP && !_ship_id) {
                throw logic_error("Cell with ship but no ship_id");
            } else if (!(type & IS_SHIP) && _ship_id) {
                throw logic_error("Cell without ship but with ship_id");
            }
        }

        [[nodiscard]] constexpr inline CellType

        type() const { return _type; }

        [[nodiscard]] constexpr inline optional<int>

        shipId() const { return _ship_id; }
    };
private:
    bool _my_turn{true};
    bool _is_finished{false};
    bool _is_victory{false};
    //buffer for the last fire target, to send it to the other player
    BoardCoordinates _last_fire_target{0, 0};
    vector <Boat> _boats;
    vector <Boat> _enemy_boats;

    //vector containing my side
    vector <vector<Cell>> _my_side{
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
    };
    //Vector containing the hidden side of the board
    vector <vector<Cell>> _their_hidden_side{
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
            {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}},
    };


    [[nodiscard]] Cell get(const bool my_side, const BoardCoordinates position) const {
        if (my_side) {
            return _my_side.at(position.y()).at(position.x());
        } else {
            return _their_hidden_side.at(position.y()).at(position.x());
        }
    }

    [[nodiscard]] optional<int> shipId(const bool my_side, const BoardCoordinates position) const {
        return get(my_side, position).shipId();
    }

    [[nodiscard]] bool check() const {
        if (_my_side.size() != _their_hidden_side.size()) {
            return false;
        }
        for (size_t i = 0; i < height(); ++i) {
            if (_my_side.at(i).size() != width() || _their_hidden_side.at(i).size() != width()) {
                return false;
            }
        }
        return true;
    }

    void parse_cell(string& token, const CellType type){
        /*
        Add cellType to client tokens 
        */
        string token_to_add{};
        switch (type) {
            case WATER: // " "
                token_to_add = "0";
                break;
            case OCEAN: // "X"
                token_to_add = "1";
                break;
            case UNDAMAGED: // "█"
                token_to_add = "2";
                break;
            case HIT: // "▒"
                token_to_add = "3";
                break;
            case SUNK: // "░"
                token_to_add = "4";
                break;
            default:
                throw NotImplementedError("GridDisplay parse_cell() unknown CellType");
        }
        add_token(token, token_to_add);
    }
    void send_fire_tokens(client_data& client, client_data& client2, const string x, const string y, const CellType cell_type,
        const bool is_player1, vector<string>& players_moves, Observers& observers);

    void parse_action_tokens(client_data& client, client_data& client2, const string x, const string y, const CellType cell_type);

    void send_fire_observers(const bool is_player1, const string x, const string y, const BoardView::CellType cell_type,
                             Observers& observers, vector<string>& players_moves);
    
    virtual bool pos_okay(Boat *boat, client_data& client);

    bool boat_in_frame(Boat *boat, client_data& client);

    bool not_overlapping(Boat *boat, client_data& client);

    bool check_neighbours(Boat *boat, client_data& client);

    virtual Boat *input_placement(const vector<int> boats_to_place, Timer& timer, client_data& client);

    Boat *find_boat(const int boat_id);

    Boat *find_enemy_boat(const int boat_id);

    bool check_if_sunk(Boat *boat);

    void set_sunk_boat(Boat *boat);

    void add_boat(Boat *boat, const bool my_side);

    void send_boat_cell_tokens(client_data& client, const string boat_cell_x, const string boat_cell_y, const string player_turn, vector<string>& players_moves);

    void send_boat_to_client(client_data& client, Boat* boat, vector<string>& players_moves, const bool is_player1);
    
public:
    Board() {
        if (!check()) {
            cerr << "The hard-coded DummyBoard is not valid" << endl;
            abort();
        }
    }

    Board(const Board &) = delete;

    Board(Board &&) = delete;

    Board &operator=(const Board &) = delete;

    Board &operator=(Board &&) = delete;

    void set_their_hidden_side(vector <vector<Cell>> their_hidden_side){_their_hidden_side=their_hidden_side;}

    vector <vector<Cell>> get_my_side() { return _my_side; }

    /**
     * @brief Invert the turn of the player and check if the game is finished
    */
    void invert_my_turn() {_my_turn = !_my_turn;}

    [[nodiscard]] bool myTurn() const override { return _my_turn; }

    [[nodiscard]] bool isFinished() const override { return _is_finished; }

    [[nodiscard]] bool isVictory() const override { return _is_victory; }

    [[nodiscard]] size_t width() const override { return _my_side.at(0).size(); }

    [[nodiscard]] size_t height() const override { return _my_side.size(); }

    [[nodiscard]] BoardCoordinates last_fire_target() const { return _last_fire_target; }

    [[nodiscard]] CellType cellType(bool my_side,BoardCoordinates position) const override {
        return get(my_side, position).type();}

    vector <Boat> get_boats() { return _boats; }

    template<typename T>
    T optional_to_value(const optional <T> &optionalValue, const T &defaultValue = T()) {
        return optionalValue.has_value() ? optionalValue.value() : defaultValue;
    }

    [[nodiscard]] bool fire(const BoardCoordinates position, client_data& client, client_data& client2, vector<string>& players_moves,
                            const bool is_player1, Observers& observers) override {
        if (cellType(false, position) & BoardView::IS_KNOWN) {
            return false;  // Invalid target
        }else {
            _last_fire_target = position;
            send_fire(position, client, client2, players_moves, is_player1, observers);
            return true;
        }
    }

    virtual void send_fire(const BoardCoordinates position, client_data& client, client_data& client2, vector<string>& players_moves, 
                          const bool is_player1, Observers& observers);

    virtual void receive_fire(const BoardCoordinates position);

    void place_enemy_boats(vector <Boat> _boats);

    virtual bool place_all_boats(shared_ptr <ConsoleBoardInput> display, client_data& client,
                                 vector<string>& players_moves, const bool is_player1, Timer& timer);

    void check_victory();

    void quit() override {
        _is_finished = true;
        _is_victory = false;
    }

    ~Board() override = default;
};
