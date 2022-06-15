#include <frontends/ijvm/compile.hpp>
#include <util/opcodes.hpp>
#include <util/util.hpp>

#define IJVM_MAGIC 0x1deadfad

static inline ssize_t max(ssize_t i1, ssize_t i2) {
    return i1 >= i2 ? i1 : i2;
}

static inline bool is_final(opcode o) {
    return in(o, {opcode::HALT, opcode::ERR, opcode::IRETURN});
}

static inline bool has_var_arg(opcode o) {
    return in(o, {opcode::ILOAD, opcode::ISTORE, opcode::IINC});
}

static inline bool has_jmp_arg(opcode o) {
    return in(o, {opcode::GOTO, opcode::IFEQ, opcode::IFLT, opcode::ICMPEQ});
}

static u16 read_wide(Buffer::Reader &r, bool wide) {
    return (wide ? r.read<u16>(Endian::Big) : r.read<u8>());
}

static i32 ijvm_main_local_count(Buffer::Reader r) {
    std::vector<size_t>todo{0}; // Places to scan
    std::set<size_t> visited;

    ssize_t var_count = 0;

    while (!todo.empty()) {
        r.seek(pop(todo));

        while (r.has_next<u8>()) {
            size_t offset = r.position();
            if (contains(visited, offset)) {
                log.info("Already visited offset %d", offset);
                break;
            }
            visited.insert(offset);

            bool wide = false;
            u8 raw =r.read<u8>();
            opcode o = opcode_parse(raw);

            // handle wide case
            while (o == opcode::WIDE) {
                log.info("Opcode wide");
                wide = true;

                raw = r.read<u8>();
                o = opcode_parse(raw);
            }

            if (has_var_arg(o)) {
                u16 var_index = read_wide(r, wide);
                log.info("Opcode Var (var=%d) {wide=%s}", var_index, wide ? "true" : "false");
                var_count = max(var_count, var_index + 1);

                if (o == opcode::IINC)
                    r.read<u8>();
            }
            else if (is_final(o)){
                log.info("Opcode Final");

                break;
            }
            else if (in(o, {opcode::LDC_W, opcode::INVOKEVIRTUAL})) {
                log.info("Opcode LDC/INVOKE");
                r.read<u16>();
            }
            else if (o == opcode::BIPUSH) {
                u8 arg = r.read<u8>();
                log.info("Opcode Bipush %d", arg);
            }
            else if (has_jmp_arg(o)) {
                log.info("Opcode Jmp");
                todo.push_back(offset + r.read<i16>(Endian::Big));

                if (o == opcode::GOTO)
                    break;
            }
            else if (o == opcode::INVALID) {
                log.info("Main contains unknown opcode %x", o);
            }
            else {
                log.info("Opcode Stack");
            }
        }
    }

    return var_count;
}

static std::string &get_local_var_name(u16 index, std::vector<std::string> &args, std::vector<std::string> &vars, bool is_main) {
    if (!is_main) {
        index--; // skip objref
    }

    if (index < args.size()) {
        return args[index];
    }

    index -= args.size();

    if (index < vars.size()) {
        return vars[index];
    }

    throw std::runtime_error{sprint("needed to get the name of a local var but function didn't have that many vars")};
}

void ijvm_compile_method(Buffer::Reader &r, std::string name, u16 nargs, u16 nvars, Assembler &a, std::vector<size_t> &funcs_found, const std::vector<i32> &constants) {
    std::vector<size_t>todo{r.position()}; // Places to scan
    std::set<size_t> visited;
    bool is_main = name == "main";

    log.info("creating func with %d args and %d vars", nargs, nvars);
    std::vector<std::string> args;
    for (u32 i = 0; i < nargs; i++) {
        args.emplace_back(sprint("arg_%d", i));
    }
    std::vector<std::string> variables;
    for (u32 i = 0; i < nvars; i++) {
        variables.emplace_back(sprint("lvar_%d", i));
    }
    a.function(name, args, variables);
    log.info("function signature for %s created", name.c_str());

    while (!todo.empty()) {
        r.seek(pop(todo));
        log.info("starting point %d", r.position());

        while (r.has_next<u8>()) {
            log.info("reading from %d", r.position());

            size_t offset = r.position();
            if (contains(visited, offset)) {
                log.info("Already visited offset %d", offset);
                goto stop_reading;
            }
            visited.insert(offset);

            a.label(sprint("loc_%04x", offset));
            u8 raw = r.read<u8>();
            opcode code = opcode_parse(raw);
            log.info("read op %#x", raw);

            bool wide = false;
            while (code == opcode::WIDE) {
                a.WIDE();
                wide = true;
                // log.info("extra wide");
            }

            switch (code) {
                case opcode::DUP:           { a.DUP(); break;}
                case opcode::ERR:           { a.ERR(); goto stop_reading; }
                case opcode::HALT:          { a.HALT(); goto stop_reading; }
                case opcode::IADD:          { a.IADD(); break;}
                case opcode::IAND:          { a.IAND(); break;}
                case opcode::IN:            { a.IN(); break;}
                case opcode::IOR:           { a.IOR(); break;}
                case opcode::ISUB:          { a.ISUB(); break;}
                case opcode::NOP:           { a.NOP(); break;}
                case opcode::OUT:           { a.OUT(); break;}
                case opcode::POP:           { a.POP(); break;}
                case opcode::SWAP:          { a.SWAP(); break;}
                case opcode::WIDE:          { a.WIDE(); break;}
                case opcode::NEWARRAY:      { a.NEWARRAY(); break;}
                case opcode::IALOAD:        { a.IALOAD(); break;}
                case opcode::IASTORE:       { a.IASTORE(); break;}
                case opcode::GC:            { a.GC(); break;}
                case opcode::NETBIND:       { a.NETBIND(); break;}
                case opcode::NETCONNECT:    { a.NETCONNECT(); break;}
                case opcode::NETIN:         { a.NETIN(); break;}
                case opcode::NETOUT:        { a.NETOUT(); break;}
                case opcode::NETCLOSE:      { a.NETCLOSE(); break;}
                case opcode::SHL:           { a.SHL(); break;}
                case opcode::SHR:           { a.SHR(); break;}
                case opcode::IMUL:          { a.IMUL(); break;}
                case opcode::IDIV:          { a.IDIV(); break;}

                case opcode::BIPUSH:        { a.BIPUSH(r.read<u8>()); break; }

                case opcode::ILOAD:         { a.ILOAD(get_local_var_name(read_wide(r, wide), args, variables, is_main)); break;}
                case opcode::ISTORE:        { a.ISTORE(get_local_var_name(read_wide(r, wide), args, variables, is_main)); break;}
                case opcode::IINC:          {
                    std::string varname = get_local_var_name(read_wide(r, wide), args, variables, is_main);
                    a.IINC(varname, r.read<i8>());
                    break;
                }

                case opcode::LDC_W:         { a.LDC_W(sprint("constant_%d", r.read<u16>(Endian::Big))); break;}
                case opcode::INVOKEVIRTUAL: {
                    u16 const_index = r.read<u16>(Endian::Big);
                    size_t func_addr = constants[const_index];
                    funcs_found.push_back(func_addr);
                    a.INVOKEVIRTUAL(sprint("func_%04x", func_addr)); break;}
                case opcode::IRETURN:       { a.IRETURN(); goto stop_reading; }

                case opcode::GOTO:          {
                    size_t jump_target = offset + r.read<i16>(Endian::Big);
                    a.GOTO(sprint("loc_%04x", jump_target));
                    todo.push_back(jump_target);
                    goto stop_reading;
                }
                case opcode::IFEQ:          {
                    size_t jump_target = offset + r.read<i16>(Endian::Big);
                    a.IFEQ(sprint("loc_%04x", jump_target));
                    todo.push_back(jump_target);
                    break;
                }
                case opcode::IFLT:{
                    size_t jump_target = offset + r.read<i16>(Endian::Big);
                    a.IFLT(sprint("loc_%04x", jump_target));
                    todo.push_back(jump_target);
                    break;
                }
                case opcode::ICMPEQ:{
                    size_t jump_target = offset + r.read<i16>(Endian::Big);
                    a.ICMPEQ(sprint("loc_%x", jump_target));
                    todo.push_back(jump_target);
                    break;
                }
                case opcode::INVALID:       {
                    throw std::runtime_error{sprint("Encountered illegal instruction %x", raw)};
                }
                default: break;
            }
        }

        log.panic("Didn't have a next? Trailing program!");

        stop_reading: log.info("end of linear trail, now checking GOTO targets");
    }
}

void ijvm_compile(Buffer &b, Assembler &a)
{
    Buffer::Reader reader = b.readinator();

    u32 magic = reader.read<u32>(Endian::Big);
    if (magic != IJVM_MAGIC) {
        throw std::runtime_error{sprint("Magic was supposed to be 0x%x but was 0x%x", 0x1deadfad, magic)};
    }

    reader.read<i32>(); // const pool address
    u32 const_size = reader.read<i32>(Endian::Big);
    log.info("There are %d constants", const_size / 4);

    std::vector<i32> constants;
    for (u32 i = 0; i < const_size / 4; i++) {
        i32 constant_value = reader.read<i32>(Endian::Big);
        a.constant(sprint("constant_%d", i), constant_value);
        constants.push_back(constant_value);
    }

    reader.read<i32>(); // text address
    u32 text_size = reader.read<i32>(Endian::Big);
    Buffer text{b, reader.position(), reader.position() + text_size};

    i32 i = ijvm_main_local_count(text.readinator());
    log.info("IJVM analysis yielded %d local vars", i);

    std::vector<size_t> funcs;
    Buffer::Reader program_reader = text.readinator();
    program_reader.seek(0);

    ijvm_compile_method(program_reader, "main", 0, i, a, funcs, constants);

    std::set<size_t> funcs_visited{};

    while (!funcs.empty()) {
        size_t func_addr = pop(funcs);
        if (contains(funcs_visited, func_addr)) {
            continue;
        }
        funcs_visited.insert(func_addr);

        program_reader.seek(func_addr);

        u16 nargs = program_reader.read<u16>(Endian::Big);
        u16 vars = program_reader.read<u16>(Endian::Big);

        ijvm_compile_method(program_reader, sprint("func_%04x", func_addr), nargs - 1, vars, a, funcs, constants);
    }
}
