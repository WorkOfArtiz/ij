#include "data.hpp"
#include "../util.hpp"

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

/* Implement Expr Class */
Expr *Expr::fun(std::string name, std::vector<Expr *> args) {
    return new FunExpr(name, args);
}
Expr *Expr::val(int32_t val) { return new ValueExpr(val); }
Expr *Expr::var(std::string var) { return new IdentExpr(var); }
Expr *Expr::op(std::string op, Expr *left, Expr *right) {
    return new OpExpr(op, left, right);
}

bool Expr::has_side_effects(Program &) const { return false; }

std::ostream &operator<<(std::ostream &o, const Expr &e) {
    e.write(o);
    return o;
}

/* Statement impl */
Stmt *Stmt::gfor(Expr *initial, Expr *cond, Expr *update,
                 std::vector<Stmt *> body) {
    return new ForStmt(initial, cond, update, body);
}
Stmt *Stmt::var(std::string identifier, Expr *e) {
    return new VarStmt(identifier, e);
}
Stmt *Stmt::expr(Expr *e) { return new ExprStmt(e); }
void Stmt::find_vars(std::vector<std::string> &) const { return; }

std::ostream &operator<<(std::ostream &o, const Stmt &e) {
    e.write(o);
    return o;
}

/* Constructors */
LabelStmt::LabelStmt(string label) : label_name{label} {}

/* Free implementations */
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
VarStmt::~VarStmt() { delete expr; }
RetStmt::~RetStmt() { delete expr; }
ExprStmt::~ExprStmt() { delete expr; }
ForStmt::~ForStmt() {
    delete initial;
    delete condition;
    delete update;
    for (auto stmt : body)
        delete stmt;
    body.clear();
}
IfStmt::~IfStmt() {
    delete condition;
    for (auto stmt : thens)
        delete stmt;
    for (auto stmt : elses)
        delete stmt;
    thens.clear();
    elses.clear();
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

void VarStmt::write(std::ostream &o) const {
    o << "VarStmt('" << identifier << "', " << *expr << ")";
}

void RetStmt::write(std::ostream &o) const { o << "RetStmt(" << *expr << ")"; }

void JasStmt::write(std::ostream &o) const {
    o << "JasStmt(" << op;

    switch (instr_type) {
    case JasType::IINC:
    case JasType::GOTO:
    case JasType::IFEQ:
    case JasType::IFLT:
    case JasType::ICMPEQ:
    case JasType::ILOAD:
    case JasType::INVOKEVIRTUAL:
    case JasType::ISTORE:
    case JasType::LDC_W:
        o << " " << arg0;
    default:
        break;
    }

    if (instr_type == JasType::BIPUSH || instr_type == JasType::IINC)
        o << " " << std::hex << iarg0;

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

    o << ") { ";
    for (Stmt *s : body)
        o << *s << "; ";

    o << "}";
}

void IfStmt::write(std::ostream &o) const {
    o << "IfStmt(" << *condition << ") {";

    for (auto stmt : thens)
        o << *stmt << ";";
    o << "}\n    Else {";
    for (auto stmt : elses)
        o << *stmt << ";";
    o << "}";
}

/* find_vars */
void VarStmt::find_vars(std::vector<std::string> &vec) const {
    vec.push_back(identifier);
}
void ForStmt::find_vars(std::vector<std::string> &vec) const {
    for (Stmt *s : body)
        s->find_vars(vec);
}
void IfStmt::find_vars(std::vector<std::string> &vec) const {
    for (Stmt *s : thens)
        s->find_vars(vec);

    for (Stmt *s : elses)
        s->find_vars(vec);
}

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

/* Constant */
std::ostream &operator<<(std::ostream &o, const Constant &c) {
    return o << "Constant(" << c.name << ", " << c.value << ')';
}

std::ostream &operator<<(std::ostream &o, const Function &f) {
    o << "Function<" << f.name << ">(" << join(", ", f.args) << ") {\n";

    for (auto stmt : f.stmts)
        o << "    " << *stmt << "; \n";

    return o << "}";
}

Program::~Program() {
    for (auto f : funcs)
        delete f;

    funcs.clear();

    for (auto c : consts)
        delete c;

    consts.clear();
}