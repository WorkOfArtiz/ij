#include "assembler.hpp"
#include "../logger.hpp"

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
        BIPUSH(value);
        return;
    }

    std::stringstream s;
    s << "c_";
    if (value < 0) {
        s << "n" << -value;
    } else {
        s << value;
    }

    std::string cn = s.str();
    constant(cn, value);
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
