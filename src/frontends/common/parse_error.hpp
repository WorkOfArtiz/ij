#ifndef EC_PARSE_ERROR
#define EC_PARSE_ERROR
#include "lexer.hpp"

/*
 * A very simple parse error class to create parser errors
 */

class parse_error : public std::runtime_error {
public:
  parse_error(Token &t, std::string msg);

private:
  std::string make_what(Token &t, std::string msg);
};


#endif
