#include <iostream>
#include <fstream>
#include <memory>

#include "logger.hpp"
#include "backends/ijvm_assembler.hpp"
#include "backends/jas_assembler.hpp"
#include "ij/parse.hpp"
//#include "buffer.hpp"
//#include "ijvm.hpp"
#include "ij/lexer.hpp"

/*
 * Add default main, calling __main__, this is to avoid
 * the shitty local vars of the entry point problem.
 */
static void add_main(Program &p) {
    JasStmt *halt = new JasStmt{"HALT"};
    JasStmt *err = new JasStmt{"ERR"};

    Function *f = new Function(
        "main", {}, {new IfStmt(new FunExpr("__main__", {}), {halt}, {err})});

    p.funcs.insert(p.funcs.begin(), f);
}

static void parse_options(int argc, char **argv, string &input, string &output,
                          bool &assembly) {
    int positional = -1;
    assembly = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            std::cerr << "Usage: ij [options] in.ij" << std::endl
                      << "   -o, --output   - output file (stdout by default)"
                      << std::endl
                      << "   -S, --assembly - generates jas assembly, compiled "
                         "otherwise"
                      << std::endl
                      << "   -v, --verbose  - prints verbose info" << std::endl
                      << "   -d, --debug    - prints debug info" << std::endl;
            exit(-1);
        } else if (arg == "-o" || arg == "--output") {
            output = argv[++i];
        } else if (arg == "-S" || arg == "--assembly") {
            assembly = true;
        } else if (arg == "-v" || arg == "--verbose") {
            log.set_log_level(LogLevel::success);
        } else if (arg == "-d" || arg == "--debug") {
            log.set_log_level(LogLevel::info);
        } else if (positional == -1) {
            input = arg;
            positional = 0;
        } else {
            if (arg[0] == '-')
                std::cerr << "Unknown option " << arg;
            else
                std::cerr << "Only one positional argument allowed: '" << arg
                          << "'";

            std::cerr << std::endl;
            exit(-1);
        }
    }

    if (positional == -1) {
        std::cerr << "Not enough options" << std::endl;
        exit(-1);
    }
}

int main(int argc, char **argv) {
    bool assembly;
    string input = "";
    string output = "";

    parse_options(argc, argv, input, output, assembly);

    std::unique_ptr<Assembler> a;

    if (assembly)
        a = std::make_unique<JASAssembler>();
    else
        a = std::make_unique<IJVMAssembler>();

    Lexer l;

    try {
        log.info("reading file %s", input.c_str());

        l.add_source(input);

        std::unique_ptr<Program> p{parse_program(l)};
        add_main(*p);

        log.info("constants %lu", p->consts.size());
        for (auto c : p->consts) {
            log.info("    - %s", cstr(*c));
            a->constant(c->name, c->value);
        }

        log.info("functions %lu", p->funcs.size());
        for (auto f : p->funcs)
            log.info("function: %s", cstr(*f));

        for (auto fiter : p->funcs)
            fiter->compile(*a);

        if (output == "")
            a->compile(std::cout);
        else {
            log.info("Writing to file %s", output.c_str());

            std::ofstream out_file;
            out_file.open(output, std::ios::binary);
            if (!out_file.is_open())
                log.panic("File %s couldn't be opened for writing",
                          output.c_str());

            a->compile(out_file);
            out_file.close();
        }

        log.success("Successfully compiled program");

    } catch (std::runtime_error &r) {
        log.panic("while compiling %s, %s", input.c_str(), r.what());
    }
    return 0;
}