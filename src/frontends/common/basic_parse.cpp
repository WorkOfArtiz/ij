#include "basic_parse.hpp"
#include <util/util.hpp>

/* basic parts */
std::string parse_identifier(Lexer &l) {
    l.expect(TokenType::Identifier);
    return l.get().value;
}

i32 parse_value(Lexer &l, long min, long max) {
    bool sign = false;
    i32 res;

    if (l.is_next(TokenType::Operator, "-")) {
        sign = true;
        l.discard();
    }

    l.expect({TokenType::Decimal, TokenType::Hexadecimal,
               TokenType::Character_literal});
    if (l.is_next(TokenType::Decimal) or l.is_next(TokenType::Hexadecimal)) {
        try {
            res = std::stol(l.peek().value, nullptr, 0);
            if (res < min || res > max)
                throw std::out_of_range{""};
        } catch (std::out_of_range &r) {
            throw parse_error{l.peek(), "number out of allowed range"};
        }

        l.discard();
    } else // Character literal
    {
        Token &t = l.peek();

        if (t.value[1] == '\\') {
            // clang-format off
            switch (t.value[2]) {
            case '"':  res = '"';   break;
            case '\\': res = '\\';  break;
            case '/':  res = '/';   break;
            case 'b':  res = '\b';  break;
            case 'f':  res = '\f';  break;
            case 'n':  res = '\n';  break;
            case 'r':  res = '\r';  break;
            case 't':  res = '\t';  break;
            // clang-format on
            default:
                throw parse_error{
                    t, sprint("Unrecognised escape symbol \\%s", t.value[2])};
            }
        } else
            res = t.value[1];

        l.discard();
    }

    return sign ? -res : res;
}