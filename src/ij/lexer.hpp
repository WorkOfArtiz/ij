#ifndef LEXER_H
#define LEXER_H
#include <string>
#include <iostream>
#include <vector>
#include <set>
#include <fstream>
#include <sstream>

using std::string;

/*
 * A lexer built for very basic parsing, it recognises:
 * - integers    :
 *   decimals:     \d+
 *   hexadecimals: 0x[a-fA-F\d]+
 *   character lits: "'" [ -~] "'"
 * - identifiers : [_A-Za-z$]\w+
 * - operators: + - & | * / < > = and their extensions += -= &= |= *= /= <= >=
 * ==
 * - comments // bla
 */

enum class TokenType {
    Decimal,
    Hexadecimal,
    Character_literal,
    Identifier,
    Keyword,
    Operator,
    Whitespace,
    BracesOpen,
    BracesClose,
    BlockOpen,
    BlockClose,
    CurlyOpen,
    CurlyClose,
    Comma,
    SemiColon,
    Colon,
    Nl,
    StringLiteral,
    Comment
};

/* make it printable */
std::ostream &operator<<(std::ostream &o, const TokenType &t);

struct Token {
    Token(string value, TokenType type, string name, size_t line, size_t srow,
          size_t erow)
        : value{value}, type{type}, name{name}, line{line}, srow{srow},
          erow{erow} {}
    Token(const Token &t)
        : value{t.value}, type{t.type}, name{t.name}, line{t.line},
          srow{t.srow}, erow{t.erow} {}
    Token(Token &&t)
        : value{t.value}, type{t.type}, name{t.name}, line{t.line},
          srow{t.srow}, erow{t.erow} {}

    string value;   /* the actual token */
    TokenType type; /* type for early processing */

    /* for proper parsing errors */
    string name;
    size_t line;
    size_t srow;
    size_t erow;
};

std::ostream &operator<<(std::ostream &o, const Token &t);

struct Source {
    string name;
    size_t line;
    size_t col;
    std::ifstream *src;

    Source(std::string path, std::string prev_path);
    Source(Source &&old);
    ~Source();

    /* reads character from source, updating its internals */
    int getchar();

    /* reads character without updating */
    int peekchar();

    bool eof();
};

class lexer_error : public std::runtime_error {
  public:
    lexer_error(Source &c, std::string msg);
    std::string make_what(Source &c, std::string msg);
};

class Lexer {
  public:
    Lexer();

    /* attaching a source to the lexer */
    void add_source(string file_path);

    bool has_token(); /* tries to see if there's something next */
    Token get();      /* get token, update internals */
    Token &peek();    /* get token, keep internals */
    void discard();   /* just discard the next one, saves you from discarding */

    /* check if the next has a particular type or set of values */
    bool is_next(TokenType t);
    bool is_next(TokenType t, std::string value);
    bool is_next(TokenType t, std::initializer_list<std::string> values);

    void set_skip(std::initializer_list<TokenType> types);
    void set_keywords(std::initializer_list<std::string> keywords);

  private:
    bool has_symbol();
    void read_token(); /* reads token and appends to back of cache */

    std::vector<Source> srcs;
    std::vector<Token> cache;
    std::set<TokenType> skip_list;
    std::set<std::string> keywords;
};

#endif
