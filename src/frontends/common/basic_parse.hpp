#ifndef FE_COMMON_PARSE
#define FE_COMMON_PARSE
#include "lexer.hpp"
#include "parse_error.hpp" // Integer out of bounds throws parse error
#include <util/types.h>

std::string parse_identifier(Lexer &l);

i32 parse_value(Lexer &l, long min, long max);

#endif // FE_COMMON_PARSE