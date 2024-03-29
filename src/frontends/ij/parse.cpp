#include <set>
#include "parse.hpp"
#include <initializer_list>
#include <sstream>

#include <util/logger.hpp>
#include <util/util.hpp>

/* High level functions */
Program *parse_program(Lexer &l) {
    Program *res = new Program();
    std::set<std::string> constants{{"main"}};
    std::set<std::string> imports;

    l.set_skip({TokenType::Whitespace, TokenType::Nl, TokenType::Comment});
    l.set_keywords({"constant", "function", "import","var",   "for",
                    "while",    "if",       "else",  "label", "jas",
                    "break",    "continue", "return",   "$getc", "$putc",
                    "$print",   "$puts",    "$halt",    "$err",  "$malloc",
                    "$push",    "$pop"});

    while (l.has_token()) {
        l.expect(TokenType::Keyword, {"function", "constant", "import"});
        Token t = l.peek(); /* copy token */

        if (t.value == "import") {
            l.discard();
            std::string fn = l.get().value;
            if (imports.count(fn) == 0) {
                imports.insert(fn);
                l.add_source(fn);
            }
        }
        else if (t.value == "constant") {
            Constant *c = parse_constant(l);
            if (constants.count(c->name) == 1)
                throw parse_error{t,
                                  "constant " + c->name + " was defined twice"};

            constants.insert(c->name);
            res->consts.push_back(c);
        }
        else if (t.value == "function") {
            Function *f = parse_function(l);

            if (constants.count(f->name))
                throw parse_error{t,
                                  "constant " + f->name + " was defined twice"};

            res->funcs.push_back(f);
        }
    }

    return res;
}

Constant *parse_constant(Lexer &l) {
    l.expect(TokenType::Keyword, "constant", true);
    std::string name = parse_identifier(l);

    l.expect(TokenType::Operator, "=", true);
    int32_t value = parse_value(l, INT32_MIN, INT32_MAX);
    l.expect(TokenType::SemiColon, ";", true);

    return new Constant(name, value);
}

CompStmt *parse_compound_stmt(Lexer &l) {
    if (!l.is_next(TokenType::CurlyOpen))
        return new CompStmt({parse_statement(l)});

    l.expect(TokenType::CurlyOpen, true);

    std::vector<Stmt *> stmts;
    while (!l.is_next(TokenType::CurlyClose)) {
        if (l.is_next(TokenType::SemiColon))
            continue;

        stmts.push_back(parse_statement(l));
    }
    l.expect(TokenType::CurlyClose, true);

    return new CompStmt(stmts);
}

static CompStmt *parse_jas_block(Lexer &l) {
    l.expect(TokenType::CurlyOpen, true);

    std::vector<Stmt *> stmts;

    while (!l.is_next(TokenType::CurlyClose)) {
        if (l.is_next(TokenType::SemiColon))
            l.discard();
        else if (l.is_next(TokenType::Keyword, "var"))
            stmts.push_back(parse_var_stmt(l));
        else if (l.is_next(TokenType::Keyword, "label"))
            stmts.push_back(parse_label_stmt(l));
        else
            stmts.push_back(parse_jas_stmt(l));
    }

    l.expect(TokenType::CurlyClose, true);
    return new CompStmt(stmts);
}

static std::vector<std::string> parse_identifier_list(Lexer &l) {
    std::vector<std::string> args;

    if (l.is_next(TokenType::Identifier))
        args.push_back(parse_identifier(l));

    while (l.is_next(TokenType::Comma)) {
        l.discard(); // discard comma
        args.push_back(parse_identifier(l));
    }

    return args;
}

Function *parse_function(Lexer &l) {
    // parses
    // function <name>(<ident_list>) { <stmts> }
    // function <name>(<ident_list>) jas { [<var_stmt> | <label> | <jas_stmt>]*
    // }

    l.expect(TokenType::Keyword, "function", true); // function
    std::string fname = parse_identifier(l);         // <name>

    l.expect(TokenType::BracesOpen, true);                   // (
    std::vector<std::string> args = parse_identifier_list(l); // <ident_list>
    l.expect(TokenType::BracesClose, true);                  // )

    if (l.is_next(TokenType::Keyword, "jas")) {
        l.discard();
        return new Function(fname, args, parse_jas_block(l));
    } else
        return new Function(fname, args, parse_compound_stmt(l));
}

static Stmt *parse_magic_print(Lexer &l) {
    bool add_newline = l.is_next(TokenType::Keyword, "$puts");
    l.discard();

    l.expect(TokenType::BracesOpen, true);
    l.expect(TokenType::StringLiteral, false);
    std::string s = l.get().value;
    l.expect(TokenType::BracesClose, true);

    std::vector<Stmt *> stmts;
    for (const char &symbol : s) {
        stmts.push_back(JasStmt::BIPUSH(symbol));
        stmts.push_back(new JasStmt("OUT"));
    }

    if (add_newline) {
        stmts.push_back(JasStmt::BIPUSH('\n'));
        stmts.push_back(new JasStmt("OUT"));
    }
    return new CompStmt(stmts);
}

static Stmt *parse_magic_putc(Lexer &l) {
    std::vector<Stmt *> stmts;

    l.discard();
    l.expect(TokenType::BracesOpen, true);
    stmts.push_back(parse_expr_stmt(l, false));
    stmts.push_back(new JasStmt("OUT"));
    l.expect(TokenType::BracesClose, true);

    return new CompStmt(stmts);
}

static Stmt *parse_stop(Lexer &l) {
    Stmt *s =
        new JasStmt(l.is_next(TokenType::Keyword, "$err") ? "ERR" : "HALT");
    l.discard();
    l.expect(TokenType::BracesOpen, true);
    l.expect(TokenType::BracesClose, true);
    return s;
}

/* Functions have statements */
Stmt *parse_statement(Lexer &l) /* delegates to types of statements */
{
    if (l.is_next(TokenType::Keyword, "for"))
        return parse_for_stmt(l);

    if (l.is_next(TokenType::Keyword, "while"))
        return parse_while_stmt(l);

    if (l.is_next(TokenType::Keyword, "if"))
        return parse_if_stmt(l);

    if (l.is_next(TokenType::Keyword, "break"))
        return parse_break_stmt(l);

    if (l.is_next(TokenType::Keyword, "continue"))
        return parse_continue_stmt(l);

    Stmt *s;

    if (l.is_next(TokenType::Keyword, "var"))
        s = parse_var_stmt(l);
    else if (l.is_next(TokenType::Keyword, "return"))
        s = parse_ret_stmt(l);
    else if (l.is_next(TokenType::Keyword, {"$print", "$puts"}))
        s = parse_magic_print(l);
    else if (l.is_next(TokenType::Keyword, {"$halt", "$err"}))
        s = parse_stop(l);
    else if (l.is_next(TokenType::Keyword, "$putc"))
        s = parse_magic_putc(l);
    else if (l.is_next(TokenType::Keyword, "$push")) {
        l.discard();
        l.expect(TokenType::BracesOpen, true);
        s = new ExprStmt(parse_expr(l), false);
        l.expect(TokenType::BracesClose, true);
    } else
        // if (!l.is_next(TokenType::Keyword))
        s = parse_expr_stmt(l, true);
    // else
    //     throw parse_error{l.peek(), "Expected a statement"};

    l.expect(TokenType::SemiColon, true);
    return s;
}

Stmt *parse_expr_stmt(Lexer &l, bool pop) /* e.g. f(1, 2, 3);   */
{
    return new ExprStmt(parse_expr(l), pop);
}

Stmt *parse_var_stmt(Lexer &l) /* e.g. var x = 2;    */
{
    l.expect(TokenType::Keyword, "var", true);
    std::string name = parse_identifier(l);

    if (l.is_next(TokenType::Operator, "=")) {
        l.discard();
        return new VarStmt(name, parse_expr(l));
    } else
        return new VarStmt(name, new ValueExpr(0));
}

Stmt *parse_ret_stmt(Lexer &l) /* e.g. return x + x; */
{
    l.expect(TokenType::Keyword, "return", true);
    return new RetStmt(parse_expr(l));
}

Stmt *parse_for_stmt(Lexer &l) /* e.g. for (i = 0; i < 3; i += 1) stmt */
{
    Stmt *init = nullptr;
    Expr *condition = nullptr;
    Expr *update = nullptr;

    l.expect(TokenType::Keyword, "for", true);
    l.expect(TokenType::BracesOpen, true);

    if (l.is_next(TokenType::Keyword, "var"))
        init = parse_var_stmt(l);
    else if (!l.is_next(TokenType::SemiColon))
        init = parse_expr_stmt(l);

    l.expect(TokenType::SemiColon, true);

    if (!l.is_next(TokenType::SemiColon))
        condition = parse_expr(l);

    l.expect(TokenType::SemiColon, true);

    if (!l.is_next(TokenType::BracesClose))
        update = parse_expr(l);

    l.expect(TokenType::BracesClose, true);

    return new ForStmt(init, condition, update, parse_compound_stmt(l));
}

Stmt *parse_while_stmt(Lexer &l) {
    Expr *condition = nullptr;

    l.expect(TokenType::Keyword, "while", true);
    l.expect(TokenType::BracesOpen, true);
    if (!l.is_next(TokenType::BracesClose)) {
        condition = parse_expr(l);
    }
    l.expect(TokenType::BracesClose, true);
    return new ForStmt(nullptr, condition, nullptr, parse_compound_stmt(l));
}

Stmt *parse_if_stmt(Lexer &l) /* e.g. if (x) stmt */
{
    l.expect(TokenType::Keyword, "if", true);
    l.expect(TokenType::BracesOpen, true);
    Expr *condition = parse_expr(l);
    l.expect(TokenType::BracesClose, true);

    CompStmt *thens, *elses;

    thens = parse_compound_stmt(l);

    if (l.is_next(TokenType::Keyword, "else")) {
        l.discard();
        elses = parse_compound_stmt(l);
    } else
        elses = new CompStmt({});

    return new IfStmt(condition, thens, elses);
}

Stmt *parse_break_stmt(Lexer &l) /* e.g. break */
{
    l.discard();
    l.expect(TokenType::SemiColon, true);

    return new BreakStmt;
}

Stmt *parse_continue_stmt(Lexer &l) /* e.g. continue */
{
    l.discard();
    l.expect(TokenType::SemiColon, true);

    return new ContinueStmt;
}

Stmt *parse_label_stmt(Lexer &l) /* e.g. label <name>: */
{
    log.info("Found label");

    l.discard(); /* discard 'label' */
    std::string label_name = l.get().value;

    log.info("Label name %s", label_name.c_str());
    l.expect(TokenType::Colon, true);
    return new LabelStmt{label_name};
}

Stmt *parse_jas_stmt(Lexer &l) /* e.g. INVOKEVIRTUAL func */
{
    l.expect(TokenType::Identifier, false);

    Token tok = l.get();
    string op = tok.value;

    if (jas_type_mapping.count(op) == 0)
        throw parse_error{l.peek(), "Unknown JAS instruction: " + str(op)};

    JasStmt *stmt = new JasStmt(op);
    if (stmt->has_const_arg() || stmt->has_fun_arg() || stmt->has_label_arg() ||
        stmt->has_var_arg())
        stmt->arg0 = parse_identifier(l);

    if (stmt->has_imm_arg())
        stmt->iarg0 = parse_value(l, I8_MIN, I8_MAX);

    return stmt;
}

/* Statements usually have expressions */
Expr *parse_expr(Lexer &l) /* delegates to types of statements */
{
    Expr *left = parse_compare_expr(l);

    while (l.is_next(TokenType::Operator, {"=", "+=", "-=", "&=", "|="})) {
        std::string op = l.peek().value;
        l.discard();

        left = new OpExpr(op, left, parse_compare_expr(l));
    }

    return left;
}

static bool is_comparator(std::string op) {
    return op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" ||
           op == ">=";
}

Expr *parse_compare_expr(Lexer &l) /* e.g. a == 3 */
{
    Expr *res = parse_logic_expr(l);

    while (l.peek().type == TokenType::Operator) {
        if (!is_comparator(l.peek().value))
            break;

        std::string op = l.get().value; // skip operator
        res = new OpExpr(op, res, parse_logic_expr(l));
    }

    return res;
}

static bool is_logic_op(std::string op) { return op == "&" || op == "|"; }

Expr *parse_logic_expr(Lexer &l) /* e.g. a | 3, a & b */
{
    Expr *res = parse_arit_expr(l);

    while (l.peek().type == TokenType::Operator) {
        if (!is_logic_op(l.peek().value))
            break;

        std::string op = l.get().value; // skip operator
        res = new OpExpr(op, res, parse_arit_expr(l));
    }

    return res;
}

static bool is_arit_op(std::string op) { return op == "+" || op == "-"; }

Expr *parse_arit_expr(Lexer &l) /* e.g. a + b, a - b */
{
    Expr *res = parse_mul_expr(l);

    while (l.peek().type == TokenType::Operator) {
        if (!is_arit_op(l.peek().value))
            break;

        std::string op = l.get().value; // skip operator
        res = new OpExpr(op, res, parse_mul_expr(l));
    }

    return res;
}

Expr *parse_mul_expr(Lexer &l) {
    Expr *res = parse_basic_expr(l);

    while (l.peek().type == TokenType::Operator) {
        if (l.peek().value != "*")
            break;

        std::string op = l.get().value; // skip operator
        res = new OpExpr(op, res, parse_basic_expr(l));
    }

    return res;
}

static bool numeric(Token &t) {
    switch (t.type) {
    case TokenType::Operator:
        return (t.value == "-");
    case TokenType::Decimal:
    case TokenType::Hexadecimal:
    case TokenType::Character_literal:
        return true;
    default:
        return false;
    }
}

Expr *parse_basic_expr(Lexer &l) /* e.g. a, 2, (1 + 3), f(1) */
{
    bool minus = false;
    Expr *res;

    if (l.peek().type == TokenType::Operator && l.peek().value == "-") {
        minus = true;
        l.discard();
    }

    if (l.is_next(TokenType::Keyword, "$getc")) {
        l.discard();
        l.expect(TokenType::BracesOpen, true);
        l.expect(TokenType::BracesClose, true);
        res = new StmtExpr(new JasStmt("IN"));
    } else if (l.is_next(TokenType::Keyword, "$push")) {
        l.discard();
        l.expect(TokenType::BracesOpen, true);
        res = new StmtExpr(
            new CompStmt({parse_expr_stmt(l, false), new JasStmt("DUP")}));
        l.expect(TokenType::BracesClose, true);
    } else if (l.is_next(TokenType::Keyword, "$pop")) {
        l.discard();
        l.expect(TokenType::BracesOpen, true);
        l.expect(TokenType::BracesClose, true);
        res = new StmtExpr(new CompStmt({}));
    } else if (l.is_next(TokenType::Keyword, "$malloc")) {
        l.discard();
        l.expect(TokenType::BracesOpen, true);
        res = new StmtExpr(
            new CompStmt({parse_expr_stmt(l, false), new JasStmt("NEWARRAY")}));
        l.expect(TokenType::BracesClose, true);
    } else if (l.is_next(TokenType::BracesOpen)) {
        l.expect(TokenType::BracesOpen, true);
        res = parse_expr(l);
        l.expect(TokenType::BracesClose, true);
    } else if (numeric(l.peek()))
        res = new ValueExpr(parse_value(l, INT32_MIN, INT32_MAX));
    else if (l.is_next(TokenType::Identifier)) {
        std::string name = parse_identifier(l);

        /* function call */
        if (l.peek().type == TokenType::BracesOpen)
            res = parse_fcall(name, l);
        else
            res = new IdentExpr(name);
    } else
        throw parse_error{l.peek(), "unknown expression"};

    while (l.is_next(TokenType::BlockOpen)) {
        l.discard();
        res = new ArrAccessExpr(res, parse_expr(l));
        l.expect(TokenType::BlockClose, true);
    }

    if (minus) {
        log.info("Unary minus detected, negating value");

        if (ValueExpr *v = dynamic_cast<ValueExpr *>(res))
            v->value *= -1;
        else
            return new OpExpr("-", new ValueExpr(0), res);
    }
    return res;
}

Expr *parse_fcall(std::string name, Lexer &l) {
    l.expect(TokenType::BracesOpen, true);
    std::vector<Expr *> args;

    if (l.peek().type != TokenType::BracesClose) {
        args.push_back(parse_expr(l));

        while (l.peek().type == TokenType::Comma) {
            l.expect(TokenType::Comma, true);
            args.push_back(parse_expr(l));
        }
    }

    l.expect(TokenType::BracesClose, true);
    return new FunExpr(name, args);
}