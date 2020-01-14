#ifndef ASSEMBLER_H
#define ASSEMBLER_H
#include <string>
#include <vector>
#include <set>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <map>
#include "../types.h"

using std::ostream;
using std::string;
using std::vector;

class Assembler {
  public:
    Assembler() {} /* creates buffer for assembler to pile up stuff into */
    virtual ~Assembler() {} /* frees resources */

    /* Constant handling is done high level */
    void constant(string name, i32 value); /* adds/sets constant */
    bool is_constant(string name); /* asks if there is any constant with name */

    /* high level API */
    virtual void compile(ostream &o) = 0; /* writes binary to ostream */
    virtual void
    label(string name) = 0; /* adds label before next instruction */

    /* ends previous function and adds new function */
    virtual void function(string name, vector<string> args,
                          vector<string> vars) = 0;
    virtual bool
    is_var(string name) = 0; /* returns whether there is a variable in the
                                current context (local and args) */

    /* pseudo instructions for commonly used shortcuts */
    virtual void PUSH_VAL(i32 value);
    virtual void SET_VAR(string var, i32 value);
    virtual void INC_VAR(string var, i32 value);
    virtual void
    IMUL(i32 value); /* Pseudo-op for multiplication with constant */

    /* Note, WIDE is done automatically for vars */
    virtual void BIPUSH(i8 value) = 0;
    virtual void DUP() = 0;
    virtual void IADD() = 0;
    virtual void IAND() = 0;
    virtual void IOR() = 0;
    virtual void ISUB() = 0;
    virtual void POP() = 0;
    virtual void SWAP() = 0;

    /* constants */
    virtual void LDC_W(string constant) = 0;

    /* local vars, WIDE is done automatically */
    virtual void ILOAD(string var) = 0;
    virtual void IINC(string var, i8 value) = 0;
    virtual void ISTORE(string var) = 0;
    virtual void WIDE() = 0;

    /* external interfacing */
    virtual void HALT() = 0;
    virtual void ERR() = 0;
    virtual void IN() = 0;
    virtual void OUT() = 0;
    virtual void NOP() = 0;

    /* control flow OPS */
    virtual void GOTO(string label) = 0;
    virtual void ICMPEQ(string label) = 0;
    virtual void IFLT(string label) = 0;
    virtual void IFEQ(string label) = 0;

    /* functions */
    virtual void INVOKEVIRTUAL(string func_name) = 0;
    virtual void IRETURN() = 0;

    /* bonus extensions */
    virtual void NEWARRAY() = 0;
    virtual void IALOAD() = 0;
    virtual void IASTORE() = 0;
    virtual void GC() = 0;

    virtual void NETBIND() = 0;
    virtual void NETCONNECT() = 0;
    virtual void NETIN() = 0;
    virtual void NETOUT() = 0;
    virtual void NETCLOSE() = 0;

  protected:
    std::unordered_map<string, i32> constant_map;
    std::vector<string> constant_order;
};

#endif
