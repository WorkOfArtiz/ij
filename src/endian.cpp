#include "endian.hpp"

Endian sys_endianess = determine_endianess();

Endian determine_endianess() {
    union indicator {
        char array[4];
        int value;
    } i = {1, 0, 0, 0};

    if (i.value == 1)
        return Endian::Little;
    else
        return Endian::Big;
}