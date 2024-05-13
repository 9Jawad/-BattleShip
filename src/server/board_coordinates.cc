#include <iostream>
#include "board_coordinates.hh"

ostream& operator<<(ostream& os, const BoardCoordinates& bc) {
    os << bc.toString();
    return os;
}

istream& operator>>(istream& is, BoardCoordinates& bc) {
    string x_string{};
    while (is.good() && !is.eof() && BoardCoordinates::isalpha(static_cast<char>(is.peek()))) {
        char c{'?'};
        is.get(c);
        x_string += c;
    }

    string y_string{};
    while (is.good() && !is.eof() && isdigit(static_cast<unsigned char>(is.peek()))) {
        char c{'?'};
        is.get(c);
        y_string += c;
    }

    optional<size_t> x{BoardCoordinates::parseX(x_string)};
    optional<size_t> y{BoardCoordinates::parseY(y_string)};

    if (x && y) {
        bc.set(x.value(), y.value());
    } else {
        is.setstate(ios_base::failbit);
    }
    return is;
}
