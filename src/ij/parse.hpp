#ifndef EC_PARSE
#define EC_PARSE
#include "data.hpp"
#include "lexer.hpp"

class parse_error : public std::runtime_error {
  public:
    parse_error(Token &t, std::string msg);

  private:
    std::string make_what(Token &t, std::string msg);
};

/* High level functions */
Program *parse_program(Lexer &l);
Constant *parse_constant(Lexer &l);
Function *parse_function(Lexer &l);

/* Functions have statements */
Stmt *parse_statement(Lexer &l);     /* delegates to types of statements */
Stmt *parse_expr_stmt(Lexer &l);     /* e.g. f(1, 2, 3);   */
Stmt *parse_var_stmt(Lexer &l);      /* e.g. var x = 2;    */
Stmt *parse_ret_stmt(Lexer &l);      /* e.g. return x + x; */
Stmt *parse_for_stmt(Lexer &l);      /* e.g. for (i = 0; i < 3; i += 1) stmt */
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
Expr *parse_basic_expr(Lexer &l);   /* e.g. a, 2, (1 + 3), f(1) */
Expr *parse_fcall(std::string fname, Lexer &l);

/* basic parts */
std::string parse_identifier(Lexer &l);
int32_t parse_value(Lexer &l);

#endif
