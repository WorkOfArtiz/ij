#include "util.hpp"

/* Fills in the compile functions of the various statements */
bool in(std::string needle, std::initializer_list<std::string> hay) {
    for (auto straw : hay)
        if (straw == needle)
            return true;

    return false;
}