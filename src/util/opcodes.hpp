#ifndef UTIL_OPCODES_HPP
#define UTIL_OPCODES_HPP

// clang-format off
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
const u8 op_iaload         = 0xD2;
const u8 op_iastore        = 0xD3;
const u8 op_gc             = 0xD4;
const u8 op_netbind        = 0xE1;
const u8 op_netconnect     = 0xE2;
const u8 op_netin          = 0xE3;
const u8 op_netout         = 0xE4;
const u8 op_netclose       = 0xE5;
const u8 op_shl            = 0x70;
const u8 op_shr            = 0x71;
const u8 op_imul           = 0x72;
const u8 op_idiv           = 0x73;

// clang-format on

// Better way of doing things
// clang-format off
enum class opcode : u8
{
    BIPUSH         = 0x10,
    DUP            = 0x59,
    ERR            = 0xFE,
    GOTO           = 0xA7,
    HALT           = 0xFF,
    IADD           = 0x60,
    IAND           = 0x7E,
    IFEQ           = 0x99,
    IFLT           = 0x9B,
    ICMPEQ         = 0x9F,
    IINC           = 0x84,
    ILOAD          = 0x15,
    IN             = 0xFC,
    INVOKEVIRTUAL  = 0xB6,
    IOR            = 0xB0,
    IRETURN        = 0xAC,
    ISTORE         = 0x36,
    ISUB           = 0x64,
    LDC_W          = 0x13,
    NOP            = 0x00,
    OUT            = 0xFD,
    POP            = 0x57,
    SWAP           = 0x5F,
    WIDE           = 0xC4,

    NEWARRAY       = 0xD1,
    IALOAD         = 0xD2,
    IASTORE        = 0xD3,
    GC             = 0xD4,
    NETBIND        = 0xE1,
    NETCONNECT     = 0xE2,
    NETIN          = 0xE3,
    NETOUT         = 0xE4,
    NETCLOSE       = 0xE5,

    SHL            = 0x70,
    SHR            = 0x71,
    IMUL           = 0x72,
    IDIV           = 0x73,

    INVALID        = 0x80,
};
// clang-format on

constexpr opcode opcode_parse(u8 i) {
    switch(static_cast<opcode>(i)) {
        case opcode::BIPUSH:
        case opcode::DUP:
        case opcode::ERR:
        case opcode::GOTO:
        case opcode::HALT:
        case opcode::IADD:
        case opcode::IAND:
        case opcode::IFEQ:
        case opcode::IFLT:
        case opcode::ICMPEQ:
        case opcode::IINC:
        case opcode::ILOAD:
        case opcode::IN:
        case opcode::INVOKEVIRTUAL:
        case opcode::IOR:
        case opcode::IRETURN:
        case opcode::ISTORE:
        case opcode::ISUB:
        case opcode::LDC_W:
        case opcode::NOP:
        case opcode::OUT:
        case opcode::POP:
        case opcode::SWAP:
        case opcode::WIDE:
        case opcode::NEWARRAY:
        case opcode::IALOAD:
        case opcode::IASTORE:
        case opcode::GC:
        case opcode::NETBIND:
        case opcode::NETCONNECT:
        case opcode::NETIN:
        case opcode::NETOUT:
        case opcode::NETCLOSE:
        case opcode::SHL:
        case opcode::SHR:
        case opcode::IMUL:
        case opcode::IDIV:
            return static_cast<opcode>(i);
        default:
            return opcode::INVALID;
    }
}

#endif