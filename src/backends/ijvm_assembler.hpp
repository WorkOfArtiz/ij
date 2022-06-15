#include <backends/assembler.hpp>
#include <util/buffer.hpp>
#include <util/opcodes.hpp>

class IJVMAssembler : public Assembler {
  public:
    IJVMAssembler(); /* creates buffer for assembler to pile up stuff into */
    virtual ~IJVMAssembler(); /* frees resources */

    /* high level API */
    virtual void compile(ostream &o); /* writes binary to ostream */
    virtual void label(string name);  /* adds label before next instruction */

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

    /* heap */
    virtual void NEWARRAY();
    virtual void IALOAD();
    virtual void IASTORE();
    virtual void GC();

    /* network */
    virtual void NETBIND();
    virtual void NETCONNECT();
    virtual void NETIN();
    virtual void NETOUT();
    virtual void NETCLOSE();

    /* arithmetic */
    virtual void SHL();
    virtual void SHR();
    virtual void IMUL();
    virtual void IDIV();

    /*
     * workhorse of the linking process
     */
    void link(std::unordered_map<string, u32> findexes);

  private:
    Buffer code;                            /* raw compiled text section */
    std::unordered_map<string, u32> laddrs; /* label -> offset in buffer */
    std::unordered_map<u32, string>
        jmpaddrs; /* JMP[1 byte op] label addresses */
    std::unordered_map<string, u32>
        faddrs; /* fname -> offset, required when linking functions */
    std::unordered_map<u32, string> invokes; /* invokevirtual name */

    string current_func; /* keep track of function */
    vector<string> vars;
};