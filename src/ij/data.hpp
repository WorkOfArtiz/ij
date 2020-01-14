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
#include "../util.hpp"

/* classes defined here */
struct Program;
struct Function;
struct Constant;
struct Stmt;
struct Expr;
struct OpExpr;
struct ValueExpr;
struct id_gen;

/* everything overwrites the << operator */
std::ostream &operator<<(std::ostream &o, const Expr &e);
std::ostream &operator<<(std::ostream &o, const Stmt &e);
std::ostream &operator<<(std::ostream &o, const Function &f);
std::ostream &operator<<(std::ostream &o, const Constant &c);

struct id_gen {
    inline id_gen() : forid{0}, ifid{0} {}
    inline ssize_t last_for() { return forid - 1; }
    inline ssize_t gfor() { return forid++; }
    inline ssize_t gif() { return ifid++; }
    ssize_t forid, ifid;
};

struct Expr {
    /* functions all expressions have to overload */
    virtual ~Expr() = 0;
    virtual void write(std::ostream &o) const = 0;
    virtual void compile(Program &p, Assembler &a, id_gen &gen) const = 0;
    virtual bool has_side_effects(Program &p) const; /* optional */
    virtual option<i32> val() const; /* returns the value, if const */
};
inline Expr::~Expr() {}

struct OpExpr : Expr {
    inline OpExpr(std::string op, Expr *l, Expr *r)
        : op{op}, left{l}, right{r} {}
    virtual ~OpExpr();
    virtual void write(std::ostream &o) const;
    virtual void compile(Program &p, Assembler &a, id_gen &gen) const;
    virtual bool has_side_effects(Program &p) const;
    virtual option<i32> val() const; /* returns the value, if const */

    bool is_comparison() const;
    bool leaves_on_stack() const; /* whether there's something on stack after */

    std::string op;
    Expr *left;
    Expr *right;
};

struct IdentExpr : Expr {
    inline IdentExpr(std::string identifier) : identifier{identifier} {}
    virtual ~IdentExpr();
    virtual void write(std::ostream &o) const;
    virtual void compile(Program &p, Assembler &a, id_gen &gen) const;

    std::string identifier;
};

struct ValueExpr : Expr {
    inline ValueExpr(int32_t value) : value{value} {}
    virtual ~ValueExpr();

    virtual void write(std::ostream &o) const;
    virtual void compile(Program &p, Assembler &a, id_gen &gen) const;
    virtual option<i32> val() const; /* returns the value, if const */

    int32_t value;
};

struct FunExpr : Expr {
    inline FunExpr(std::string name, std::vector<Expr *> args)
        : fname{name}, args{args} {}
    virtual ~FunExpr();

    virtual void write(std::ostream &o) const;
    virtual void compile(Program &p, Assembler &a, id_gen &gen) const;
    virtual bool has_side_effects(Program &p) const;

    std::string fname;
    std::vector<Expr *> args;
};

struct StmtExpr : Expr {
    inline StmtExpr(Stmt *s) : stmt{s} {}
    virtual ~StmtExpr();

    virtual void write(std::ostream &o) const;
    virtual void compile(Program &p, Assembler &a, id_gen &gen) const;
    virtual bool has_side_effects(Program &p) const;

    Stmt *stmt;
};

struct ArrAccessExpr : Expr {
    inline ArrAccessExpr(Expr *array, Expr *index)
        : array{array}, index{index} {}
    inline ~ArrAccessExpr() {
        delete array;
        delete index;
    }

    virtual void write(std::ostream &o) const;
    virtual void compile(Program &p, Assembler &a, id_gen &g) const;
    virtual bool has_side_effects(Program &p) const;

    Expr *array;
    Expr *index;
};

struct Stmt {
    virtual void write(std::ostream &o) const = 0;
    virtual void compile(Program &p, Assembler &a, id_gen &gen) const = 0;
    virtual void find_vars(std::vector<std::string> &vec) const;
    virtual ~Stmt() = 0;
};
inline Stmt::~Stmt() {}

struct CompStmt : Stmt {
    inline CompStmt(std::vector<Stmt *> stmts) : stmts{std::move(stmts)} {}
    inline CompStmt(CompStmt &&old) {
        for (auto x : old.stmts)
            stmts.push_back(x);

        old.stmts.clear();
    }
    virtual ~CompStmt();

    virtual void write(std::ostream &o) const;
    virtual void compile(Program &p, Assembler &a, id_gen &gen) const;
    virtual void find_vars(std::vector<string> &vec) const;

    bool
    is_terminal() const; /* whether it contains a return, IRETURN, HALT, ERR */
    inline bool empty() const { return stmts.size() == 0; }
    vector<Stmt *> stmts;
};

struct VarStmt : Stmt {
    inline VarStmt(std::string identifier, Expr *expr)
        : identifier{identifier}, expr{expr} {}
    virtual ~VarStmt();

    virtual void write(std::ostream &o) const;
    virtual void compile(Program &p, Assembler &a, id_gen &gen) const;
    virtual void find_vars(std::vector<std::string> &vec) const;

    std::string identifier;
    Expr *expr;
};

struct RetStmt : Stmt {
    inline RetStmt(Expr *e) : expr{e} {}
    virtual ~RetStmt();

    virtual void write(std::ostream &o) const;
    virtual void compile(Program &p, Assembler &a, id_gen &gen) const;

    Expr *expr;
};

struct ExprStmt : Stmt {
    inline ExprStmt(Expr *e, bool pop) : expr{e}, pop{pop} {}
    virtual ~ExprStmt();

    virtual void write(std::ostream &o) const;
    virtual void compile(Program &p, Assembler &a, id_gen &gen) const;

    Expr *expr;
    bool pop;
};

struct ForStmt : Stmt {
    inline ForStmt(Stmt *initial, Expr *condition, Expr *update, CompStmt *body)
        : initial{initial}, condition{condition}, update{update}, body{body} {}
    virtual ~ForStmt();

    virtual void write(std::ostream &o) const;
    virtual void compile(Program &p, Assembler &a, id_gen &gen) const;
    virtual void find_vars(std::vector<std::string> &vec) const;

    Stmt *initial;
    Expr *condition;
    Expr *update;
    CompStmt *body;
};

struct IfStmt : Stmt {
    inline IfStmt(Expr *condition, CompStmt *thens, CompStmt *elses)
        : condition{condition}, thens{thens}, elses{elses} {}
    virtual ~IfStmt();

    virtual void write(std::ostream &o) const;
    virtual void compile(Program &p, Assembler &a, id_gen &gen) const;
    virtual void find_vars(std::vector<std::string> &vec) const;

    Expr *condition;
    CompStmt *thens;
    CompStmt *elses;
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
    inline JasStmt(string s) : op{s}, instr_type{jas_type_mapping.at(op)} {}
    virtual void write(std::ostream &o) const;
    virtual void compile(Program &p, Assembler &a, id_gen &gen) const;
    virtual ~JasStmt();

    inline bool has_var_arg() const // ILOAD, ISTORE, IINC
    {
        return in(instr_type, {JasType::ILOAD, JasType::ISTORE, JasType::IINC});
    }

    inline bool has_const_arg() const // LDC_W
    {
        return instr_type == JasType::LDC_W;
    }

    inline bool has_imm_arg() const // IINC, BIPUSH
    {
        return in(instr_type, {JasType::IINC, JasType::BIPUSH});
    }

    inline bool has_label_arg() const // GOTO, IFEQ, IFLT, ICMPEQ
    {
        return in(instr_type, {JasType::GOTO, JasType::IFEQ, JasType::IFLT,
                               JasType::ICMPEQ});
    }

    inline bool has_fun_arg() const // INVOKEVIRTUAL
    {
        return instr_type == JasType::INVOKEVIRTUAL;
    }

    inline static JasStmt *BIPUSH(i8 arg) {
        JasStmt *stmt = new JasStmt("BIPUSH");
        stmt->iarg0 = arg;
        return stmt;
    }

    std::string op;
    JasType instr_type;
    std::string arg0;
    i32 iarg0;
};

struct LabelStmt : Stmt {
    inline LabelStmt(string name) : label_name{name} {}
    virtual void write(std::ostream &o) const;
    virtual void compile(Program &p, Assembler &a, id_gen &gen) const;
    virtual ~LabelStmt();

    std::string label_name;
};

struct BreakStmt : Stmt {
    virtual void write(std::ostream &o) const;
    virtual void compile(Program &p, Assembler &a, id_gen &gen) const;
    virtual ~BreakStmt();
};

struct ContinueStmt : Stmt {
    virtual void write(std::ostream &o) const;
    virtual void compile(Program &p, Assembler &a, id_gen &gen) const;
    virtual ~ContinueStmt();
};

struct Function {
    inline Function(std::string ident, std::vector<std::string> args,
                    CompStmt *stmts, bool jas = false)
        : name{ident}, args{args}, stmts{stmts}, jas{jas} {}

    inline Function(Function &&other) : name{other.name}, args{other.args} {
        stmts = std::move(other.stmts);
    }

    inline ~Function() {
        log.info("Function %s is being deleted", name.c_str());
        delete stmts;
    }

    std::vector<std::string> get_vars() const;
    void compile(Program &p, Assembler &a) const;

    std::string name;
    std::vector<std::string> args;
    CompStmt *stmts;
    bool jas;
};

struct Constant {
    Constant(std::string name, int32_t value) : name{name}, value{value} {}

    std::string name;
    int32_t value;
};

struct Program {
    ~Program();

    std::vector<Function *> funcs;
    std::vector<Constant *> consts;

    void compile(Assembler &a) const;
};
#endif