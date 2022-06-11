#include "compile.hpp"

#include <frontends/common/basic_parse.hpp>
#include <util/logger.hpp>
#include <util/util.hpp>
#include <util/types.h>

static void parse_constant_block(Lexer &l, Assembler &a)
{
    l.discard();

    while (l.is_next(TokenType::Identifier)) {
        std::string constant = parse_identifier(l);
        i32 constant_value = parse_value(l, INT32_MIN, INT32_MAX);
        a.constant(constant, constant_value);
    }

    l.expect(TokenType::Period, true);
    l.expect(TokenType::Keyword, "end", true);
    l.expect(TokenType::Operator, "-", true);
    l.expect(TokenType::Keyword, "constant", true);
}

static void parse_optional_vars(Lexer &l, std::vector<std::string> &vars)
{
    if (l.is_next(TokenType::Period)) {
        l.discard();
        l.expect(TokenType::Keyword, "var", true);

        while (l.is_next(TokenType::Identifier))
            vars.push_back(l.get().value);

        l.expect(TokenType::Period, true);
        l.expect(TokenType::Keyword, "end", true);
        l.expect(TokenType::Operator, "-", true);
        l.expect(TokenType::Keyword, "var", true);
    }
}

static void parse_jas_op(Lexer &l, Assembler &a)
{
    Token t = l.get();
    string val = t.value;

    // clang-format off
         if (val == "BIPUSH")        a.BIPUSH(parse_value(l, I8_MIN, I8_MAX));
    else if (val == "DUP")           a.DUP();
    else if (val == "ERR")           a.ERR();
    else if (val == "GOTO")          a.GOTO(parse_identifier(l));
    else if (val == "HALT")          a.HALT();
    else if (val == "IADD")          a.IADD();
    else if (val == "IAND")          a.IAND();
    else if (val == "IFEQ")          a.IFEQ(parse_identifier(l));
    else if (val == "IFLT")          a.IFLT(parse_identifier(l));
    else if (val == "ICMPEQ")        a.ICMPEQ(parse_identifier(l));
    else if (val == "IF_ICMPEQ")     a.ICMPEQ(parse_identifier(l));
    else if (val == "ILOAD")         a.ILOAD(parse_identifier(l));
    else if (val == "IN")            a.IN();
    else if (val == "INVOKEVIRTUAL") a.INVOKEVIRTUAL(parse_identifier(l));
    else if (val == "IOR")           a.IOR();
    else if (val == "IRETURN")       a.IRETURN();
    else if (val == "ISTORE")        a.ISTORE(parse_identifier(l));
    else if (val == "ISUB")          a.ISUB();
    else if (val == "LDC_W")         a.LDC_W(parse_identifier(l));
    else if (val == "NOP")           a.NOP();
    else if (val == "OUT")           a.OUT();
    else if (val == "POP")           a.POP();
    else if (val == "SWAP")          a.SWAP();
    else if (val == "WIDE")          a.WIDE();
    else if (val == "NEWARRAY")      a.NEWARRAY();
    else if (val == "IALOAD")        a.IALOAD();
    else if (val == "IASTORE")       a.IASTORE();
    else if (val == "NETBIND")       a.NETBIND();
    else if (val == "NETCONNECT")    a.NETCONNECT();
    else if (val == "NETIN")         a.NETIN();
    else if (val == "NETOUT")        a.NETOUT();
    else if (val == "NETCLOSE")      a.NETCLOSE();
    else if (val == "SHL")           a.SHL();
    else if (val == "SHR")           a.SHR();
    else if (val == "IMUL")          a.IMUL();
    else if (val == "IDIV")          a.IDIV();
    else if (val == "IINC") {
        std::string name = parse_identifier(l);
        a.IINC(name, parse_value(l, I8_MIN, I8_MAX));
    }
    else
        throw parse_error(t, concat("Expected JAS OP code, found: '", val, "'"));
    // clang-format on
}

static void parse_method(Lexer &l, Assembler &a)
{
    bool main = false;
    std::string              name;
    std::vector<std::string> args;
    std::vector<std::string> vars;

    // parse .main and .main()
    if (l.is_next(TokenType::Keyword, "main")) {
        l.discard();
        main = true;
        name = "main";

        if (l.is_next(TokenType::BracesOpen)) {
            l.expect(TokenType::BracesOpen, true);
            l.expect(TokenType::BracesClose, true);
        }
    }
    // parse .method name(arg1, arg2, ..., argn)
    else {
        l.expect(TokenType::Keyword, "method", true);
        l.expect(TokenType::Identifier);
        name = l.get().value;
        l.expect(TokenType::BracesOpen, true);

        if (l.is_next(TokenType::Identifier)) {
            args.push_back(l.get().value);

            while (l.is_next(TokenType::Comma)) {
                l.discard();
                l.expect(TokenType::Identifier);
                args.push_back(l.get().value);
            }
        }

        l.expect(TokenType::BracesClose, true);
    }

    // parse .var block
    parse_optional_vars(l, vars);

    log.info("name: %s", name.c_str());
    log.info("args: %s", cstr(join(", ", args)));
    log.info("vars: %s", cstr(join(", ", vars)));
    a.function(name, args, vars);

    while (l.is_next(TokenType::Identifier) || l.is_next(TokenType::Keyword)){
        // labels are identifier followed by :
        if (l.is_next(TokenType::Identifier)) {
            string labelname = parse_identifier(l);
            l.expect(TokenType::Colon, true);
            a.label(labelname);
        }
        // Everything else should be an op
        else
            parse_jas_op(l, a);
    }

    l.expect(TokenType::Period, true);
    l.expect(TokenType::Keyword, "end", true);
    l.expect(TokenType::Operator, "-", true);

    if (main)
        l.expect(TokenType::Keyword, "main", true);
    else
        l.expect(TokenType::Keyword, "method", true);

    log.success("Successfully parsed method %s", name.c_str());
}

void jas_compile(Lexer &l, Assembler &a)
{
    l.set_skip({TokenType::Whitespace, TokenType::Nl, TokenType::Comment});
    l.set_keywords({
        "constant", "main", "method", "var", "end",
        "BIPUSH",        "DUP",           "ERR",           "GOTO",
        "HALT",          "IADD",          "IAND",          "IFEQ",
        "IFLT",          "ICMPEQ",        "IF_ICMPEQ",     "ILOAD",
        "IN",            "INVOKEVIRTUAL", "IOR",           "IRETURN",
        "ISTORE",        "ISUB",          "LDC_W",         "NOP",
        "OUT",           "POP",           "SWAP",          "WIDE",
        "IINC",          "NEWARRAY",      "IALOAD",        "IASTORE",
        "GC",            "NETBIND",       "NETCONNECT",    "NETIN",
        "NETOUT",        "NETCLOSE",      "SHL",           "SHR",
        "IMUL",          "IMUL"
    });

    while (l.has_token() && l.is_next(TokenType::Period)) {
        l.expect(TokenType::Period, true);
        l.expect(TokenType::Keyword, {"constant", "main", "method"});

        if (l.is_next(TokenType::Keyword, "constant")) {
            parse_constant_block(l, a);
        }
        else if (l.is_next(TokenType::Keyword, "main")) {
            parse_method(l, a);
        }
        else if (l.is_next(TokenType::Keyword, "method")) {
            parse_method(l, a);
        }
    }
}