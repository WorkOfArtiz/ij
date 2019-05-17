#include "assembler.hpp"
#include "../buffer.hpp"

/* opcodes */
const u8 op_bipush         = 0x10;
const u8 op_dup            = 0x59;
const u8 op_err            = 0xFE;
const u8 op_goto           = 0xA7;
const u8 op_halt           = 0xFF;
const u8 op_iadd           = 0x60;
const u8 op_iand           = 0x7E;
const u8 op_ifeq           = 0x99;
const u8 op_iflt           = 0x9B;
const u8 op_icmpeq         = 0x9F;
const u8 op_iinc           = 0x84;
const u8 op_iload          = 0x15;
const u8 op_in             = 0xFC;
const u8 op_invokevirtual  = 0xB6;
const u8 op_ior            = 0xB0;
const u8 op_ireturn        = 0xAC;
const u8 op_istore         = 0x36;
const u8 op_isub           = 0x64;
const u8 op_ldc_w          = 0x13;
const u8 op_nop            = 0x00;
const u8 op_out            = 0xFD;
const u8 op_pop            = 0x57;
const u8 op_swap           = 0x5F;
const u8 op_wide           = 0xC4;
const u8 op_newarray       = 0xD1;
const u8 op_iastore        = 0xD2;
const u8 op_iaload         = 0xD3;
const u8 op_netbind        = 0xE1;
const u8 op_netconnect     = 0xE2;
const u8 op_netin          = 0xE3;
const u8 op_netout         = 0xE4;
const u8 op_netclose       = 0xE5;

class IJVMAssembler : public Assembler
{
  public:
  IJVMAssembler();  /* creates buffer for assembler to pile up stuff into */
  ~IJVMAssembler(); /* frees resources */

  /* high level API */
  virtual void compile(ostream &o);                  /* writes binary to ostream */
  virtual void label(string name);                   /* adds label before next instruction */

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

  /* bonus extensions */
  virtual void NEWARRAY();
  virtual void IALOAD();
  virtual void IASTORE();
  virtual void NETBIND();
  virtual void NETCONNECT();
  virtual void NETIN();
  virtual void NETOUT();
  virtual void NETCLOSE();

  /*
   * workhorse of the linking process
   */
  void link(std::unordered_map<string, u32> findexes);

  private:
  buffer                          code;     /* raw compiled text section */
  std::unordered_map<string, u32> laddrs;   /* label -> offset in buffer */
  std::unordered_map<u32, string> jmpaddrs; /* JMP[1 byte op] label addresses */
  std::unordered_map<string, u32> faddrs;   /* fname -> offset, required when linking functions */
  std::unordered_map<u32, string> invokes;  /* invokevirtual name */


  string                          current_func; /* keep track of function */
  vector<string>                  vars;
};