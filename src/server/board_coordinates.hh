#pragma once

#include <optional>
#include <string>
#include "not_implemented_error.hh"

/** A pair of 0-indexed board coordinates.
 *
 * {0, 0} is top-left.
 *
 * NOTE: This is not the coordinates of a pixel on the screen. */
class BoardCoordinates final {
    size_t _x;
    size_t _y;

public:
    constexpr BoardCoordinates(size_t x, size_t y) : _x{x}, _y{y} {}

    [[nodiscard]] constexpr inline size_t x() const { return _x; }
    [[nodiscard]] constexpr inline size_t y() const { return _y; }

    void set(const size_t x, const size_t y) {
        _x = x;
        _y = y;
    }

    /** Whether c is in [A-Za-z] */
    constexpr static bool isalpha(const char c) {
        // Contrary to isalpha, works with char and not modified by locale
        return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z');
    }

    /** "A" -> 0, "B" -> 1, ..., "AA" -> 26, ...
    * Any non alpha character (as defined by BoardCoordinates::isalpha) produces a null
    * result. */
    [[nodiscard]] static optional<size_t> parseX(const string& x_string) {
        if (x_string.empty()) {
            return nullopt;
        }

        size_t result{0};
        for (char c : x_string) {
            if (!isalpha(c)) {
                return nullopt;
            }
            result = result * 26 + static_cast<unsigned>(toupper(c) - 'A' + 1);
        }
        return result - 1;
    }

    /** "1" -> 0, "2" -> 1, ...
    * Uses stoull. Return null if stoull throws an exception. */
    [[nodiscard]] static optional<size_t> parseY(const string& y_string) {
        const int BASE{10};
        try {
            unsigned long long parsed = stoull(y_string, nullptr, BASE);
            if (parsed == 0) {
                return nullopt;
            }
            return parsed - 1;
        } catch (const logic_error&) {
            return nullopt;
        }
    }

    /** {0, 0} returns "A1" */
    [[nodiscard]] inline string toString() const { return xToString() + yToString(); }

    /** returns the x / letter part of toString() */
    [[nodiscard]] inline string xToString() const {
        string result{};
        size_t n = _x + 1;

        while (n > 0) {
            result = static_cast<char>('A' + (n - 1) % 26) + result;
            n      = (n - 1) / 26;
        }
        return result;
    }
    /** returns the y / number part of toString() */
    [[nodiscard]] inline string yToString() const { return to_string(_y + 1); }
};

/** Put bc.toString() on os */
ostream& operator<<(ostream& os, const BoardCoordinates& bc);

/** Extract bc from os */
istream& operator>>(istream& is, BoardCoordinates& bc);
