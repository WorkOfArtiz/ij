#ifndef COMPILE_IJVM_HPP
#define COMPILE_IJVM_HPP
#include <backends/assembler.hpp>
#include <util/buffer.hpp>

void ijvm_compile(Buffer &b, Assembler &a);

#endif