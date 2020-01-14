#include "assembler.hpp"
#include <xbyak/xbyak.h>
/*
 * BIPUSH 1
 * LDC_W  __OBJ_REF
 * BIPUSH 3
 * INVOKEVIRTUAL x (1 arg)
 *
 * leads to a stack like this, everything 64 bit:
 *
 *        +--------------+ <-+
 *        |   OLDER_RBP  |   |
 *        +--------------+   |
 *        |   RETADDR    |   |
 *        +--------------+   |
 *        |      1       |   |
 * RBP -> +--------------+   |
 *        |   OLD_RBP    | --+
 *        +--------------+
 *        |      3       |
 *        +--------------+
 *        |   RETADDR    |
 *        +--------------+
 */

class X64Assembler : public Assembler {
  public:
    X64Assembler();
    virtual ~X64Assembler();

    /* high level API */
    virtual void compile(ostream &o); /* writes binary to ostream */
    virtual void label(string name);  /* adds label before next instruction */
    void run();                       /* run the code, does not return */

    /* ends previous function and adds new function */
    virtual void function(string name, vector<string> args,
                          vector<string> vars);
    virtual bool is_var(string name); /* returns whether there is a variable in
                                         the current context (local and args) */

    /* Note, WIDE is done automatically for vars */
    virtual void BIPUSH(int8_t value);
    virtual void DUP();
    virtual void IADD();
    virtual void IAND();
    virtual void IOR();
    virtual void ISUB();
    virtual void POP();
    virtual void SWAP();

    /* constants */
    virtual void LDC_W(string constant);

    /* local vars, WIDE is done automatically */
    virtual void ILOAD(string var);
    virtual void IINC(string var, int8_t value);
    virtual void ISTORE(string var);
    virtual void WIDE();

    /* external interfacing */
    virtual void HALT();
    virtual void ERR();
    virtual void IN();
    virtual void OUT();
    virtual void NOP();

    /* control flow OPS */
    virtual void GOTO(string label);
    virtual void ICMPEQ(string label);
    virtual void IFLT(string label);
    virtual void IFEQ(string label);

    /* functions */
    virtual void INVOKEVIRTUAL(string func_name);
    virtual void IRETURN();

    /* bonus extensions */
    virtual void NEWARRAY();
    virtual void IALOAD();
    virtual void IASTORE();
    virtual void GC();

    virtual void NETBIND();
    virtual void NETCONNECT();
    virtual void NETIN();
    virtual void NETOUT();
    virtual void NETCLOSE();

  private:
    Xbyak::CodeGenerator x64;
    const Xbyak::Reg64 &r_functions;

    string fname;
    std::unordered_map<string, int> _fn_argc;
    std::unordered_map<string, int> _local_variables;
    bool _io_added;
};