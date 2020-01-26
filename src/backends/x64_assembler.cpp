#include <iostream>
#include "x64_assembler.hpp"
#include "ijvm_assembler.hpp"
#include <util/util.hpp>
#include <sys/syscall.h>

#ifdef DEBUG
#define DUMP_INSTRUCTION(op)                            \
    if (log.get_log_level() == LogLevel::info) {        \
        /* store op and tos into argument registers */  \
        x64.mov(x64.rdi, op);                           \
        x64.mov(x64.rsi, x64.ptr[x64.rsp]);             \
                                                        \
        /* calculate func offset */                     \
        x64.mov(x64.rax, x64.ptr[r_functions + r_function_debug]); \
                                                        \
        external_c_call(); \
    }
#else 
#define DUMP_INSTRUCTION(op) 
#endif


/*
 * We follow the System-5 AMD64 ABI so that we can call into c functions:
 *     halt() handles a HALT
 *     error() handles an ERR
 *     getchar() handles getting a character from stdin
 *     putchar() handles putting a character on stdout
 *
 * Off limits:
 *     rbx, rsp, rbp, r12, r13, r14, and r15
 *
 * This means we can use the following for scratch registers:
 *     rax, rdi, rsi, rdx, rcx, r8, r9, r10, r11
 *
 * Function arguments are given by:
 *     rdi, rsi, rdx, rcx, r8, r9
 *
 * IJVM functions follow the following model
 *
 * |            |
 * |            |
 * +------------+ <- rbp
 * |            |
 * |   args     |
 * |            |                   |
 * +------------+                   |
 * |   prevpc   |                   |
 * |   prevrbp  |                   | Stack growing down
 * +------------+                   |
 * |            |                   |
 * |   lvars    |                   v
 * |            |
 * +------------+
 * |            |
 * |  lstack    |
 * |            |
 * +------------+ <- rsp
 *
 */
uint64_t __in__() {
    int c = getchar();
    c = (c < 0) ? 0 : c;

    log.info(" -> read char 0x%02x[%c]", c, c);
    return c;
}

void __out__(int64_t val) {
    putchar(val);
}

void __halt__() {
    // fprintf(stderr, "Closing the IJVM gracefully\n");
    exit(0);
}

void __err__() {
    fprintf(stderr, "ERROR Encountered\n");
    exit(1);
}

uint64_t __newarray__(int64_t size) {
    int64_t *arr = (int64_t *)calloc(size, sizeof(size));    
    log.info(" -> newarray(%ld) -> %p", size, (void *)arr);
    return (uint64_t)arr;
}

int64_t __iaload__(int64_t *arr, int64_t index) {
    log.info(" -> iaload(%p, %ld) -> %ld", (void *)arr, index, arr[index]);
    return arr[index];
}

void __iastore__(int64_t *arr, int64_t index, int64_t value) {
    log.info(" -> iastore(%p, %ld, %ld)", (void *)arr, index, value);
    arr[index] = value;
}

#ifdef DEBUG
void debug(i64 op, i64 tos) {
    switch (op) {
        case op_bipush:        log.info("bipush [tos:%llx]", tos);           break;                       
        case op_dup:           log.info("dup [tos:%llx]", tos);              break;                    
        case op_err:           log.info("err [tos:%llx]", tos);              break;                    
        case op_goto:          log.info("goto [tos:%llx]", tos);             break;                     
        case op_halt:          log.info("halt [tos:%llx]", tos);             break;                     
        case op_iadd:          log.info("iadd [tos:%llx]", tos);             break;                     
        case op_iand:          log.info("iand [tos:%llx]", tos);             break;                     
        case op_ifeq:          log.info("ifeq [tos:%llx]", tos);             break;                     
        case op_iflt:          log.info("iflt [tos:%llx]", tos);             break;                     
        case op_icmpeq:        log.info("icmpeq [tos:%llx]", tos);           break;                       
        case op_iinc:          log.info("iinc [tos:%llx]", tos);             break;                     
        case op_iload:         log.info("iload [tos:%llx]", tos);            break;                      
        case op_in:            log.info("in [tos:%llx]", tos);               break;                   
        case op_invokevirtual: log.info("invokevirtual [tos:%llx]", tos);    break;                                 
        case op_ior:           log.info("ior [tos:%llx]", tos);              break;                    
        case op_ireturn:       log.info("ireturn [tos:%llx]", tos);          break;                        
        case op_istore:        log.info("istore [tos:%llx]", tos);           break;                       
        case op_isub:          log.info("isub [tos:%llx]", tos);             break;                     
        case op_ldc_w:         log.info("ldc_w [tos:%llx]", tos);            break;                      
        case op_nop:           log.info("nop [tos:%llx]", tos);              break;                    
        case op_out:           log.info("out [tos:%llx]", tos);              break;                    
        case op_pop:           log.info("pop [tos:%llx]", tos);              break;                    
        case op_swap:          log.info("swap [tos:%llx]", tos);             break;                     
        case op_wide:          log.info("wide [tos:%llx]", tos);             break;                     
        case op_newarray:      log.info("newarray [tos:%llx]", tos);         break;                         
        case op_iaload:        log.info("iaload [tos:%llx]", tos);           break;                       
        case op_iastore:       log.info("iastore [tos:%llx]", tos);          break;                        
        case op_gc:            log.info("gc [tos:%llx]", tos);               break;                   
        case op_netbind:       log.info("netbind [tos:%llx]", tos);          break;                        
        case op_netconnect:    log.info("netconnect [tos:%llx]", tos);       break;                           
        case op_netin:         log.info("netin [tos:%llx]", tos);            break;                      
        case op_netout:        log.info("netout [tos:%llx]", tos);           break;                       
        case op_netclose:      log.info("netclose [tos:%llx]", tos);         break;                      
        default:
            log.panic("incorrect op");                
    }
}
#endif

X64Assembler::X64Assembler()
    : x64{4096 * 16, Xbyak::AutoGrow}, r_functions{x64.r14}, r_safe{x64.r15}, _io_added{false} {
    
    // if program becomes too long, the default relative jump would simply be too 
    // short and compilation would fail
    x64.setDefaultJmpNEAR(true);

    // The r15 register contains a lookup pointer to the essential functions.
    // It's runtime so to speak.
    x64.mov(r_functions, x64.rdi);
}
X64Assembler::~X64Assembler() {}

void X64Assembler::compile(ostream &o) {
    o.write(x64.getCode<const char *>(), x64.getSize());
}

#define r_function_getchar ((0 * 8))
#define r_function_putchar ((1 * 8))
#define r_function_halt ((2 * 8))
#define r_function_error ((3 * 8))
#define r_function_calloc ((4 * 8))
#define r_function_newarray ((5 * 8))
#define r_function_iaload ((6 * 8))
#define r_function_iastore ((7 * 8))
#define r_function_debug ((8 * 8))

void X64Assembler::run() {
    void *functions[] = {(void *)__in__,     (void *)__out__,
                         (void *)__halt__,   (void *)__err__,
                         (void *)calloc,     (void *)__newarray__,
                         (void *)__iaload__, (void *)__iastore__,
#ifdef DEBUG
                         (void *)debug
#endif
    };

    x64.ready();
    auto code = x64.getCode<void (*)(void **)>();
    code(functions);
    log.panic("Shouldn't have run this oh doodoo");
}

void X64Assembler::external_c_call() {
    /* External C calls are... delecate... aka they ignore AMD64 ABI sometimes */

    // originally stored this in r15, which vprintf just fucks up
    // so have to save c functions pointer on stack
    x64.push(r_functions);

    // store old stack pointer in local variables and align stack
    x64.mov(x64.ptr[x64.rbp + _local_variables["__rsp__"]], x64.rsp);
    x64.and_(x64.rsp, ~0xf);

    // do the actual call
    x64.call(x64.rax);

    // restore rsp (mis)alignment    
    x64.mov(x64.rsp, x64.ptr[x64.rbp + _local_variables["__rsp__"]]);

    // restore func pointer
    x64.pop(r_functions);
}

void X64Assembler::label(string name) {
    log.info("");
    log.info("  %s#%s:", fname.c_str(), name.c_str());
    x64.L(concat(fname, "#", name));
}

void X64Assembler::function(string name, vector<string> args,
                            vector<string> vars) {
    log.info("Building function %s:", name.c_str());

    /* internal book keeping, fill metadata */
    fname = name;
    _local_variables.clear();

    log.info("    stack_frame for function:");
    int offset = 0;

    if (name != "main") {
        _local_variables["__obj_ref__"] = offset;
        log.info("    [rbp - %d] = arg __obj_ref__", offset);
    } else
        offset = -8;

    for (const string &s : args) {
        _local_variables[s] = (offset += 8);
        log.info("    [rbp - %2d] = arg %s", offset, s.c_str());
    }

    _local_variables["__ret_addr__"] = (offset += 8);
    log.info("    [rbp - %2d] = __ret_addr__", offset);

    _local_variables["__base_ptr__"] = (offset += 8);
    log.info("    [rbp - %2d] = __base_ptr__", offset);
    
    // needed for rsp alignment
    _local_variables["__rsp__"] = (offset += 8);
    log.info("    [rbp - %2d] = __rsp__", offset);

    for (const string &s : vars) {
        _local_variables[s] = (offset += 8);
        log.info("    [rbp - %2d] = lvar %s", offset, s.c_str());
    }

    /* code generation */

    // SETUP jmp label so we can call this site
    x64.L(name);

    log.info("; declare function");
    log.info("%s:", name.c_str());
    x64.push(x64.rbp); // remember prev rbp
    log.info("    push rbp                  ; save previous rbp");

    // we need to calculate rbp, which is
    // rbp := rsp + [#args + 2] * (sizeof(register))
    x64.lea(x64.rbp, x64.ptr[x64.rsp + (2 + args.size()) * 8]);
    log.info("    lea rbp, [rsp + %3d]      ; calculate where rbp should go",
             (2 + args.size()) * 8);

    // now we need to setup rsp, which means we need to reserve enough room for
    // all the lvars
    x64.sub(x64.rsp, (vars.size() + 1) * 8);
    log.info("    sub rsp, %-4d             ; reserve space for lvars + __rsp__ space",
             (vars.size() + 1) * 8);

    // create safety barier
    x64.mov(x64.rax, 0x1337133713371337ULL);
    x64.push(x64.rax);
}

bool X64Assembler::is_var(string name) { return _local_variables.count(name); }
void X64Assembler::BIPUSH(int8_t value) {
    log.info("    push %-14d       ; BIPUSH %d", value, value);
    DUMP_INSTRUCTION(op_bipush);

    x64.push(value);
}

void X64Assembler::LDC_W(string constant) {
    log.info("    push %-14d       ; LDC_W %s", constant_map[constant],
             constant.c_str());
    DUMP_INSTRUCTION(op_ldc_w);

    x64.push(constant_map[constant]);

}

void X64Assembler::DUP() {
    log.info("    mov rax, [rsp]      ; DUP");
    log.info("    push rax");
    DUMP_INSTRUCTION(op_dup);

    x64.mov(x64.rax, x64.ptr[x64.rsp]);
    x64.push(x64.rax);
}

void X64Assembler::IAND() {
    log.info("    pop rax             ; IAND");
    log.info("    and [rsp], rax");
    DUMP_INSTRUCTION(op_iand);

    x64.pop(x64.rax);
    x64.and_(x64.ptr[x64.rsp], x64.rax);
}

void X64Assembler::IOR() {
    log.info("    pop rax             ; IOR");
    log.info("    or [rsp], rax");
    DUMP_INSTRUCTION(op_ior);

    x64.pop(x64.rax);
    x64.or_(x64.ptr[x64.rsp], x64.rax);
}

void X64Assembler::IADD() {
    log.info("    pop rax             ; IADD");
    DUMP_INSTRUCTION(op_iadd);
    
    x64.pop(x64.rax);
    x64.pop(x64.rcx);
    x64.add(x64.ecx, x64.eax);
    x64.movsxd(x64.rax, x64.ecx);
    x64.push(x64.rax);
}

void X64Assembler::ISUB() {
    log.info("    pop rax                   ; ISUB");
    log.info("    sub [rsp], rax");
    DUMP_INSTRUCTION(op_isub);

    x64.pop(x64.rax);
    x64.pop(x64.rcx);
    x64.sub(x64.ecx, x64.eax);
    x64.movsxd(x64.rax, x64.ecx);
    x64.push(x64.rax);
}

void X64Assembler::POP() {
    log.info("    pop rax                   ; POP");
    DUMP_INSTRUCTION(op_pop);

    x64.pop(x64.rax);
}

void X64Assembler::SWAP() {
    log.info("    pop rax                   ; SWAP");
    log.info("    pop rcx");
    log.info("    push rax");
    log.info("    push rcx");
    DUMP_INSTRUCTION(op_swap);

    x64.pop(x64.rax);
    x64.pop(x64.rcx);
    x64.push(x64.rax);
    x64.push(x64.rcx);
}

void X64Assembler::ILOAD(string var) {
    log.info("    mov rax, [rbp - %4d]      ; ILOAD %s", _local_variables[var],
             var.c_str());
    log.info("    push rax");
    DUMP_INSTRUCTION(op_iload);

    x64.mov(x64.rax, x64.ptr[x64.rbp - _local_variables[var]]);
    x64.push(x64.rax);
}
void X64Assembler::ISTORE(string var) {
    log.info("    pop rax                   ; ISTORE %s", var.c_str());
    log.info("    mov [rbp - %4d] rax", _local_variables[var]);
    DUMP_INSTRUCTION(op_istore);

    x64.pop(x64.rax);
    x64.mov(x64.ptr[x64.rbp - _local_variables[var]], x64.rax);
}

void X64Assembler::IINC(string var, int8_t value) {
    log.info("    add qword [rbp - %4d], %-2d; IINC %s %d",
             _local_variables[var], value, var.c_str(), value);
    DUMP_INSTRUCTION(op_iinc);
    
    x64.add(x64.qword[x64.rbp - _local_variables[var]], value);
}

void X64Assembler::WIDE() {
    DUMP_INSTRUCTION(op_wide);
}

void X64Assembler::HALT() {
    log.info("    mov rax, halt          ; HALT");
    log.info("    call halt");
    DUMP_INSTRUCTION(op_halt);

    x64.and_(x64.rsp, ~0xf);
    x64.mov(x64.rax, x64.ptr[r_functions + r_function_halt]);
    external_c_call();
}

void X64Assembler::ERR() {
    log.info("    mov rax, error         ; ERR");
    log.info("    call error");
    DUMP_INSTRUCTION(op_err);
    
    x64.mov(x64.rax, x64.ptr[r_functions + r_function_error]);
    external_c_call();
}

void X64Assembler::IN() {
    log.info("    mov rax, getchar          ; IN");
    log.info("    call getchar");
    log.info("    push rax");
    DUMP_INSTRUCTION(op_in);

    x64.mov(x64.rax, x64.ptr[r_functions + r_function_getchar]);
    external_c_call();
    x64.push(x64.rax);
}

void X64Assembler::OUT() {
    log.info("    pop rdi                   ; OUT");
    log.info("    mov rax, putchar");
    log.info("    call putchar");
    DUMP_INSTRUCTION(op_out);

    x64.pop(x64.rdi);
    x64.mov(x64.rax, x64.ptr[r_functions + r_function_putchar]);
    external_c_call();
}

void X64Assembler::NOP() {
    DUMP_INSTRUCTION(op_nop);
}

void X64Assembler::GOTO(string label) {
    log.info("    jmp .%-20s ; GOTO %s", label.c_str(), label.c_str());
    DUMP_INSTRUCTION(op_goto);
    
    x64.jmp(concat(fname, "#", label));
}

void X64Assembler::ICMPEQ(string label) {
    log.info("    pop rax                   ; ICMPEQ %s", label.c_str());
    log.info("    pop rcx");
    log.info("    cmp rax, rcx");
    log.info("    je  .%s", label.c_str());

    DUMP_INSTRUCTION(op_icmpeq);

    x64.pop(x64.rax);
    x64.pop(x64.rcx);
    x64.cmp(x64.rax, x64.rcx);
    x64.je(concat(fname, "#", label));
}
void X64Assembler::IFLT(string label) {
    log.info("    pop rax                   ; IFLT %s", label.c_str());
    log.info("    cmp rax, 0");
    log.info("    jl .%s", label.c_str());

    DUMP_INSTRUCTION(op_iflt);

    x64.pop(x64.rax);
    x64.cmp(x64.rax, 0);
    x64.jl(concat(fname, "#", label));
}
void X64Assembler::IFEQ(string label) {
    log.info("    pop rax                   ; IFEQ %s", label.c_str());
    log.info("    test rax, rax");
    log.info("    jz .%s", label.c_str());

    DUMP_INSTRUCTION(op_ifeq);

    x64.pop(x64.rax);
    x64.cmp(x64.rax, 0);
    x64.je(concat(fname, "#", label));
}

void X64Assembler::INVOKEVIRTUAL(string func_name) {
    log.info("    call %20s ; INVOKEVIRTUAL %s", func_name.c_str(),
             func_name.c_str());
    log.info("    push rax");

    DUMP_INSTRUCTION(op_invokevirtual);
    x64.call(func_name);
    x64.push(x64.rax);
}

void X64Assembler::IRETURN() {
    log.info("    pop rax                   ; IRETURN");
    log.info("    mov rcx, [rbp - %3d]", _local_variables["__ret_addr__"]);
    log.info("    mov rsp, rbp");
    log.info("    mov rbp, [rbp - %3d]", _local_variables["__base_ptr__"]);
    log.info("    jmp rcx");

    DUMP_INSTRUCTION(op_ireturn);

    // pop return value off stack
    x64.pop(x64.rax);

    // load previous rip in rcx
    x64.mov(x64.rcx, x64.ptr[x64.rbp - _local_variables["__ret_addr__"]]);

    // set top of stack to previous top of stack - args
    x64.mov(x64.rsp, x64.rbp);

    // set base pointer to previous base pointer
    x64.mov(x64.rbp, x64.ptr[x64.rbp - _local_variables["__base_ptr__"]]);

    // jump to previous ptr
    x64.jmp(x64.rcx);
}

void X64Assembler::NEWARRAY() {
    log.info("    pop rdi                   ; NEWARRAY, newarray(tos())");
    // log.info("    mov rsi, 8");
    log.info("    mov rax, newarray");
    log.info("    call rax");
    log.info("    push rax");

    DUMP_INSTRUCTION(op_newarray);


    x64.pop(x64.rdi);
    x64.mov(x64.rax, x64.ptr[r_functions + r_function_newarray]);
    external_c_call();
    x64.push(x64.rax);
}

void X64Assembler::IALOAD() {
    log.info("    pop rdi                   ; IALOAD");
    log.info("    pop rsi");
    log.info("    mov rax, iaload");
    log.info("    call rax");
    log.info("    push rax");

    DUMP_INSTRUCTION(op_iaload);

    x64.pop(x64.rdi);
    x64.pop(x64.rsi);
    x64.mov(x64.rax, x64.ptr[r_functions + r_function_iaload]);
    external_c_call();
    x64.push(x64.rax);
}

void X64Assembler::IASTORE() {
    log.info("    pop rdi                   ; IASTORE");
    log.info("    pop rsi");
    log.info("    pop rdx");
    log.info("    mov rax, iastore");
    log.info("    call rax");
    log.info("    push rax");

    DUMP_INSTRUCTION(op_iastore);

    x64.pop(x64.rdi);
    x64.pop(x64.rsi);
    x64.pop(x64.rdx);
    x64.mov(x64.rax, x64.ptr[r_functions + r_function_iastore]);
    external_c_call();
}

void X64Assembler::GC() { throw std::runtime_error{"Not implemented: GC"}; }

void X64Assembler::NETBIND() {
    throw std::runtime_error{"Not implemented: NETBIND"};
}
void X64Assembler::NETCONNECT() {
    throw std::runtime_error{"Not implemented: NETCONNECT"};
}
void X64Assembler::NETIN() {
    throw std::runtime_error{"Not implemented: NETIN"};
}
void X64Assembler::NETOUT() {
    throw std::runtime_error{"Not implemented: NETOUT"};
}
void X64Assembler::NETCLOSE() {
    throw std::runtime_error{"Not implemented: NETCLOSE"};
}