#include "data.hpp"
#include "../util.hpp"

/* Fills in the compile functions of the various statements */
bool in(std::string needle, std::initializer_list<std::string> hay)
{
    for (auto straw : hay)
        if (straw == needle)
            return true;

    return false;
}

/*
 * we need to take care of the following cases
 * != == <= < >= > comparators, which shouldn't be handled here
 * += -= &= |= updaters
 * + - & | arithmetic
 * = assign
 */
void OpExpr::compile(Assembler &a) const
{
    if (in(op, {"!=", "==", "<", ">", ">=", "<="}))
        throw std::runtime_error{"Compile error: no support for " + op + " outside of conditionals"};

    if (op == "=")
    {
        if (left->type() != ExprType::IdentExpr)
            throw std::runtime_error{"Compile error: you can only reassign variables"};

        right->compile(a);
        a.ISTORE(reinterpret_cast<IdentExpr *>(left)->identifier);
    }
    else if (in(op, {"+=", "-=", "&=", "|="}))
    {
        if (left->type() != ExprType::IdentExpr)
            throw std::runtime_error{"Compile error: you can only reassign variables"};

        right->compile(a);
        a.ILOAD(reinterpret_cast<IdentExpr *>(left)->identifier);

        switch(op[0])
        {
            case '+': a.IADD(); break;
            case '-': a.ISUB(); break;
            case '&': a.IAND(); break;
            case '|': a.IOR(); break;
        }

        a.ISTORE(reinterpret_cast<IdentExpr *>(left)->identifier);
    }
    else if(in(op, {"+", "-", "&", "|"})) {
        left->compile(a);
        right->compile(a);

        switch(op[0])
        {
            case '+': a.IADD(); break;
            case '-': a.ISUB(); break;
            case '&': a.IAND(); break;
            case '|': a.IOR(); break;
        }
    }
    else {
        std::runtime_error{"unsupported operator found: " + op};
    }
}

void IdentExpr::compile(Assembler &a) const
{
    if (a.is_var(identifier))
        a.ILOAD(identifier);
    else if (a.is_constant(identifier))
        a.LDC_W(identifier);
    else
        throw std::runtime_error{"Couldn't find reference to " + identifier};
}

void ValueExpr::compile(Assembler &a) const
{
    a.PUSH_VAL(value); // takes care of BIPUSH limitations
}

void FunExpr::compile(Assembler &a) const
{
    if (!a.is_constant("__OBJREF__"))
        a.constant("__OBJREF__", 0xd000d000);
    a.LDC_W("__OBJREF__");

    for (auto e : args)
        e->compile(a);

    a.INVOKEVIRTUAL(fname);
}

void ExprStmt::compile(Assembler &a, id_gen &) const
{
    expr->compile(a);

    if (OpExpr *o = dynamic_cast<OpExpr *>(expr))
        if (!o->leaves_on_stack())
            return;

    a.POP();
}

void VarStmt::compile(Assembler &a, id_gen &) const
{
    expr->compile(a);
    a.ISTORE(identifier);
}

void RetStmt::compile(Assembler &a, id_gen &) const
{
    expr->compile(a);
    a.IRETURN();
}

static void compile_comparison(Assembler &a, OpExpr *con, std::string if_true, std::string if_false)
{
    if (con->op == "<")
    {
        con->left->compile(a);
        con->right->compile(a);
        a.ISUB();
        a.IFLT(if_true);
        a.GOTO(if_false);
    }
    else if (con->op == ">")
    {
        con->right->compile(a);
        con->left->compile(a);
        a.ISUB();
        a.IFLT(if_true);
        a.GOTO(if_false);
    }
    else if (con->op == ">=")
    {
        con->left->compile(a);
        con->right->compile(a);
        a.ISUB();
        a.IFLT(if_false);
        a.GOTO(if_true);
    }
    else if (con->op == "<=")
    {
        con->right->compile(a);
        con->left->compile(a);
        a.ISUB();
        a.IFLT(if_false);
        a.GOTO(if_true);
    }
    else if(con->op == "==")
    {
        con->left->compile(a);
        con->right->compile(a);
        a.ICMPEQ(if_true);
        a.GOTO(if_false);
    }
    else if (con->op == "!=")
    {
        con->left->compile(a);
        con->right->compile(a);
        a.ICMPEQ(if_false);
        a.GOTO(if_true);
    }
    else
    {
        log.info("operator '%s'", con->op.c_str());
        throw std::runtime_error{"if didn't implement all comparisons yet"};
    }
}

void ForStmt::compile(Assembler &a, id_gen &gen) const
{
    size_t for_id = gen.gfor();

    std::string for_start = sprint("for%d_start", for_id);
    std::string for_condition = sprint("for%d_condition", for_id);
    std::string for_body = sprint("for%d_body", for_id);
    std::string for_update = sprint("for%d_update", for_id);
    std::string for_end = sprint("for%d_end", for_id);

    a.label(for_start);
    initial->compile(a);

    a.label(for_condition);
    if (OpExpr *con = dynamic_cast<OpExpr *>(condition))
    {
        if (con->is_comparison())
            compile_comparison(a, con, for_body, for_end);
        else
        {
            condition->compile(a);
            a.IFEQ(for_end);
        }
    }
    else
    {
        condition->compile(a);
        a.IFEQ(for_end);
    }

    a.label(for_body);
    for (Stmt *s : body)
    {
        s->compile(a, gen);
    }

    a.label(for_update);
    update->compile(a);
    a.GOTO(for_condition);
    a.label(for_end);
}

void IfStmt::compile(Assembler &a, id_gen &gen) const
{
    size_t if_id = gen.gif();

    std::string if_start = sprint("if%d_condition", if_id);
    std::string if_then = sprint("if%d_then", if_id);
    std::string if_else = sprint("if%d_else", if_id);
    std::string if_end = sprint("if%d_end", if_id);

    a.label(if_start);
    if (OpExpr *con = dynamic_cast<OpExpr *>(condition))
    {
        if(con->is_comparison())
        {
            compile_comparison(a, con, if_then, if_else);
        }
        else 
        {
            condition->compile(a);
            a.IFEQ(if_else);
        }
    }
    else
    {
        condition->compile(a);
        a.IFEQ(if_else);
    }

    a.label(if_then);
    for (Stmt *s : thens)
        s->compile(a, gen);
    
    // GOTO only needs to be added if there is
    // code to jump over
    if (elses.size())
    {
        a.GOTO(if_end);

        a.label(if_else);
        for (Stmt *s : elses)
            s->compile(a, gen);
    }
    else 
        a.label(if_else);

    a.label(if_end);
}

void JasStmt::compile(Assembler &a, id_gen &) const
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
    }
}

void Function::compile(Assembler &a) const
{
    id_gen generator;

    std::vector<std::string> vars;
    for (const Stmt *s : stmts)
        s->find_vars(vars);

    a.function(name, args, vars);

    for (Stmt *stmt : this->stmts)
        stmt->compile(a, generator);
}