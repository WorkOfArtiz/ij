#include "data.hpp"
#include "../util.hpp"

/* All ostream operator << overloads */
std::ostream &operator<<(std::ostream &o, const Expr &e) {
    e.write(o);
    return o;
}

std::ostream &operator<<(std::ostream &o, const Stmt &e) {
    e.write(o);
    return o;
}

std::ostream &operator<<(std::ostream &o, const Constant &c) {
    return o << "Constant(" << c.name << ", " << c.value << ')';
}

std::ostream &operator<<(std::ostream &o, const Function &f) {
    o << "Function<" << f.name << ">(" << join(", ", f.args) << ") {\n";

    for (auto stmt : f.stmts->stmts)
        o << "    " << *stmt << "; \n";

    return o << "}";
}

/* Destructor implementations */
Program::~Program() {
    for (auto f : funcs)
        delete f;

    funcs.clear();

    for (auto c : consts)
        delete c;

    consts.clear();
}

OpExpr::~OpExpr() {
    delete left;
    delete right;
}

IdentExpr::~IdentExpr() {}

ValueExpr::~ValueExpr() {}

FunExpr::~FunExpr() {
    for (auto arg : args)
        delete arg;
    args.clear();
}

InExpr::~InExpr() {}

CompStmt::~CompStmt() {
    for (auto stmt : stmts)
        delete stmt;

    stmts.clear();
}
VarStmt::~VarStmt() { delete expr; }
RetStmt::~RetStmt() { delete expr; }
ExprStmt::~ExprStmt() { delete expr; }
ForStmt::~ForStmt() {
    delete initial;
    delete condition;
    delete update;
    delete body;
}
IfStmt::~IfStmt() {
    delete condition;
    delete thens;
    delete elses;
}
JasStmt::~JasStmt() {}
BreakStmt::~BreakStmt() {}
ContinueStmt::~ContinueStmt() {}
LabelStmt::~LabelStmt() {}

/* Write implementations */
void OpExpr::write(std::ostream &o) const {
    o << "Operator<'" << op << "'>(" << *left << ", " << *right << ")";
}

void IdentExpr::write(std::ostream &o) const {
    o << "Identifier('" << identifier << "')";
}

void ValueExpr::write(std::ostream &o) const { o << "Value(" << value << ")"; }

void FunExpr::write(std::ostream &o) const {
    o << "Function(" << fname << ", (";

    for (auto it = args.cbegin(); it != args.cend(); it++) {
        o << **it;

        if (it + 1 != args.cend())
            o << ", ";
    }

    o << ")";
}

void InExpr::write(std::ostream &o) const { o << "In()"; }

void CompStmt::write(std::ostream &o) const {
    o << "{ ";
    for (Stmt *s : stmts)
        o << *s << "; ";
    o << "}";
}

void VarStmt::write(std::ostream &o) const {
    o << "VarStmt('" << identifier << "', " << *expr << ")";
}

void RetStmt::write(std::ostream &o) const { o << "RetStmt(" << *expr << ")"; }

void JasStmt::write(std::ostream &o) const {
    o << "JasStmt(" << op;

    if (has_var_arg() || has_const_arg() || has_label_arg() || has_fun_arg())
        o << " " << arg0;

    if (has_imm_arg())
        o << " " << iarg0;

    o << ")";
}

void BreakStmt::write(std::ostream &o) const { o << "Break"; }

void ContinueStmt::write(std::ostream &o) const { o << "Continue"; }

void LabelStmt::write(std::ostream &o) const {
    o << "Label(" << label_name << ")";
}

void ExprStmt::write(std::ostream &o) const { o << "Stmt(" << *expr << ")"; }

void ForStmt::write(std::ostream &o) const {
    o << "ForStmt(init=";
    if (initial)
        o << *initial;
    else
        o << "empty";

    o << ", condition=";
    if (condition)
        o << *condition;
    else
        o << "empty";

    o << ", update=";

    if (update)
        o << *update;
    else
        o << "empty";

    o << ")";
    o << body;
}

void IfStmt::write(std::ostream &o) const {
    o << "IfStmt(" << *condition << ") ";
    o << thens;
    o << "\n    Else";
    o << elses;
}

/* has side effects */
bool Expr::has_side_effects(Program &) const { return false; }
bool OpExpr::has_side_effects(Program &p) const {
    return left->has_side_effects(p) || right->has_side_effects(p);
}
bool FunExpr::has_side_effects(Program &) const {
    return true;
} // TODO add for further optimizations

bool InExpr::has_side_effects(Program &) const { return true; }

/* val() aka find whether something gives a constant return value */
option<i32> Expr::val() const { return option<i32>(); }
option<i32> ValueExpr::val() const { return option<i32>(value); }
option<i32> OpExpr::val() const {
    // log.panic("Val on op %s", op.c_str());

    option<i32> l = left->val();
    option<i32> r = right->val();

    if (not l.isset() or not r.isset())
        return option<i32>();

    i32 left = l;
    i32 right = r;

    if (leaves_on_stack())
        log.panic("Trying to get value from non-returning update");

    // clang-format off
    if (op == "==")         return left == right;
    else if (op == "<=")    return left <= right;
    else if (op == "<")     return left <  right;
    else if (op == ">")     return left >  right;
    else if (op == ">=")    return left >= right;
    else if (op == "+")     return left +  right;
    else if (op == "-")     return left -  right;
    else if (op == "|")     return left |  right;
    else // if (op == "&") 
        return left &  right;
    // clang-format on
}

/* Finding var statements */
void Stmt::find_vars(std::vector<std::string> &) const { return; }

/* find_vars */
void CompStmt::find_vars(std::vector<std::string> &vec) const {
    for (auto s : stmts)
        s->find_vars(vec);
}

void VarStmt::find_vars(std::vector<std::string> &vec) const {
    vec.push_back(identifier);
}
void ForStmt::find_vars(std::vector<std::string> &vec) const {
    body->find_vars(vec);
}
void IfStmt::find_vars(std::vector<std::string> &vec) const {
    thens->find_vars(vec);
    elses->find_vars(vec);
}

// clang-format off
const std::unordered_map<string, JasType> jas_type_mapping =
{
    {"BIPUSH",        JasType::BIPUSH},        {"DUP",           JasType::DUP},
    {"ERR",           JasType::ERR},           {"GOTO",          JasType::GOTO},
    {"HALT",          JasType::HALT},          {"IADD",          JasType::IADD},
    {"IAND",          JasType::IAND},          {"IFEQ",          JasType::IFEQ},
    {"IFLT",          JasType::IFLT},          {"ICMPEQ",        JasType::ICMPEQ},
    {"IINC",          JasType::IINC},          {"ILOAD",         JasType::ILOAD},
    {"IN",            JasType::IN},            {"INVOKEVIRTUAL", JasType::INVOKEVIRTUAL},
    {"IOR",           JasType::IOR},           {"IRETURN",       JasType::IRETURN},
    {"ISTORE",        JasType::ISTORE},        {"ISUB",          JasType::ISUB},
    {"LDC_W",         JasType::LDC_W},         {"NOP",           JasType::NOP},
    {"OUT",           JasType::OUT},           {"POP",           JasType::POP},
    {"SWAP",          JasType::SWAP},          {"WIDE",          JasType::WIDE},
    {"IF_ICMPEQ",     JasType::ICMPEQ},        {"NEWARRAY",      JasType::NEWARRAY},
    {"IALOAD",        JasType::IALOAD},        {"IASTORE",       JasType::IASTORE},
    {"NETBIND",       JasType::NETBIND},       {"NETCONNECT",    JasType::NETCONNECT},
    {"NETIN",         JasType::NETIN},         {"NETOUT",        JasType::NETOUT},
    {"NETCLOSE",      JasType::NETCLOSE},
};
// clang-format on

/* OpExpr methods */
bool OpExpr::is_comparison() const {
    switch (op[0]) {
    case '=':
        return op == "==";
    case '<':
        return true;
    case '>':
        return true;
    case '!':
        return op == "!=";
    }
    return false;
}

bool OpExpr::leaves_on_stack() const {
    switch (op[0]) {
    case '+':
    case '-':
    case '|':
    case '&':
    case '/':
    case '*':
        return op.size() == 1;
    }
    return false;
}
