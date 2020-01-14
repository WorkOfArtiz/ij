#include "ijvm_assembler.hpp"
#include <util/logger.hpp>
#include <util/util.hpp>

IJVMAssembler::IJVMAssembler() : current_func{"main"} {}

IJVMAssembler::~IJVMAssembler() {}

void IJVMAssembler::compile(ostream &o) {
    Endian e = Endian::Big;

    /* collect constants */
    vector<u32> consts;
    std::unordered_map<string, u32> findexes;

    for (string &s : constant_order)
        consts.push_back(constant_map[s]);

    for (std::pair<string, u32> p : faddrs) {
        findexes[p.first] = consts.size();
        consts.push_back(p.second);
    }

    link(findexes);

    buffer output;

    /* put magic word */
    output.append<u32>(0x1deadfad, e);

    /* write constants block */
    output.append<u32>(0xd000d000, e);
    output.append<u32>(consts.size() * sizeof(i32), e);
    for (u32 &c : consts)
        output.append<i32>(c, e);

    /* write text block */
    output.append<u32>(0x00000000, e);
    output.append<u32>(code.size(), e);
    output.append<const buffer &>(code);

    /* write function addresses (symbol tables) */
    buffer symbol;
    for (std::pair<string, u32> p : faddrs) {
        symbol.append<u32>(p.second, e);
        symbol.append<const char *>(p.first.c_str(), e);
        symbol.append<u8>(0);
    }

    /* write function symbols */
    output.append<u32>(0xEEeeEEee, e);
    output.append<u32>(symbol.size(), e);
    output.append<const buffer &>(symbol);

    symbol.clear(); /* clear symbols */

    for (std::pair<string, u32> p : laddrs) {
        symbol.append<u32>(p.second, e);
        symbol.append<const char *>(p.first.c_str(), e);
        symbol.append<u8>(0);
    }

    /* write label symbols */
    output.append<u32>(0xFFffFFff, e);
    output.append<u32>(symbol.size(), e);
    output.append<const buffer &>(symbol);

    o << output;
}

void IJVMAssembler::label(string name) {
    // log.info("%s <- %u", sprint("%s#%s", current_func, name).c_str(),
    // code.size());
    laddrs[sprint("%s#%s", current_func, name)] = code.size();
}

void IJVMAssembler::function(string name, vector<string> args,
                             vector<string> vars) {
    // Since the main 'function' is special, we skip it here
    if (name == "main")
        return;

    // log.info("New function made: %s", name.c_str());
    current_func = name;
    faddrs[name] = code.size();
    code.append<u16>(args.size() + 1, Endian::Big);
    code.append<u16>(vars.size(), Endian::Big);

    this->vars.clear();
    this->vars.push_back("OBJREF");
    this->vars.insert(this->vars.end(), args.begin(), args.end());
    this->vars.insert(this->vars.end(), vars.begin(), vars.end());
}

bool IJVMAssembler::is_var(string name) { return contains(vars, name); }

void IJVMAssembler::BIPUSH(int8_t value) {
    code.append<u8>(op_bipush);
    code.append<i8>(value);
}

void IJVMAssembler::DUP() { code.append<u8>(op_dup); }

void IJVMAssembler::IADD() { code.append<u8>(op_iadd); }

void IJVMAssembler::IAND() { code.append<u8>(op_iand); }

void IJVMAssembler::IOR() { code.append<u8>(op_ior); }

void IJVMAssembler::ISUB() { code.append<u8>(op_isub); }

void IJVMAssembler::POP() { code.append<u8>(op_pop); }

void IJVMAssembler::SWAP() { code.append<u8>(op_swap); }

void IJVMAssembler::LDC_W(string constant) {
    if (!is_constant(constant)) {
        log.info("LDC_W couldn't find constant %s", constant.c_str());
        throw std::runtime_error{"Tried calling LDC_W on none-existing const"};
    }
    code.append<u8>(op_ldc_w);
    code.append<u16>(indexOf(constant_order, constant), Endian::Big);
}

void IJVMAssembler::ILOAD(string var) {
    int index = indexOf(vars, var);
    if (index < 0)
        throw std::runtime_error{"Tried calling ILOAD on none-existing var"};

    if (index > 255)
        WIDE();

    // log.info("ILOAD %s, index=%d", var.c_str(), index);

    code.append<u8>(op_iload);

    if (index > 255)
        code.append<u16>(static_cast<u16>(index));
    else
        code.append<u8>(static_cast<u8>(index));
}

void IJVMAssembler::IINC(string var, int8_t value) {
    int index = indexOf(vars, var);
    if (index < 0)
        throw std::runtime_error{"Tried calling IINC on none-existing var"};

    if (index > 255)
        WIDE();

    code.append<u8>(op_iinc);

    if (index > 255)
        code.append<u16>(static_cast<u16>(index));
    else
        code.append<u8>(static_cast<u8>(index));

    code.append<i8>(value);
}

void IJVMAssembler::ISTORE(string var) {
    int index = indexOf(vars, var);
    if (index < 0)
        throw std::runtime_error{"Tried calling ISTORE on none-existing var"};

    if (index > 255)
        WIDE();

    code.append<u8>(op_istore);

    if (index > 255)
        code.append<u16>(static_cast<u16>(index));
    else
        code.append<u8>(static_cast<u8>(index));
}

void IJVMAssembler::WIDE() { code.append<u8>(op_wide); }

void IJVMAssembler::HALT() { code.append<u8>(op_halt); }

void IJVMAssembler::ERR() { code.append<u8>(op_err); }

void IJVMAssembler::IN() { code.append<u8>(op_in); }

void IJVMAssembler::OUT() { code.append<u8>(op_out); }

void IJVMAssembler::NOP() { code.append<u8>(op_nop); }

void IJVMAssembler::GOTO(string label) {
    jmpaddrs[code.size()] = sprint("%s#%s", current_func, label);
    code.append<u8>(op_goto);
    code.append<i16>(0); // overwritten at link time
}

void IJVMAssembler::ICMPEQ(string label) {
    jmpaddrs[code.size()] = sprint("%s#%s", current_func, label);
    code.append<u8>(op_icmpeq);
    code.append<i16>(0); // overwritten at link time
}

void IJVMAssembler::IFLT(string label) {
    jmpaddrs[code.size()] = sprint("%s#%s", current_func, label);
    code.append<u8>(op_iflt);
    code.append<i16>(0); // overwritten at link time
}
void IJVMAssembler::IFEQ(string label) {
    jmpaddrs[code.size()] = sprint("%s#%s", current_func, label);
    code.append<u8>(op_ifeq);
    code.append<i16>(0); // overwritten at link time
}

void IJVMAssembler::INVOKEVIRTUAL(string func_name) {
    invokes[code.size()] = func_name;
    code.append<u8>(op_invokevirtual);
    code.append<i16>(0); // overwritten at link time
}

void IJVMAssembler::IRETURN() { code.append<u8>(op_ireturn); }

/* bonus extensions */
void IJVMAssembler::NEWARRAY() { code.append<u8>(op_newarray); }

void IJVMAssembler::IALOAD() { code.append<u8>(op_iaload); }

void IJVMAssembler::IASTORE() { code.append<u8>(op_iastore); }

void IJVMAssembler::GC() { code.append<u8>(op_gc); }

void IJVMAssembler::NETBIND() { code.append<u8>(op_netbind); }

void IJVMAssembler::NETCONNECT() { code.append<u8>(op_netconnect); }

void IJVMAssembler::NETIN() { code.append<u8>(op_netin); }

void IJVMAssembler::NETOUT() { code.append<u8>(op_netout); }

void IJVMAssembler::NETCLOSE() { code.append<u8>(op_netclose); }

void IJVMAssembler::link(std::unordered_map<string, u32> findexes) {
    /* first compile the jump instructions */
    for (std::pair<u32, string> p : jmpaddrs) {
        if (laddrs.count(p.second) != 1) {
            log.info("All addresses");
            for (std::pair<string, u32> p : laddrs)
                log.info("    - %s", p.first.c_str());

            log.panic("No mapping %s to address", p.second.c_str());
        }

        u32 address = laddrs[p.second];
        code.write(static_cast<i16>(address - p.first), p.first + 1,
                   Endian::Big);
    }

    for (std::pair<u32, string> p : invokes) {
        if (findexes.count(p.second) == 0) {
            log.info("All function names");
            for (std::pair<string, u32> p : findexes)
                log.info("    - %s", p.first.c_str());

            log.panic("INVOKEVIRTUAL call to unfound function '%s'",
                      p.second.c_str());
        }

        code.write<u16>(findexes[p.second], p.first + 1, Endian::Big);
    }
}