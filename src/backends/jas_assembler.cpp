#include <iostream>
#include "jas_assembler.hpp"

void JASAssembler::compile(ostream &o) {
    if (!constant_order.empty())
        o << ".constant" << '\n';

    for (string &s : constant_order) {
        i32 value = constant_map[s];
        o << "    " << s << " 0x" << std::hex << value << '\n';
    }

    if (!constant_order.empty())
        o << ".end-constant" << '\n' << '\n';

    o << ".main\n";
    o << cs.str();

    if (_fn_declared)
        o << ".end-method";
    else
        o << ".end-main";

    o << '\n';
}

// bool JASAssembler::is_constant(string name) { return consts.count(name) == 1;
// }
void JASAssembler::label(string name) { cs << name << ":\n"; }
void JASAssembler::function(string name, vector<string> args,
                            vector<string> vars) {
    if (_fn_declared)
        cs << ".end-method\n\n";
    else {
        cs << ".end-main\n\n";
        _fn_declared = true;
    }

    cs << ".method " << name << "(";
    for (size_t i = 0; i < args.size(); i++) {
        cs << args[i];
        if (i + 1 != args.size())
            cs << ", ";
    }
    cs << ")\n";

    if (!vars.empty()) {
        cs << ".var\n";
        for (auto var : vars)
            cs << "    " << var << '\n';
        cs << ".end-var\n";
    }

    accessible_vars.clear();
    for (auto arg : args)
        accessible_vars.insert(arg);

    for (auto var : vars)
        accessible_vars.insert(var);
}

bool JASAssembler::is_var(string name) { return accessible_vars.count(name); }

void JASAssembler::BIPUSH(int8_t value) {
    cs << "    BIPUSH " << (int)value << '\n';
}
void JASAssembler::DUP() { cs << "    DUP\n"; }
void JASAssembler::IADD() { cs << "    IADD\n"; }
void JASAssembler::IAND() { cs << "    IAND\n"; }
void JASAssembler::IOR() { cs << "    IOR\n"; }
void JASAssembler::ISUB() { cs << "    ISUB\n"; }
void JASAssembler::POP() { cs << "    POP\n"; }
void JASAssembler::SWAP() { cs << "    SWAP\n"; }
void JASAssembler::LDC_W(string constant) {
    cs << "    LDC_W " << constant << '\n';
}
void JASAssembler::ILOAD(string var) { cs << "    ILOAD " << var << '\n'; }
void JASAssembler::IINC(string var, int8_t value) {
    cs << "    IINC " << var << ' ' << (int)value << '\n';
}
void JASAssembler::ISTORE(string var) { cs << "    ISTORE " << var << '\n'; }
void JASAssembler::WIDE() { cs << "    WIDE\n"; }
void JASAssembler::HALT() { cs << "    HALT\n"; }
void JASAssembler::ERR() { cs << "    ERR\n"; }
void JASAssembler::IN() { cs << "    IN\n"; }
void JASAssembler::OUT() { cs << "    OUT\n"; }
void JASAssembler::NOP() { cs << "    NOP\n"; }
void JASAssembler::GOTO(string label) { cs << "    GOTO " << label << '\n'; }
void JASAssembler::ICMPEQ(string label) {
    cs << "    ICMPEQ " << label << '\n';
}
void JASAssembler::IFLT(string label) { cs << "    IFLT " << label << '\n'; }
void JASAssembler::IFEQ(string label) { cs << "    IFEQ " << label << '\n'; }
void JASAssembler::INVOKEVIRTUAL(string func_name) {
    cs << "    INVOKEVIRTUAL " << func_name << '\n';
}
void JASAssembler::IRETURN() { cs << "    IRETURN\n"; }
void JASAssembler::NEWARRAY() { cs << "    NEWARRAY\n"; }
void JASAssembler::IALOAD() { cs << "    IALOAD\n"; }
void JASAssembler::IASTORE() { cs << "    IASTORE\n"; }
void JASAssembler::NETBIND() { cs << "    NETBIND\n"; }
void JASAssembler::NETCONNECT() { cs << "    NETCONNECT\n"; }
void JASAssembler::NETIN() { cs << "    NETIN\n"; }
void JASAssembler::NETOUT() { cs << "    NETOUT\n"; }
void JASAssembler::NETCLOSE() { cs << "    NETCLOSE\n"; }