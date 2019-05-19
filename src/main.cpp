#include <iostream>
#include <fstream>

#include "logger.hpp"
#include "backends/ijvm_assembler.hpp"
#include "backends/jas_assembler.hpp"
#include "ij/parse.hpp"
//#include "buffer.hpp"
//#include "ijvm.hpp"
#include "ij/lexer.hpp"

/*
 * Set up the calling of the main function
 */
void call_main(Assembler &a, bool verbose) {
    if (verbose)
        log.info("constructing entry point");

    a.constant("__OBJREF__", 0xdeadc001);

    a.LDC_W("__OBJREF__");
    a.INVOKEVIRTUAL("__main__");
    a.IFLT("__error__");
    a.label("__correct__");
    a.HALT();
    a.label("__error__");
    a.ERR();
}

static void parse_options(int argc, char **argv, string &input, string &output,
                          bool &assembly, bool &verbose) {
    int positional = -1;
    assembly = verbose = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            std::cerr << "Usage: ij [options] in.ij" << std::endl
                      << "   -o, --output   - output file (stdout by default)"
                      << std::endl
                      << "   -S, --assembly - generates jas assembly, compiled "
                         "otherwise"
                      << std::endl
                      << "   -v, --verbose  - prints verbose info" << std::endl;
            exit(-1);
        } else if (arg == "-o" || arg == "--output") {
            output = argv[++i];
        } else if (arg == "-S" || arg == "--assembly") {
            assembly = true;
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
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
    bool assembly, verbose;
    string input = "";
    string output = "";

    parse_options(argc, argv, input, output, assembly, verbose);

    Assembler *a;
    if (assembly)
        a = new JASAssembler{};
    else
        a = new IJVMAssembler{};

    Lexer l;

    try {
        if (verbose)
            log.info("reading file %s", input.c_str());

        call_main(*a, verbose);
        l.add_source(input);

        Program *p = parse_program(l);

        if (verbose)
            log.info("constants %lu", p->consts.size());

        for (auto c : p->consts) {
            if (verbose)
                log.info("    - %s", str(*c).c_str());
            a->constant(c->name, c->value);
        }

        if (verbose) {
            log.info("functions %lu", p->funcs.size());
            for (auto f : p->funcs)
                log.info("function: %s", str(*f).c_str());
        }

        for (auto fiter : p->funcs)
            fiter->compile(*a);

        if (output == "")
            a->compile(std::cout);
        else {
            if (verbose)
                log.info("Writing to file %s", output.c_str());

            std::ofstream out_file;
            out_file.open(output, std::ios::binary);
            if (!out_file.is_open())
                log.panic("File %s couldn't be opened for writing",
                          output.c_str());

            a->compile(out_file);
            out_file.close();
        }

    } catch (std::runtime_error &r) {
        log.panic("while compiling %s, %s", input.c_str(), r.what());
    }

    return 0;
}