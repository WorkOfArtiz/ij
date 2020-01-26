#ifndef EC_PARSE
#define EC_PARSE
#include "data.hpp"
#include <frontends/common/lexer.hpp>
#include <frontends/common/basic_parse.hpp> // parsing identifiers and integers
#include <frontends/common/parse_error.hpp>

/* High level functions */
Program *parse_program(Lexer &l);
Constant *parse_constant(Lexer &l);
Function *parse_function(Lexer &l);

/* Functions have statements */
CompStmt *parse_compoundstmt(Lexer &l); /* Stmt or { Stmt* } */
Stmt *parse_statement(Lexer &l);        /* delegates to types of statements */
Stmt *parse_expr_stmt(Lexer &l, bool pop = true); /* e.g. f(1, 2, 3);   */
Stmt *parse_var_stmt(Lexer &l);                   /* e.g. var x = 2;    */
Stmt *parse_ret_stmt(Lexer &l);                   /* e.g. return x + x; */
Stmt *parse_for_stmt(Lexer &l);      /* e.g. for (i = 0; i < 3; i += 1) stmt */
Stmt *parse_while_stmt(Lexer &l);    /* e.g. while (i < 3) { u; } */
Stmt *parse_if_stmt(Lexer &l);       /* e.g. if (x) stmt */
Stmt *parse_break_stmt(Lexer &l);    /* e.g. break; */
Stmt *parse_continue_stmt(Lexer &l); /* e.g. continue; */
Stmt *parse_jas_stmt(Lexer &l);      /* e.g. INVOKEVIRTUAL func */
Stmt *parse_label_stmt(Lexer &l);    /* e.g. label x: */

/* Statements usually have expressions */
Expr *parse_expr(Lexer &l);         /* delegates to types of statements */
Expr *parse_compare_expr(Lexer &l); /* e.g. a == 3 */
Expr *parse_logic_expr(Lexer &l);   /* e.g. a | 3, a & b */
Expr *parse_arit_expr(Lexer &l);    /* e.g. a + b, a - b */
Expr *parse_mul_expr(Lexer &l);     /* e.g. a * b  */
Expr *parse_basic_expr(Lexer &l);   /* e.g. a, 2, (1 + 3), f(1) */
Expr *parse_fcall(std::string fname, Lexer &l);

#endif
