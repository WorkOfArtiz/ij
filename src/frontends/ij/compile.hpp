#ifndef IJ_COMPILE_HPP
#define IJ_COMPILE_HPP

#include <frontends/common/lexer.hpp>
#include <backends/assembler.hpp>

#include "data.hpp"
#include "parse.hpp"

/* compiles ij */
void ij_compile(Lexer &l, Assembler &a);

#endif