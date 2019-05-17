#pragma once
#include "types.h"

enum class Endian 
{
    Little,
    Big
};

/*******************************************************************************
 * Endianess bs
 ******************************************************************************/
Endian determine_endianess();
extern Endian sys_endianess;

template<typename T>
static inline T swap_endianess(T x)
{
    union {
        u8 store[sizeof(x)];
        T  value;
    } from, to;

    from.value = x;
    for(size_t i = 0;i < sizeof(x); i++)
        to.store[i] = from.store[sizeof(x) - i - 1];
    
    return to.value;
}
