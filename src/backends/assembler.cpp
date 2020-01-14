#include "assembler.hpp"
#include "../logger.hpp"
#include "../util.hpp"

bool Assembler::is_constant(string name) {
    return constant_map.count(name) != 0;
}

void Assembler::constant(string name, i32 value) {
    if (!is_constant(name))
        constant_order.push_back(name);

    constant_map[name] = value;
}

void Assembler::PUSH_VAL(int32_t value) {
    if (value >= -128 && value <= 127) {
        // log.info("PUSH_VAL chose a bipush for value %d", value);
        BIPUSH(value);
        return;
    }

    std::string cn = sprint("__const_%s%s__", abs(value), value < 0 ? "n" : "");
    if (!is_constant(cn))
        constant(cn, value);

    // log.info("PUSH_VAL chose an LDC_W for value %d (const %s)", value,
    //  cn.c_str());
    LDC_W(cn);
}

void Assembler::SET_VAR(string var, int32_t value) {
    PUSH_VAL(value);
    ISTORE(var);
}

void Assembler::INC_VAR(string var, int32_t value) {
    if (value >= -128 && value <= 127) {
        ILOAD(var);
        PUSH_VAL(value);
        ISTORE(var);
    }
}

void Assembler::IMUL(i32 value) {
    i32 bits = 0;
    log.info("IMUL %d", value);

    if (!value) {
        this->POP();
        this->BIPUSH(0);
        return;
    }

    bool sign = false;
    if (value < 0) {
        value = -value;
        sign = true;
        this->BIPUSH(0);
        this->SWAP();
    }

    for (i32 shift_value = value; shift_value & ~1; shift_value >>= 1) {
        if (shift_value & 1) {
            this->DUP();
            bits++;
        }

        this->DUP();
        this->IADD();
    }

    log.info("    %d had %d bits set", value, bits);
    for (; bits > 0; bits--)
        this->IADD();

    if (sign)
        this->ISUB();
}
