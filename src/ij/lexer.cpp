#include "lexer.hpp"
#include "../logger.hpp"
#include <sstream>
#include <fstream>
#include <cassert>
#include <cctype>

/*******************************************************************************
 * TokenType 
 *****************************************************************************/
std::ostream &operator<<(std::ostream &o, const TokenType &t)
{
    switch (t)
    {
        case TokenType::Decimal:           return o << "Decimal";
        case TokenType::Hexadecimal:       return o << "Hexadecimal";
        case TokenType::Character_literal: return o << "Character_literal";
        case TokenType::Identifier:        return o << "Identifier";
        case TokenType::Keyword:           return o << "Keyword";
        case TokenType::Operator:          return o << "Operator";
        case TokenType::Whitespace:        return o << "Whitespace";
        case TokenType::BracesOpen:        return o << "BracesOpen";
        case TokenType::BracesClose:       return o << "BracesClose";
        case TokenType::CurlyOpen:         return o << "CurlyOpen";
        case TokenType::CurlyClose:        return o << "CurlyClose";
        case TokenType::Comma:             return o << "Comma";
        case TokenType::SemiColon:         return o << "SemiColon";
        case TokenType::Nl:                return o << "Nl";
        case TokenType::Comment:           return o << "Comment";
    }

    throw std::runtime_error{"Unsupported tokentype"};
}

/*******************************************************************************
* Token 
*******************************************************************************/
std::ostream &operator<<(std::ostream &o, const Token &t)
{
    if (t.type == TokenType::Nl)
        return o << "Token<" << t.type << ">(\"\\n\")";

    return o << "Token<" << t.type << ">(\"" << t.value << "\")"; 
}

/*******************************************************************************
* Source 
*******************************************************************************/
Source::Source(std::string path)
    : name{path}, line{1}, col{0}, src{new std::ifstream()}
{
    src->open(path, std::ifstream::in);
    if (!src->is_open())
        log.panic("Couldn't open file %s", path.c_str());
}

Source::Source(Source &&old)
    : name{old.name}, line{old.line}, col{old.col}, src{old.src}
{
    old.src = nullptr;
}

Source::~Source()
{
    if (!src)
        return;

    src->close();
    delete src;
}

int Source::getchar()
{
    int c = src->get();

    if (c == '\n')
    {
        line++;
        col = 0;
    }
    else
        col++;

    if (c < 1 || c > '~')
        return -1;

    return c;
}

int Source::peekchar()
{
    int c = src->peek();

    if (c < 1 || c > '~')
        return -1;

    return c;
}

bool Source::eof()
{
    return src->eof() || src->peek() == EOF;
}

/*******************************************************************************
* Lexer Error handling 
*******************************************************************************/
lexer_error::lexer_error(Source &c, std::string msg) 
    : std::runtime_error{make_what(c, msg)}
{}

std::string lexer_error::make_what(Source &c, std::string msg)
{
    std::stringstream what;

    what << "Lexer error ";
    what << c.name << ":" << c.line << "@" << c.col;
    what << " " << msg;
    return what.str();
}

/*******************************************************************************
 * Lexer
 ******************************************************************************/
Lexer::Lexer()
{
    skip_list.clear();
    keywords.clear();
}


void Lexer::add_source(string file_path)
{
    srcs.emplace_back(file_path);
}

void Lexer::read_token()
{
    std::stringstream builder;

    /* pop off empty srcs */
    if (!has_symbol())
        throw std::runtime_error{"lexer tried reading token but nothing left to parse"};

    Source &src = srcs.back();

    string sn = src.name;
    size_t ln = src.line;
    size_t cb = src.col;

    int c = src.getchar();
    if (c < 1)
        throw std::runtime_error{"Couldn't read from file"};

    /* push first into builder */
    builder << static_cast<char>(c);

    /* comment */
    if (c == '/' && src.peekchar() == '/')
    {
        // get rid of extra /
        src.getchar();

        while (!src.eof() && src.peekchar() != '\n')
            builder << static_cast<char>(src.getchar());
        
        cache.emplace_back(builder.str(), TokenType::Comment, sn, ln, cb, src.col);
        return;
    }

    /* newline */
    if (c == '\n')
    {
        cache.emplace_back(builder.str(), TokenType::Nl, sn, ln, cb, cb+1);
        return;
    }

    /* scan whitespace */
    if (std::isspace(c))
    {
        while (std::isspace(src.peekchar()) && src.peekchar() != '\n')
            builder << static_cast<char>(src.getchar());

        cache.emplace_back(builder.str(), TokenType::Whitespace, sn, ln, cb, src.col);
        return;
    }

    /* identifier */
    if (std::isalpha(c) || c == '_')
    {
        while (std::isalnum(src.peekchar()) || src.peekchar() == '_')
            builder << static_cast<char>(src.getchar());

        std::string value = builder.str();
        TokenType type = keywords.count(value) ? TokenType::Keyword : TokenType::Identifier;
        cache.emplace_back(value, type, sn, ln, cb, src.col);
        return;
    }

    /* character literals */
    if (c == '\'')
    {
        builder << static_cast<char>(src.peekchar());
        if (src.getchar() == '\\')
            builder << static_cast<char>(src.getchar());

        builder << static_cast<char>(src.peekchar());
        if (src.getchar() != '\'')
            throw lexer_error{src, "Character literal wasn't terminated"};
        
        cache.emplace_back(builder.str(), TokenType::Character_literal, sn, ln, cb, src.col);
        return;
    }

    /* integers */
    if (std::isdigit(c))
    {
        if (c == '0' && src.peekchar() == 'x')
        {
            builder << static_cast<char>(src.getchar());

            while (std::isxdigit(src.peekchar()))
                builder << static_cast<char>(src.getchar());
            
            cache.emplace_back(builder.str(), TokenType::Hexadecimal, sn, ln, cb, src.col);
            return;
        }

        while (std::isdigit(src.peekchar()))
            builder << static_cast<char>(src.getchar());
        
        cache.emplace_back(builder.str(), TokenType::Decimal, sn, ln, cb, src.col);
        return;
    }

    /* operators */
    string operators = "+-*/&<>=";
    if (operators.find(c) != std::string::npos)
    {
        if (src.peekchar() == '=')
            builder << static_cast<char>(src.getchar());

        cache.emplace_back(builder.str(), TokenType::Operator, sn, ln, cb, src.col);
        return;
    }

    TokenType t;
    switch(c)
    {
        case ';': t = TokenType::SemiColon; break;
        case ',': t = TokenType::Comma; break;
        case '{': t = TokenType::CurlyOpen; break;
        case '}': t = TokenType::CurlyClose; break;
        case '(': t = TokenType::BracesOpen; break;
        case ')': t = TokenType::BracesClose; break;
        default:
            //std::cout << "ERROR SYMBOL: " << builder.str() << std::endl;
            throw lexer_error{src, "can't identify symbol '" +  builder.str() + "'"};
    }

    cache.emplace_back(builder.str(), t, sn, ln, cb, src.col);
}


Token &Lexer::peek()
{
    while (cache.empty())
    {
        read_token();

        if (skip_list.count(cache.back().type) == 1)
            cache.pop_back();
    }

    return cache.back();
}

Token Lexer::get()
{
    Token result = peek();
    cache.pop_back();

    return result;
}

bool Lexer::has_symbol()
{
    while (!srcs.empty() && srcs.back().eof())
        srcs.pop_back();

    return !srcs.empty();    
}

bool Lexer::has_token()
{
    while (true)
    {
        if (cache.empty() && !has_symbol())
            return false;
        
        if (cache.empty())
            read_token();
        
        if (skip_list.count(cache.back().type) == 0)
            break;

        cache.pop_back();
    }

    return true;
}

void Lexer::discard()
{
    peek(); /* ensure theres something in the cache */
    cache.pop_back();
}

bool Lexer::is_next(TokenType t)
{
    return peek().type == t;
}

bool Lexer::is_next(TokenType t, std::string value)
{
    Token &tok = peek();

    return tok.type == t && tok.value == value;
}

bool Lexer::is_next(TokenType t, std::initializer_list<std::string> values)
{
    Token &tok = peek();

    if (tok.type != t)
        return false;

    for (auto value : values)
        if (tok.value == value)
            return true;

    return false;
}



void Lexer::set_skip(std::initializer_list<TokenType> types)
{
    skip_list.clear();
    skip_list.insert(types.begin(), types.end());
}

void Lexer::set_keywords(std::initializer_list<std::string> keywords)
{
    this->keywords.clear();
    this->keywords.insert(keywords.begin(), keywords.end());
}