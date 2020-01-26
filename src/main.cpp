#include <iostream>
#include <vector>
#include <fstream>
#include <memory>

#include <frontends/ij/compile.hpp>
#include <frontends/jas/compile.hpp>
#include <backends/ijvm_assembler.hpp>
#include <backends/jas_assembler.hpp>
#include <backends/x64_assembler.hpp>
#include <util/logger.hpp>

struct options {
    bool run = false;             // whether we run or compile
    std::string src_file;         // file to be compiled
    std::string input_file = "";  // only relevant for run
    std::string output_file = ""; // stdout if empty
                                  // if run -> file to replace stdout
                                  // else      file to write program to
    std::string fmt = "jas";      // only relevant for compile
                                  //   what is the output, options: {jas, jit, x64}
    bool verbose = false;         // whether verbose output is given
    bool debug = false;           // whether debug output is given
};

static std::vector<std::string> args(int argc, char **argv) {
    std::vector<std::string> args;

    for (int i = 1; i < argc; i++)
        args.emplace_back(argv[i]);

    return args;
}

static void print_basic_help(string cmd) {
    std::cerr << "Usage: ij {compile,run} [options] in.ij\n";

    if (!cmd.empty())
        std::cerr << "    invalid command: " << cmd << std::endl;

    exit(1);
}

static void print_compile_help(std::string msg) {
    std::cerr << "Usage: ij compile [options] in.ij\n"
              << "       ij c       [options] in.ij\n"
              << "          compiles the sources to jas/ijvm, options:\n\n"
              << "          -o, --output   - output file (stdout by default)\n"
              << "          -f, --format {jas, ijvm, x64}\n"
              << "                         - which output format, default=jas\n"
              << "          -v, --verbose  - prints verbose info\n"
              << "          -d, --debug    - prints debug info\n\n";

    if (!msg.empty())
        log.panic("Error: %s", msg.c_str());

    exit(-1);
}

static void print_run_help(std::string msg) {
    std::cerr
        << "Usage: ij run [options] in.ij\n"
        << "       ij r   [options] in.ij\n"
        << "    jit compiles the sources to x64 and executes them, options:\n\n"
        << "    -i, --input    - IN reads from file instead of stdin\n"
        << "    -o, --output   - OUT writes to file instead of stdout\n"
        << "    -v, --verbose  - prints verbose info\n"
        << "    -d, --debug    - prints debug info\n";

    if (!msg.empty())
        log.panic("Error: %s", msg.c_str());

    exit(-1);
}

static void parse_compile_options(std::vector<std::string> args, options &o) {
    for (unsigned i = 1; i < args.size(); i++) {
        std::string &arg = args[i];

        if (arg == "-h" || arg == "--help") {
            print_compile_help("");
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 < args.size())
                o.output_file = args[++i];
            else
                print_compile_help("output requires an argument");
        } else if (arg == "-f" || arg == "--format") {
            if (i + 1 >= args.size())
                print_compile_help("format requires jas, ijvm or x64 as arg");
            else if (in(args[i + 1], {"jas", "ijvm", "x64"}))
                o.fmt = args[++i];
            else
                print_compile_help(
                    sprint("argument %s is invalid", args[i + 1]));
        } else if (arg == "-v" || arg == "--verbose") {
            log.set_log_level(LogLevel::success);
        } else if (arg == "-d" || arg == "--debug") {
            log.set_log_level(LogLevel::info);
        } else if (arg[0] == '-') {
            print_compile_help(concat("unknown option ", arg, " is invalid"));
        } else if (o.src_file.empty()) {
            o.src_file = arg;
        } else {
            print_compile_help(
                concat("only one positional argument supported, found ",
                       o.src_file, " and ", arg));
        }
    }

    if (o.src_file.empty())
        print_compile_help(sprint("Missing source file!"));
}

static void parse_run_options(std::vector<std::string> args, options &o) {
    o.fmt = "x64"; // always requires the x64 architecture

    for (unsigned i = 1; i < args.size(); i++) {
        std::string &arg = args[i];

        if (arg == "-h" || arg == "--help") {
            print_run_help("");
        } else if (arg == "-i" || arg == "--input") {
            if (i + 1 < args.size())
                o.input_file = args[++i];
            else
                print_run_help("input requires an argument");
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 < args.size())
                o.output_file = args[++i];
            else
                print_run_help("output requires an argument");
        } else if (arg == "-v" || arg == "--verbose") {
            log.set_log_level(LogLevel::success);
        } else if (arg == "-d" || arg == "--debug") {
            log.set_log_level(LogLevel::info);
        } else if (arg[0] == '-') {
            print_run_help(concat("unknown option ", arg, " is invalid"));
        } else if (o.src_file.empty()) {
            o.src_file = arg;
        } else {
            print_run_help(
                concat("only one positional argument supported, found ",
                       o.src_file, " and ", arg));
        }
    }

    if (o.src_file.empty())
        print_run_help(sprint("Missing source file!"));
}

static void parse_options(std::vector<std::string> args, options &o) {
    if (args.empty()) {
        print_basic_help("No command given");
    } else if (args[0] == "r" || args[0] == "run") {
        log.info("Executing the run command");
        parse_run_options(args, o);
        o.run = true;
        return;
    } else if (args[0] == "c" || args[0] == "compile") {
        log.info("Executing the compile command");
        parse_compile_options(args, o);
        return;
    } else {
        print_basic_help(concat("Didn't recognise command ", args[0]));
    }
}

static void x64_run(options &o, Assembler &a) {
    if (X64Assembler *x64 = dynamic_cast<X64Assembler *>(&a)) {
        if (!o.input_file.empty())
            assert(freopen(o.input_file.c_str(), "r", stdin));

        if (!o.output_file.empty())
            assert(freopen(o.output_file.c_str(), "w+", stdout));

        x64->run();
    } else {
        log.panic("Format might have been wrong");
    }
}

static void compile_to_file(options &o, Assembler &a) {
    if (o.output_file.empty()) {
        log.info("Writing to stdout");
        a.compile(std::cout);
    } 
    else {
        log.info("Writing to file %s", o.output_file.c_str());

        std::ofstream out_file;
        out_file.open(o.output_file, std::ios::binary);
        if (!out_file.is_open())
            log.panic("File %s couldn't be opened for writing",
                        o.output_file.c_str());

        a.compile(out_file);
        out_file.close();
    }
}

int main(int argc, char **argv) {
    options o;
    parse_options(args(argc, argv), o);

    std::unique_ptr<Assembler> a;

    if (o.fmt == "jas")
        a = std::make_unique<JASAssembler>();
    else if (o.fmt == "ijvm")
        a = std::make_unique<IJVMAssembler>();
    else
        a = std::make_unique<X64Assembler>();

    Lexer l;

    try {
        l.add_source(o.src_file);

        if (endswith(o.src_file, ".jas"))
            jas_compile(l, *a);
        else if (endswith(o.src_file, ".ij"))
            ij_compile(l, *a);
        else
            log.panic("Can't parse file with that extension!");

        if (o.run) {
            x64_run(o, *a);
        } else {
            compile_to_file(o, *a);
        }

    } catch (std::runtime_error &r) {
        log.panic("while compiling %s, %s", o.src_file.c_str(), r.what());
    }
    return 0;
}