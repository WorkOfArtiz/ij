#ifndef COMPILE_JAS_HPP
#define COMPILE_JAS_HPP
#include <util/buffer.hpp>
#include <frontends/common/lexer.hpp>
#include <backends/assembler.hpp>

void jas_compile(Lexer &l, Assembler &a);

#endif