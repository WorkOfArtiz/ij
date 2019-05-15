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

u16 swap_endianess(u16 x);
u32 swap_endianess(u32 x);
