#include "assembler.hpp"

class JASAssembler : public Assembler
{
  public:
  JASAssembler() : _fn_declared{false} {}  /* creates buffer for assembler to pile up stuff into */
  ~JASAssembler() {} /* frees resources */

  /* high level API */
  virtual void compile(ostream &o);                  /* writes binary to ostream */
  // virtual void constant(string name, int32_t value); /* adds/sets constant */
  virtual void label(string name);                   /* adds label before next instruction */
  // virtual bool is_constant(string name);             /* asks if there is any constant with name */

  /* ends previous function and adds new function */
  virtual void function(string name, vector<string> args, vector<string> vars);
  virtual bool is_var(string name); /* returns whether there is a variable in the current context (local and args) */

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

  private:
  std::stringstream                   cs;
  std::set<string>                    accessible_vars;
  bool                                _fn_declared;
};