#include "data.hpp"
#include "../util.hpp"

/* Fills in the compile functions of the various statements */
bool in(std::string needle, std::initializer_list<std::string> hay) {
    for (auto straw : hay)
        if (straw == needle)
            return true;

    return false;
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
void OpExpr::compile(Assembler &a) const {
    if (in(op, {"!=", "==", "<", ">", ">=", "<="}))
        throw std::runtime_error{"Compile error: no support for " + op +
                                 " outside of conditionals"};

    if (op == "=") {
        if (IdentExpr *var = dynamic_cast<IdentExpr *>(left)) {
            if (!a.is_var(var->identifier))
                throw std::runtime_error{
                    "only local variables can be assigned"};

            right->compile(a);
            a.ISTORE(var->identifier);
        } else if (ArrAccessExpr *arr = dynamic_cast<ArrAccessExpr *>(left)) {
            right->compile(a);
            arr->index->compile(a);
            arr->array->compile(a);
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
            right->compile(a);
            compile_arit_op(op[0], a);
            a.ISTORE(var->identifier);
        } else if (ArrAccessExpr *arr = dynamic_cast<ArrAccessExpr *>(left)) {
            arr->index->compile(a);
            arr->array->compile(a);
            a.IALOAD();
            right->compile(a);
            compile_arit_op(op[0], a);
            arr->index->compile(a);
            arr->array->compile(a);
            a.IASTORE();
        } else
            throw std::runtime_error{
                "Compile error: you can only reassign variables"};

    } else if (in(op, {"+", "-", "&", "|"})) {
        left->compile(a);
        right->compile(a);
        compile_arit_op(op[0], a);
    } else {
        std::runtime_error{"unsupported operator found: " + op};
    }
}

void IdentExpr::compile(Assembler &a) const {
    if (a.is_var(identifier))
        a.ILOAD(identifier);
    else if (a.is_constant(identifier))
        a.LDC_W(identifier);
    else
        throw std::runtime_error{"Couldn't find reference to " + identifier};
}

void ValueExpr::compile(Assembler &a) const {
    a.PUSH_VAL(value); // takes care of BIPUSH limitations
}

void FunExpr::compile(Assembler &a) const {
    if (!a.is_constant("__OBJREF__"))
        a.constant("__OBJREF__", 0xd000d000);
    a.LDC_W("__OBJREF__");

    for (auto e : args)
        e->compile(a);

    a.INVOKEVIRTUAL(fname);
}

void InExpr::compile(Assembler &a) const { a.IN(); }

void ArrAccessExpr::compile(Assembler &a) const {
    index->compile(a);
    array->compile(a);
    a.IALOAD();
}

void CompStmt::compile(Program &p, Assembler &a, id_gen &gen) const {
    for (auto s : stmts)
        s->compile(p, a, gen);
}

void ExprStmt::compile(Program &, Assembler &a, id_gen &) const {
    expr->compile(a);

    if (OpExpr *o = dynamic_cast<OpExpr *>(expr))
        if (!o->leaves_on_stack())
            return;

    if (pop)
        a.POP();
}

void VarStmt::compile(Program &, Assembler &a, id_gen &) const {
    expr->compile(a);
    a.ISTORE(identifier);
}

void RetStmt::compile(Program &, Assembler &a, id_gen &) const {
    expr->compile(a);
    a.IRETURN();
}

static void compile_comparison(Assembler &a, OpExpr *con, std::string if_true,
                               std::string if_false) {
    if (con->op == "<") {
        con->left->compile(a);
        con->right->compile(a);
        a.ISUB();
        a.IFLT(if_true);
        a.GOTO(if_false);
    } else if (con->op == ">") {
        con->right->compile(a);
        con->left->compile(a);
        a.ISUB();
        a.IFLT(if_true);
        a.GOTO(if_false);
    } else if (con->op == ">=") {
        con->left->compile(a);
        con->right->compile(a);
        a.ISUB();
        a.IFLT(if_false);
        a.GOTO(if_true);
    } else if (con->op == "<=") {
        con->right->compile(a);
        con->left->compile(a);
        a.ISUB();
        a.IFLT(if_false);
        a.GOTO(if_true);
    } else if (con->op == "==") {
        con->left->compile(a);
        con->right->compile(a);
        a.ICMPEQ(if_true);
        a.GOTO(if_false);
    } else if (con->op == "!=") {
        con->left->compile(a);
        con->right->compile(a);
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
            compile_comparison(a, con, for_body, for_end);
        else {
            condition->compile(a);
            a.IFEQ(for_end);
        }
    } else if (condition != nullptr) {
        condition->compile(a);
        a.IFEQ(for_end);
    }

    a.label(for_body);
    body->compile(p, a, gen);

    a.label(for_update);
    if (update != nullptr)
        update->compile(a);
    a.GOTO(for_condition);
    a.label(for_end);
}

void IfStmt::compile(Program &p, Assembler &a, id_gen &gen) const {
    option<i32> cond_val = condition->val();

    if (cond_val.isset()) {
        if (condition->has_side_effects(p)) {
            condition->compile(a);
            a.POP();
        }

        if (cond_val != 0) // only then has to be compiled
            thens->compile(p, a, gen);
        else
            elses->compile(p, a, gen);

        return;
    }

    size_t if_id = gen.gif();

    std::string if_start = sprint("if%d_condition", if_id);
    std::string if_then = sprint("if%d_then", if_id);
    std::string if_else = sprint("if%d_else", if_id);
    std::string if_end = sprint("if%d_end", if_id);

    a.label(if_start);
    if (OpExpr *con = dynamic_cast<OpExpr *>(condition)) {
        if (con->is_comparison()) {
            compile_comparison(a, con, if_then, if_else);
        } else {
            condition->compile(a);
            a.IFEQ(if_else);
        }
    } else {
        condition->compile(a);
        a.IFEQ(if_else);
    }

    a.label(if_then);
    thens->compile(p, a, gen);

    // GOTO only needs to be added if there is
    // code to jump over
    if (!elses->empty()) {
        a.GOTO(if_end);

        a.label(if_else);
        elses->compile(p, a, gen);
    } else
        a.label(if_else);

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

    if (name != "main")
        a.function(name, args, vars);

    stmts->compile(p, a, generator);
}