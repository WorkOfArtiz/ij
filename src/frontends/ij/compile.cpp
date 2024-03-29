#include "compile.hpp"
#include "data.hpp"
#include <memory>
#include <vector>
#include <util/util.hpp>

/*
 * Add default main, calling __main__, this is to avoid
 * the shitty local vars of the entry point problem.
 */
static void add_main(Program &p) {
    Function *f = new Function(
        "main", {},
        new CompStmt({new IfStmt(new FunExpr("__main__", {}),
                                 new CompStmt({new JasStmt{"ERR"}}),
                                 new CompStmt({new JasStmt{"HALT"}}))}));

    p.funcs.insert(p.funcs.begin(), f);
}

static void prune(Program &p) {
    std::set<std::string> reachable_funcs;
    std::set<std::string> reachable_consts;

    std::vector<const Function *> todo{p.funcs[0]};
    std::vector<const Expr *> exprs{};
    std::vector<const Stmt *> stmts{};

    while (!todo.empty()) {
        const Function *f = todo.back();
        if (contains(reachable_funcs, f->name)) {
            todo.pop_back();
            continue;
        }

        reachable_funcs.insert(f->name);
        todo.pop_back();

        f->stmts->statements(stmts);

        for (const Stmt *s : stmts) {
            // log.info(" - Statement: %s", cstr(*s));

            if (const JasStmt *jas_stmt = dynamic_cast<const JasStmt *>(s)) {
                if (jas_stmt->has_fun_arg()){
                    std::string fname = jas_stmt->arg0;
                    option<const Function *> f = p.get_function(fname);

                    if (!f.isset()) {
                        log.panic("Couldnt find function of name '%s' even though it was mentioned", fname.c_str());
                    }

                    todo.push_back(f);
                }

                if (jas_stmt->has_const_arg()) {
                    reachable_consts.insert(jas_stmt->arg0);
                }
            }
        }
        stmts.clear();

        f->stmts->expressions(exprs);

        for (const Expr * e : exprs) {
            if (const FunExpr *fun_expr = dynamic_cast<const FunExpr *>(e)) {
                option<const Function *> f = p.get_function(fun_expr->fname);
                if (!f.isset()) {
                    log.panic("Couldnt find function of name '%s' even though it was mentioned in func call", fun_expr->fname.c_str());
                }

                const Function *function = f;
                todo.push_back(function);
            }

            if (const IdentExpr *ident_expr = dynamic_cast<const IdentExpr *>(e)) {
                if (!f->has_var(ident_expr->identifier)) {
                    reachable_consts.insert(ident_expr->identifier);
                }
            }
        }

        exprs.clear();
    }

    for (std::string s : reachable_funcs) {
        log.info(" > Function %s is reachable", s.c_str());
    }

    for (std::string s : reachable_consts) {
        log.info(" > Constant %s is reachable", s.c_str());
    }

    auto func_it = p.funcs.begin();
    while (func_it != p.funcs.end()) {
        Function *f = *func_it;

        if (!contains(reachable_funcs, f->name)) {
            log.info(" > Function %s is not reachable ", f->name.c_str());
            delete f;
            p.funcs.erase(func_it);
        }
        else {
            func_it++;
        }
    }

    auto const_it = p.consts.begin();
    while (const_it != p.consts.end()) {
        Constant *c = *const_it;

        if (!contains(reachable_consts, c->name)) {
            log.info(" > Constant %s is not reachable ", c->name.c_str());
            delete c;
            p.consts.erase(const_it);
        }
        else {
            const_it++;
        }
    }
}



void ij_compile(Lexer &l, Assembler &a) {
    std::unique_ptr<Program> p{parse_program(l)};
    add_main(*p);
    prune(*p);

    log.info("constants %lu", p->consts.size());
    for (auto c : p->consts) {
        log.info("    - %s", cstr(*c));
        a.constant(c->name, c->value);
    }

    log.info("functions %lu", p->funcs.size());
    for (auto f : p->funcs)
        log.info("function: %s", cstr(*f));

    for (auto fiter : p->funcs) {
        log.info("Compiling function %s", fiter->name.c_str());
        fiter->compile(*p, a);
    }

    log.success("Successfully compiled program");
}

static void compile_arit_op(char op, Assembler &a) {
    // clang-format off
    switch (op) {
    case '+': a.IADD(); break;
    case '-': a.ISUB(); break;
    case '&': a.IAND(); break;
    case '|': a.IOR();  break;
    }
    // clang-format on
}

/*
 * we need to take care of the following cases
 * != == <= < >= > comparators, which shouldn't be handled here
 * += -= &= |= updaters
 * + - & | arithmetic
 * = assign
 */
void OpExpr::compile(Program &p, Assembler &a, id_gen &g) const {
    if (in(op, {"!=", "==", "<", ">", ">=", "<="}))
        throw std::runtime_error{"Compile error: no support for " + op +
                                 " outside of conditionals"};

    if (op == "=") {
        if (IdentExpr *var = dynamic_cast<IdentExpr *>(left)) {
            if (!a.is_var(var->identifier))
                throw std::runtime_error{
                    "only local variables can be assigned"};

            right->compile(p, a, g);
            a.ISTORE(var->identifier);
        } else if (ArrAccessExpr *arr = dynamic_cast<ArrAccessExpr *>(left)) {
            right->compile(p, a, g);
            arr->index->compile(p, a, g);
            arr->array->compile(p, a, g);
            a.IASTORE();
        } else
            throw std::runtime_error{
                "Compile error: you can only reassign variables and arrays"};
    } else if (in(op, {"+=", "-=", "&=", "|="})) {
        if (IdentExpr *var = dynamic_cast<IdentExpr *>(left)) {
            if (!a.is_var(var->identifier))
                throw std::runtime_error{
                    "only local variables can be reassigned"};

            // if value is constant and representable as i8, use iinc
            if (ValueExpr *val = dynamic_cast<ValueExpr *>(right)) {
                i32 value = op[0] == '-' ? -val->value : val->value;

                if ((op[0] == '+' || op[0] == '-') && value > -128 &&
                    value < 128) {
                    a.IINC(var->identifier, value);
                    return;
                }
            }

            a.ILOAD(var->identifier);
            right->compile(p, a, g);
            compile_arit_op(op[0], a);
            a.ISTORE(var->identifier);
        } else if (ArrAccessExpr *arr = dynamic_cast<ArrAccessExpr *>(left)) {
            arr->index->compile(p, a, g);
            arr->array->compile(p, a, g);
            a.IALOAD();
            right->compile(p, a, g);
            compile_arit_op(op[0], a);
            arr->index->compile(p, a, g);
            arr->array->compile(p, a, g);
            a.IASTORE();
        } else
            throw std::runtime_error{
                "Compile error: you can only reassign variables"};

    } else if (in(op, {"+", "-", "&", "|"})) {
        left->compile(p, a, g);
        right->compile(p, a, g);
        compile_arit_op(op[0], a);
    } else if (op == "*") {
        log.info("Special IMUL case triggered");
        if (ValueExpr *val = dynamic_cast<ValueExpr *>(left)) {
            right->compile(p, a, g);
            a.IMUL(val->value);
        } else if (ValueExpr *val = dynamic_cast<ValueExpr *>(right)) {
            left->compile(p, a, g);
            a.IMUL(val->value);
        } else {
            log.info("neither is constant");
            throw std::runtime_error{sprint("multiplication only supported with constant, expression: %s", str(*this))};
        }
    } else {
        throw std::runtime_error{"unsupported operator found: " + op};
    }
}

void IdentExpr::compile(Program &, Assembler &a, id_gen &) const {
    if (a.is_var(identifier))
        a.ILOAD(identifier);
    else if (a.is_constant(identifier))
        a.LDC_W(identifier);
    else
        throw std::runtime_error{"Couldn't find reference to " + identifier};
}

void ValueExpr::compile(Program &, Assembler &a, id_gen &) const {
    a.PUSH_VAL(value); // takes care of BIPUSH limitations
}

void FunExpr::compile(Program &p, Assembler &a, id_gen &g) const {
    if (!a.is_constant("__OBJREF__"))
        a.constant("__OBJREF__", 0x00d00d00);
    a.LDC_W("__OBJREF__");

    for (auto e : args)
        e->compile(p, a, g);

    a.INVOKEVIRTUAL(fname);
}

void StmtExpr::compile(Program &p, Assembler &a, id_gen &g) const {
    stmt->compile(p, a, g);
}

void ArrAccessExpr::compile(Program &p, Assembler &a, id_gen &g) const {
    index->compile(p, a, g);
    array->compile(p, a, g);
    a.IALOAD();
}

void CompStmt::compile(Program &p, Assembler &a, id_gen &gen) const {
    for (auto s : stmts)
        s->compile(p, a, gen);
}

void ExprStmt::compile(Program &p, Assembler &a, id_gen &g) const {
    expr->compile(p, a, g);

    if (OpExpr *o = dynamic_cast<OpExpr *>(expr))
        if (!o->leaves_on_stack())
            return;

    if (pop)
        a.POP();
}

void VarStmt::compile(Program &p, Assembler &a, id_gen &g) const {
    expr->compile(p, a, g);
    a.ISTORE(identifier);
}

void RetStmt::compile(Program &p, Assembler &a, id_gen &g) const {
    expr->compile(p, a, g);
    a.IRETURN();
}

static void compile_comparison(Program &p, Assembler &a, id_gen &g, OpExpr *con,
                               std::string if_true, std::string if_false) {
    if (con->op == "<") {
        con->left->compile(p, a, g);
        con->right->compile(p, a, g);
        a.ISUB();
        a.IFLT(if_true);
        a.GOTO(if_false);
    } else if (con->op == ">") {
        con->right->compile(p, a, g);
        con->left->compile(p, a, g);
        a.ISUB();
        a.IFLT(if_true);
        a.GOTO(if_false);
    } else if (con->op == ">=") {
        con->left->compile(p, a, g);
        con->right->compile(p, a, g);
        a.ISUB();
        a.IFLT(if_false);
        a.GOTO(if_true);
    } else if (con->op == "<=") {
        con->right->compile(p, a, g);
        con->left->compile(p, a, g);
        a.ISUB();
        a.IFLT(if_false);
        a.GOTO(if_true);
    } else if (con->op == "==") {
        con->left->compile(p, a, g);
        con->right->compile(p, a, g);
        a.ICMPEQ(if_true);
        a.GOTO(if_false);
    } else if (con->op == "!=") {
        con->left->compile(p, a, g);
        con->right->compile(p, a, g);
        a.ICMPEQ(if_false);
        a.GOTO(if_true);
    } else {
        log.info("operator '%s'", con->op.c_str());
        throw std::runtime_error{"if didn't implement all comparisons yet"};
    }
}

void ForStmt::compile(Program &p, Assembler &a, id_gen &gen) const {
    size_t for_id = gen.gfor();

    std::string for_start = sprint("for%d_start", for_id);
    std::string for_condition = sprint("for%d_condition", for_id);
    std::string for_body = sprint("for%d_body", for_id);
    std::string for_update = sprint("for%d_update", for_id);
    std::string for_end = sprint("for%d_end", for_id);

    a.label(for_start);
    if (initial != nullptr)
        initial->compile(p, a, gen);

    a.label(for_condition);
    if (OpExpr *con = dynamic_cast<OpExpr *>(condition)) {
        if (con->is_comparison())
            compile_comparison(p, a, gen, con, for_body, for_end);
        else {
            condition->compile(p, a, gen);
            a.IFEQ(for_end);
        }
    } else if (condition != nullptr) {
        condition->compile(p, a, gen);
        a.IFEQ(for_end);
    }

    a.label(for_body);
    body->compile(p, a, gen);

    a.label(for_update);
    if (update != nullptr)
        update->compile(p, a, gen);
    a.GOTO(for_condition);
    a.label(for_end);
}

void IfStmt::compile(Program &p, Assembler &a, id_gen &gen) const {
    option<i32> cond_val = condition->val();

    if (cond_val.isset()) {
        if (condition->has_side_effects(p)) {
            condition->compile(p, a, gen);
            a.POP();
        }

        if (cond_val != 0) // only then has to be compiled
            thens->compile(p, a, gen);
        else
            elses->compile(p, a, gen);

        return;
    }

    size_t if_id = gen.gif();
    bool else_enabled = !elses->empty();

    std::string if_start = sprint("if%d_condition", if_id);
    std::string if_then = sprint("if%d_then", if_id);
    // std::string if_else = sprint("if%d_else", if_id);
    std::string if_end = sprint("if%d_end", if_id);
    std::string if_else = else_enabled ? sprint("if%d_else", if_id) : if_end;

    a.label(if_start);
    if (OpExpr *con = dynamic_cast<OpExpr *>(condition)) {
        if (con->is_comparison()) {
            compile_comparison(p, a, gen, con, if_then, if_else);
        } else {
            condition->compile(p, a, gen);
            a.IFEQ(if_else);
        }
    } else {
        condition->compile(p, a, gen);
        a.IFEQ(if_else);
    }

    a.label(if_then);
    thens->compile(p, a, gen);

    // GOTO only needs to be added if there is
    // code to jump over
    if (else_enabled) {
        if (!thens->is_terminal()) {
            a.GOTO(if_end);
        }


        a.label(if_else);
        elses->compile(p, a, gen);
    }

    a.label(if_end);
}

void LabelStmt::compile(Program &, Assembler &a, id_gen &) const {
    a.label(label_name);
}

// clang-format off
void JasStmt::compile(Program &, Assembler &a, id_gen &) const
{
    switch (instr_type)
    {
        case JasType::BIPUSH:        a.BIPUSH(iarg0);        break;
        case JasType::DUP:           a.DUP();                break;
        case JasType::ERR:           a.ERR();                break;
        case JasType::GOTO:          a.GOTO(arg0);           break;
        case JasType::HALT:          a.HALT();               break;
        case JasType::IADD:          a.IADD();               break;
        case JasType::IAND:          a.IAND();               break;
        case JasType::IFEQ:          a.IFEQ(arg0);           break;
        case JasType::IFLT:          a.IFLT(arg0);           break;
        case JasType::ICMPEQ:        a.ICMPEQ(arg0);         break;
        case JasType::IINC:          a.IINC(arg0, iarg0);    break;
        case JasType::ILOAD:         a.ILOAD(arg0);          break;
        case JasType::IN:            a.IN();                 break;
        case JasType::INVOKEVIRTUAL: a.INVOKEVIRTUAL(arg0);  break;
        case JasType::IOR:           a.IOR();                break;
        case JasType::IRETURN:       a.IRETURN();            break;
        case JasType::ISTORE:        a.ISTORE(arg0);         break;
        case JasType::ISUB:          a.ISUB();               break;
        case JasType::LDC_W:         a.LDC_W(arg0);          break;
        case JasType::NOP:           a.NOP();                break;
        case JasType::OUT:           a.OUT();                break;
        case JasType::POP:           a.POP();                break;
        case JasType::SWAP:          a.SWAP();               break;
        case JasType::WIDE:          a.WIDE();               break;
        case JasType::NEWARRAY:      a.NEWARRAY();           break;
        case JasType::IALOAD:        a.IALOAD();             break;
        case JasType::IASTORE:       a.IASTORE();            break;
        case JasType::NETBIND:       a.NETBIND();            break;
        case JasType::NETCONNECT:    a.NETCONNECT();         break;
        case JasType::NETIN:         a.NETIN();              break;
        case JasType::NETOUT:        a.NETOUT();             break;
        case JasType::NETCLOSE:      a.NETCLOSE();           break;
        case JasType::SHL:           a.SHL();                break;
        case JasType::SHR:           a.SHR();                break;
        case JasType::IMUL:          a.IMUL();               break;
        case JasType::IDIV:          a.IDIV();               break;

    }
}
// clang-format on

void BreakStmt::compile(Program &, Assembler &a, id_gen &gen) const {
    if (gen.last_for() == -1)
        log.panic("break outside for detected");

    string label = sprint("for%d_end", gen.last_for());
    a.GOTO(label);
}

void ContinueStmt::compile(Program &, Assembler &a, id_gen &gen) const {
    if (gen.last_for() == -1)
        log.panic("break outside for detected");

    string label = sprint("for%d_update", gen.last_for());
    a.GOTO(label);
}

void Function::compile(Program &p, Assembler &a) const {
    id_gen generator;

    std::vector<std::string> vars;
    stmts->find_vars(vars);

    a.function(name, args, vars);
    stmts->compile(p, a, generator);
}