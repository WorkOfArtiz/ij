#include "endian.hpp"

Endian sys_endianess = determine_endianess();

Endian determine_endianess()
{
    union indicator {
        char array[4];
        int  value;
    } i = {1, 0, 0, 0};

    if (i.value == 1)
        return Endian::Little;
    else
        return Endian::Big;
}

u16 swap_endianess(u16 x)
{
    union swapper {char b[2]; u16 value; } s1, s2;
    s1.value = x;
    s2 = {s1.b[1], s1.b[0]};
    return s2.value;
}

u32 swap_endianess(u32 x)
{
    union swapper {char b[4]; u32 value; } s1, s2;
    s1.value = x;
    s2 = {s1.b[3], s1.b[2], s1.b[1], s1.b[0]};
    return s2.value;
}
