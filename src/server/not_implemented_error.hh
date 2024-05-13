#pragma once

#include <stdexcept>
#include <string>

using namespace std;

/** Exception class to indicate that a feature has not been (yet) implemented */
class NotImplementedError : public logic_error {
public:
    explicit NotImplementedError(const string& what_arg) : logic_error("Not implemented: " + what_arg) {}
};
