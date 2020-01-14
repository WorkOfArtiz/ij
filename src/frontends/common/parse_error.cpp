#include "parse_error.hpp"

parse_error::parse_error(Token &t, std::string msg)
    : std::runtime_error{make_what(t, msg)} {}

std::string parse_error::make_what(Token &t, std::string msg) {
    std::stringstream what;

    what << "Parse error ";
    what << t.name << ":" << t.line << "@" << t.srow;
    what << " found " << t.type << " '" << t.value << "', ";
    what << msg;

    return what.str();
}

