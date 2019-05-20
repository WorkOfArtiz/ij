/*
 * The data structures necessary for compiling EC
 *
 * EC has functions and constants
 *
 * Constants are just name value pairs
 * Functions are
 *  - a name
 *  - a number of arguments (not including objref)
 *  - a number of statements making up the body
 *
 * Statements are:
 *  - variable declarations
 *  - return statement
 *  - for statement
 *  - if statement
 *  - continue statement (inside for)
 *  - break statement (inside for)
 *  - just an expression
 *  - jas statement (inside jas function)
 *  - label statement (inside jas function)
 *
 * Expressions are:
 *  - Operator apply
 *  - an identifier (variable)
 *  - function call
 *  - a direct value
 *
 */

#ifndef ECDATA_H
#define ECDATA_H
#include <string>
#include <vector>
#include <iostream>
#include "../backends/assembler.hpp"
#include "../logger.hpp"

struct Program;

struct id_gen {
    id_gen() : forid{0}, ifid{0} {}

    ssize_t last_for() { return forid - 1; }
    ssize_t gfor() { return forid++; }
    ssize_t gif() { return ifid++; }
    ssize_t forid, ifid;
};

struct Expr {
    virtual void write(std::ostream &o) const = 0;
    virtual void compile(Assembler &a) const = 0;
    virtual bool has_side_effects(Program &p) const;
    virtual ~Expr() = 0;

    static Expr *fun(std::string name, std::vector<Expr *> args);
    static Expr *val(int32_t val);
    static Expr *var(std::string var);
    static Expr *op(std::string op, Expr *left, Expr *right);
};

inline Expr::~Expr() {}
std::ostream &operator<<(std::ostream &o, const Expr &e);

struct OpExpr : Expr {
    OpExpr(std::string op, Expr *l, Expr *r) : op{op}, left{l}, right{r} {}
    virtual ~OpExpr();

    virtual void write(std::ostream &o) const;
    virtual void compile(Assembler &a) const;

    bool is_comparison() const;
    bool leaves_on_stack() const; /* whether there's something on stack after */

    std::string op;
    Expr *left;
    Expr *right;
};

struct IdentExpr : Expr {
    IdentExpr(std::string identifier) : identifier{identifier} {}
    virtual ~IdentExpr();

    virtual void write(std::ostream &o) const;
    virtual void compile(Assembler &a) const;

    std::string identifier;
};

struct ValueExpr : Expr {
    ValueExpr(int32_t value) : value{value} {}
    virtual ~ValueExpr();

    virtual void write(std::ostream &o) const;
    virtual void compile(Assembler &a) const;

    int32_t value;
};

struct FunExpr : Expr {
    FunExpr(std::string name, std::vector<Expr *> args)
        : fname{name}, args{args} {}
    virtual ~FunExpr();

    virtual void write(std::ostream &o) const;
    virtual void compile(Assembler &a) const;

    std::string fname;
    std::vector<Expr *> args;
};

struct Stmt {
    virtual void write(std::ostream &o) const = 0;
    virtual void compile(Assembler &a, id_gen &gen) const = 0;
    virtual void find_vars(std::vector<std::string> &vec) const;
    virtual ~Stmt() = 0;

    static Stmt *gfor(Expr *initial, Expr *cond, Expr *update,
                      std::vector<Stmt *> body);
    static Stmt *var(std::string identifier, Expr *e);
    static Stmt *expr(Expr *e);
};

inline Stmt::~Stmt() {}

std::ostream &operator<<(std::ostream &o, const Stmt &e);

struct VarStmt : Stmt {
    VarStmt(std::string identifier, Expr *expr)
        : identifier{identifier}, expr{expr} {}
    virtual ~VarStmt();

    virtual void write(std::ostream &o) const;
    virtual void compile(Assembler &a, id_gen &gen) const;
    virtual void find_vars(std::vector<std::string> &vec) const;

    std::string identifier;
    Expr *expr;
};

struct RetStmt : Stmt {
    RetStmt(Expr *e) : expr{e} {}
    virtual ~RetStmt();

    virtual void write(std::ostream &o) const;
    virtual void compile(Assembler &a, id_gen &gen) const;

    Expr *expr;
};

struct ExprStmt : Stmt {
    ExprStmt(Expr *e) : expr{e} {}
    virtual ~ExprStmt();

    virtual void write(std::ostream &o) const;
    virtual void compile(Assembler &a, id_gen &gen) const;

    Expr *expr;
};

struct ForStmt : Stmt {
    ForStmt(Expr *initial, Expr *condition, Expr *update,
            std::vector<Stmt *> body)
        : initial{initial}, condition{condition}, update{update}, body{body} {}
    virtual ~ForStmt();

    virtual void write(std::ostream &o) const;
    virtual void compile(Assembler &a, id_gen &gen) const;
    virtual void find_vars(std::vector<std::string> &vec) const;

    Expr *initial;
    Expr *condition;
    Expr *update;
    std::vector<Stmt *> body;
};

struct IfStmt : Stmt {
    IfStmt(Expr *condition, std::vector<Stmt *> thens,
           std::vector<Stmt *> elses)
        : condition{condition}, thens{thens}, elses{elses} {}
    virtual ~IfStmt();

    virtual void write(std::ostream &o) const;
    virtual void compile(Assembler &a, id_gen &gen) const;
    virtual void find_vars(std::vector<std::string> &vec) const;

    Expr *condition;
    std::vector<Stmt *> thens;
    std::vector<Stmt *> elses;
};

// clang-format off
enum class JasType
{
    BIPUSH,    DUP,           ERR,     GOTO,
    HALT,      IADD,          IAND,    IFEQ,
    IFLT,      ICMPEQ,        IINC,    ILOAD,
    IN,        INVOKEVIRTUAL, IOR,     IRETURN,
    ISTORE,    ISUB,          LDC_W,   NOP,
    OUT,       POP,           SWAP,    WIDE,
    NEWARRAY,  IALOAD,        IASTORE, NETBIND,
    NETCONNECT,NETIN,         NETOUT,  NETCLOSE
};
// clang-format on

extern const std::unordered_map<string, JasType> jas_type_mapping;

struct JasStmt : Stmt {
    virtual void write(std::ostream &o) const;
    virtual void compile(Assembler &a, id_gen &gen) const;
    virtual ~JasStmt();

    std::string op;
    JasType instr_type;
    std::string arg0;
    i32 iarg0;
};

struct LabelStmt : Stmt {
    LabelStmt(string name);
    virtual void write(std::ostream &o) const;
    virtual void compile(Assembler &a, id_gen &gen) const;
    virtual ~LabelStmt();

    std::string label_name;
};

struct BreakStmt : Stmt {
    virtual void write(std::ostream &o) const;
    virtual void compile(Assembler &a, id_gen &gen) const;
    virtual ~BreakStmt();
};

struct ContinueStmt : Stmt {
    virtual void write(std::ostream &o) const;
    virtual void compile(Assembler &a, id_gen &gen) const;
    virtual ~ContinueStmt();
};

struct Function {
    Function(std::string ident, std::vector<std::string> args,
             std::vector<Stmt *> stmts, bool jas = false)
        : name{ident}, args{args}, stmts{stmts}, jas{jas} {}

    Function(Function &&other) : name{other.name}, args{other.args} {
        stmts = std::move(other.stmts);
        other.stmts.clear();
    }

    ~Function() {
        log.info("Function %s is being deleted", name.c_str());

        while (!stmts.empty()) {
            delete stmts.back();
            stmts.pop_back();
        }
    }

    std::vector<std::string> get_vars() const;
    void compile(Assembler &a) const;

    std::string name;
    std::vector<std::string> args;
    std::vector<Stmt *> stmts;
    bool jas;
};

std::ostream &operator<<(std::ostream &o, const Function &f);

struct Constant {
    Constant(std::string name, int32_t value) : name{name}, value{value} {}

    std::string name;
    int32_t value;
};

std::ostream &operator<<(std::ostream &o, const Constant &c);

struct Program {
    ~Program();

    std::vector<Function *> funcs;
    std::vector<Constant *> consts;

    void compile(Assembler &a) const;
};
#endif