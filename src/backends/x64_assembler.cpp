#include <iostream>
#include "x64_assembler.hpp"
#include "../util.hpp"
#include <sys/syscall.h>

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
uint64_t op_in() {
    int c = getchar();
    return (c < 0) ? 0 : c;
}
void halt() {
    // fprintf(stdout, "Closing the IJVM gracefully\n");
    exit(0);
}

void error() {
    fprintf(stdout, "ERROR Encountered\n");
    exit(1);
}

uint64_t op_newarray(int64_t size) {
    printf("NEWARRAY %ld\n", size);
    int64_t *arr = (int64_t *)calloc(size, sizeof(size));
    printf("newarray(%ld) -> %p\n", size, (void *)arr);
    return (uint64_t)arr;
}

int64_t op_iaload(int64_t *arr, int64_t index) {
    printf("iaload(%p, %ld) -> %ld\n", (void *)arr, index, arr[index]);
    return arr[index];
}

void op_iastore(int64_t *arr, int64_t index, int64_t value) {
    printf("iastore(%p, %ld, %ld)\n", (void *)arr, index, index);
    arr[index] = value;
}

X64Assembler::X64Assembler()
    : x64{4096 * 16, Xbyak::AutoGrow}, r_functions{x64.r15}, _io_added{false} {
    x64.setDefaultJmpNEAR(true);
    // store arg1, getchar
    // store arg2, putchar
    x64.mov(r_functions, x64.rdi);
}
X64Assembler::~X64Assembler() {}

void X64Assembler::compile(ostream &o) {
    o.write(x64.getCode<const char *>(), x64.getSize());

    // o <<
    // o <<
    //     "\n;============================\n; SPECIAL
    //     FUNCTIONS\n;============================\n\n"
    //     // "; __instruction_out__ simply writes the char as buffer to STDOUT
    //     with SYS_WRITE\n"
    //     "__instruction_out__:\n"
    //     "    mov [rsp-1], al ; store value of al on stack\n"
    //     "    mov rax, 1      ; SYS_WRITE\n"
    //     "    mov rdi, 1      ; STDOUT\n"
    //     "    lea rsi, [rsp-1]; store stack address in rsi\n"
    //     "    mov rdx, 1      ; single char\n"
    //     "    syscall\n"
    //     "    ret\n"
    //     "\n\n"
    //     "__instruction_in__:\n"
    //     "    mov rax, 2      ; SYS_READ\n"
    //     "    mov rdi, 0      ; STDIN\n"
    //     "    lea rsi, [rsp-1]; store stack address in rsi\n"
    //     "    mov rdx, 1      ; single char\n"
    //     "    syscall\n"
    //     "    cmp rax, -1     ; compare rax to -1\n"
    //     "    xor rax, rax    ; zero rax\n"
    //     "    jg .end         ; if rax < -1, add 0\n"
    //     "    mov al, [rsp-1] ; copy resulting read value\n"
    //     ".end:\n"
    //     "    ret\n";

    // label("__instruction_out__");
    // move(ptr[rsp - 1], al);
}
#define r_function_getchar ((0 * 8))
#define r_function_putchar ((1 * 8))
#define r_function_halt ((2 * 8))
#define r_function_error ((3 * 8))
#define r_function_calloc ((4 * 8))
#define r_function_newarray ((5 * 8))
#define r_function_iaload ((6 * 8))
#define r_function_iastore ((7 * 8))

void X64Assembler::run() {
    void *functions[] = {(void *)op_in,     (void *)putchar,
                         (void *)halt,      (void *)error,
                         (void *)calloc,    (void *)op_newarray,
                         (void *)op_iaload, (void *)op_iastore};

    x64.ready();
    auto code = x64.getCode<void (*)(void **)>();
    code(functions);

    log.panic("Shouldn't have run this oh doodoo");
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
        log.info("    [rbp + %d] = arg __obj_ref__", offset);
    } else
        offset = -8;

    for (const string &s : args) {
        _local_variables[s] = (offset += 8);
        log.info("    [rbp + %d] = arg %s", offset, s.c_str());
    }

    _local_variables["__ret_addr__"] = (offset += 8);
    log.info("    [rbp + %d] = ret addr", offset);

    _local_variables["__base_ptr__"] = (offset += 8);
    log.info("    [rbp + %d] = base ptr", offset);

    for (const string &s : vars) {
        _local_variables[s] = (offset += 8);
        log.info("    [rbp + %d] = lvar %s", offset, s.c_str());
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
    x64.sub(x64.rsp, vars.size() * 8);
    log.info("    sub rsp, %-4d             ; reserve space for lvars",
             vars.size() * 8);
}

bool X64Assembler::is_var(string name) { return _local_variables.count(name); }
void X64Assembler::BIPUSH(int8_t value) {
    log.info("    push %-14d       ; BIPUSH %d", value, value);
    x64.push(value);
}
void X64Assembler::LDC_W(string constant) {
    log.info("    push %-14d       ; LDC_W %s", constant_map[constant],
             constant.c_str());
    x64.push(constant_map[constant]);
}

void X64Assembler::DUP() {
    log.info("    mov rax, [rsp]      ; DUP");
    log.info("    push rax");

    x64.mov(x64.rax, x64.ptr[x64.rsp]);
    x64.push(x64.rax);
}

void X64Assembler::IAND() {
    log.info("    pop rax             ; IAND");
    log.info("    and [rsp], rax");

    x64.pop(x64.rax);
    x64.and_(x64.ptr[x64.rsp], x64.rax);
}
void X64Assembler::IOR() {
    log.info("    pop rax             ; IOR");
    log.info("    or [rsp], rax");

    x64.pop(x64.rax);
    x64.or_(x64.ptr[x64.rsp], x64.rax);
}

void X64Assembler::IADD() {
    log.info("    pop rax             ; IADD");
    x64.pop(x64.rax);
    x64.pop(x64.rcx);
    x64.add(x64.ecx, x64.eax);
    x64.movsxd(x64.rax, x64.ecx);
    x64.push(x64.rax);
    // #ifndef UB_OVERFLOW
    //     log.info("    mov rcx, 0xffFFffFF00000000");
    //     log.info("    or rax, rcx;");
    //     // ensure it overflows if eax + dword[rsp] overflows
    //     x64.mov(x64.rcx, 0xffFFffFF00000000ULL);
    //     x64.or_(x64.rax, x64.rcx);
    // #endif

    // log.info("    add [rsp], rax");
    // x64.add(x64.ptr[x64.rsp], x64.rax);

    // #ifndef UB_OVERFLOW
    //     log.info("    and [rsp], 0xffffffff");
    //     x64.and_(x64.qword[x64.rsp], 0xffFFffFFULL);
    // #endif
}

void X64Assembler::ISUB() {
    log.info("    pop rax                   ; ISUB");
    log.info("    sub [rsp], rax");

    x64.pop(x64.rax);
    x64.pop(x64.rcx);
    x64.sub(x64.ecx, x64.eax);
    x64.movsxd(x64.rax, x64.ecx);
    x64.push(x64.rax);
    // x64.sub(x64.word[x64.rsp], x64.eax);

#ifndef UB_UNDERFLOW
    // log.info("    and [rsp], 0xffffffff");
    // ensure that when underflow happens, those bits dont get stored
    // x64.and_(x64.qword[x64.rsp], 0xffFFffFF);
#endif
}
void X64Assembler::POP() {
    log.info("    pop rax                   ; POP");
    x64.pop(x64.rax);
}
void X64Assembler::SWAP() {
    log.info("    pop rax                   ; SWAP");
    log.info("    pop rcx");
    log.info("    push rax");
    log.info("    push rcx");

    x64.pop(x64.rax);
    x64.pop(x64.rcx);
    x64.push(x64.rax);
    x64.push(x64.rcx);
}
void X64Assembler::ILOAD(string var) {
    log.info("    mov rax [rbp - %4d]      ; ILOAD %s", _local_variables[var],
             var.c_str());
    log.info("    push rax");
    x64.mov(x64.rax, x64.ptr[x64.rbp - _local_variables[var]]);
    x64.push(x64.rax);
}
void X64Assembler::ISTORE(string var) {
    log.info("    pop rax                   ; ISTORE %s", var.c_str());
    log.info("    mov [rbp - %4d] rax", _local_variables[var]);
    x64.pop(x64.rax);
    x64.mov(x64.ptr[x64.rbp - _local_variables[var]], x64.rax);
}

void X64Assembler::IINC(string var, int8_t value) {
    log.info("    add qword [rbp - %4d], %-2d; IINC %s %d",
             _local_variables[var], value, var.c_str(), value);
    x64.add(x64.qword[x64.rbp - _local_variables[var]], value);
}

void X64Assembler::WIDE() {}
void X64Assembler::HALT() {
    log.info("    mov rax, halt          ; HALT");
    log.info("    call halt");
    x64.mov(x64.rax, x64.ptr[r_functions + r_function_halt]);
    x64.call(x64.rax);
}

void X64Assembler::ERR() {
    log.info("    mov rax, error         ; ERR");
    log.info("    call error");
    x64.mov(x64.rax, x64.ptr[r_functions + r_function_error]);
    x64.call(x64.rax);
}

void X64Assembler::IN() {
    log.info("    mov rax, getchar          ; IN");
    log.info("    call getchar");
    log.info("    push rax");
    x64.mov(x64.rax, x64.ptr[r_functions + r_function_getchar]);
    x64.call(x64.rax);
    x64.push(x64.rax);
}
void X64Assembler::OUT() {
    log.info("    pop rdi                   ; OUT");
    log.info("    mov rax, putchar");
    log.info("    call putchar");
    x64.pop(x64.rdi);
    x64.mov(x64.rax, x64.ptr[r_functions + r_function_putchar]);
    x64.call(x64.rax);
}
void X64Assembler::NOP() {}
void X64Assembler::GOTO(string label) {
    log.info("    jmp .%-20s ; GOTO %s", label.c_str(), label.c_str());
    x64.jmp(concat(fname, "#", label));
}
void X64Assembler::ICMPEQ(string label) {
    log.info("    pop rax                   ; ICMPEQ %s", label.c_str());
    log.info("    pop rcx");
    log.info("    cmp rax, rcx");
    log.info("    je  .%s", label.c_str());

    x64.pop(x64.rax);
    x64.pop(x64.rcx);
    x64.cmp(x64.rax, x64.rcx);
    x64.je(concat(fname, "#", label));
}
void X64Assembler::IFLT(string label) {
    log.info("    pop rax                   ; IFLT %s", label.c_str());
    log.info("    cmp rax, 0");
    log.info("    jl .%s", label.c_str());

    x64.pop(x64.rax);
    x64.cmp(x64.rax, 0);
    x64.jl(concat(fname, "#", label));
}
void X64Assembler::IFEQ(string label) {
    log.info("    pop rax                   ; IFEQ %s", label.c_str());
    log.info("    test rax, rax");
    log.info("    jz .%s", label.c_str());

    x64.pop(x64.rax);
    x64.cmp(x64.rax, 0);
    x64.je(concat(fname, "#", label));
}

void X64Assembler::INVOKEVIRTUAL(string func_name) {
    log.info("    call %20s ; INVOKEVIRTUAL %s", func_name.c_str(),
             func_name.c_str());
    log.info("    push rax");
    x64.call(func_name);
    x64.push(x64.rax);
}

void X64Assembler::IRETURN() {
    log.info("    pop rax                   ; IRETURN");
    log.info("    mov rcx, [rbp - %3d]", _local_variables["__ret_addr__"]);
    log.info("    mov rsp, rbp");
    log.info("    mov rbp, [rbp - %3d]", _local_variables["__base_ptr__"]);
    log.info("    jmp rcx");

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

    x64.pop(x64.rdi);
    // x64.mov(x64.rsi, 8);
    x64.mov(x64.rax, x64.ptr[r_functions + r_function_newarray]);
    x64.call(x64.rax);
    x64.push(x64.rax);
}

void X64Assembler::IALOAD() {
    log.info("    pop rdi                   ; IALOAD");
    log.info("    pop rsi");
    log.info("    mov rax, iaload");
    log.info("    call rax");
    log.info("    push rax");

    x64.pop(x64.rdi);
    x64.pop(x64.rsi);
    x64.mov(x64.rax, x64.ptr[r_functions + r_function_iaload]);
    x64.call(x64.rax);
    x64.push(x64.rax);

    // log.info("    pop rcx                  ; IALOAD");
    // log.info("    pop rax");
    // log.info("    mov rax, [rcx + rax * 8]");
    // log.info("    push rax");

    // x64.pop(x64.rax);
    // x64.pop(x64.rcx);
    // x64.mov(x64.rax, x64.ptr[x64.rcx + x64.rax * 8]);
    // x64.push(x64.rax);
}

void X64Assembler::IASTORE() {
    log.info("    pop rdi                   ; IASTORE");
    log.info("    pop rsi");
    log.info("    mov rax, iastore");
    log.info("    call rax");
    log.info("    push rax");

    x64.pop(x64.rdi);
    x64.pop(x64.rsi);
    x64.mov(x64.rax, x64.ptr[r_functions + r_function_iastore]);
    x64.call(x64.rax);
    // log.info("    pop rcx                  ; IASTORE");
    // log.info("    pop rax");
    // log.info("    pop rdx");
    // log.info("    mov [rcx + rax * 8], rdx");

    // x64.pop(x64.rcx);
    // x64.pop(x64.rax);
    // x64.pop(x64.rdx);
    // x64.mov(x64.ptr[x64.rcx + x64.rax * 8], x64.rdx);
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