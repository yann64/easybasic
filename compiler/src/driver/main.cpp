#include <array>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "../ast/ast.hpp"
#include "../codegen/codegen.hpp"
#include "../diagnostics/diagnostics.hpp"
#include "../lexer/lexer.hpp"
#include "../parser/parser.hpp"
#include "../preprocessor/macro_expander.hpp"
#include "../sema/sema.hpp"
#include "easybasic/version.hpp"
#include "process.hpp"

namespace {

struct Options {
    std::string inputPath;
    std::string outputPath;
    std::string cxxCompiler = "g++";
    bool debugMode = false;
    bool showVersion = false;
};

void printUsage() {
    std::cerr << "Usage: pbcxx <input.pb> -o <output> [-d] [-cxx <compiler>]\n"
                 "  -o <output>    Output executable path (required)\n"
                 "  -d             Debug mode: Debug statements print, exactly like\n"
                 "                 pbcompilerc's own -d/--debugger flag\n"
                 "  -cxx <compiler> Backend C++ compiler to invoke (default: g++)\n"
                 "  -v, --version  Print pbcxx's version and exit\n";
}

bool parseArgs(int argc, char** argv, Options& opts) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-o" && i + 1 < argc) {
            opts.outputPath = argv[++i];
        } else if (arg == "-d") {
            opts.debugMode = true;
        } else if (arg == "-cxx" && i + 1 < argc) {
            opts.cxxCompiler = argv[++i];
        } else if (arg == "-v" || arg == "--version") {
            opts.showVersion = true;
        } else if (!arg.empty() && arg[0] == '-') {
            std::cerr << "pbcxx: unknown option '" << arg << "'\n";
            return false;
        } else {
            opts.inputPath = arg;
        }
    }
    return true;
}

std::string readFile(const std::string& path, bool& ok) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        ok = false;
        return {};
    }
    std::ostringstream contents;
    contents << in.rdbuf();
    ok = true;
    return contents.str();
}

std::string runtimeIncludeDir() {
    // Build-tree fallback only for now; see version.hpp.in's own comment.
    return EASYBASIC_BUILD_TREE_RUNTIME_INCLUDE_DIR;
}

/// Runs `pkg-config <mode> <package>` (`mode` is `--cflags` or `--libs` -
/// kept as two separate calls, not one combined `--cflags --libs`, because
/// the resulting flags need to land in *different* positions on the
/// backend-compiler command line: compile flags before the source file,
/// link flags after it - GNU ld resolves library symbols against object
/// files already seen, so `-lgtk-3` before the `.cpp` that uses it is a
/// silent-until-link-time "undefined reference" failure, confirmed by
/// trying the combined form first) and splits its one-line stdout on
/// whitespace into separate argv-style tokens (pkg-config's own output
/// never contains spaces within a single flag, so this simple split is
/// safe). Returns `std::nullopt` when `pkg-config` itself isn't found or
/// the package isn't known - the caller is expected to treat that as a
/// hard error with its own clear message, not silently proceed to a
/// confusing backend-compiler failure. POSIX-only (`popen`) - fine for
/// this project's current Linux/Haiku-first GUI support; Windows/MinGW GUI
/// linking isn't addressed by this helper yet (see M7b's own roadmap
/// notes).
std::optional<std::vector<std::string>> pkgConfigFlags(const std::string& mode, const std::string& package) {
    std::string command = "pkg-config " + mode + " " + package + " 2>/dev/null";
    FILE* pipe = popen(command.c_str(), "r");
    if (pipe == nullptr) {
        return std::nullopt;
    }
    std::string output;
    std::array<char, 256> buffer{};
    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output += buffer.data();
    }
    int exitCode = pclose(pipe);
    if (exitCode != 0) {
        return std::nullopt;
    }
    std::vector<std::string> flags;
    std::istringstream stream(output);
    std::string token;
    while (stream >> token) {
        flags.push_back(token);
    }
    return flags;
}

} // namespace

int main(int argc, char** argv) {
    Options opts;
    if (!parseArgs(argc, argv, opts)) {
        printUsage();
        return 1;
    }
    if (opts.showVersion) {
        std::cout << "pbcxx " << EASYBASIC_VERSION;
        if (!std::string(EASYBASIC_GIT_HASH).empty()) {
            std::cout << " (" << EASYBASIC_GIT_HASH << ")";
        }
        std::cout << '\n';
        return 0;
    }
    if (opts.inputPath.empty() || opts.outputPath.empty()) {
        printUsage();
        return 1;
    }

    bool readOk = false;
    std::string source = readFile(opts.inputPath, readOk);
    if (!readOk) {
        std::cerr << "pbcxx: cannot read '" << opts.inputPath << "'\n";
        return 1;
    }

    easybasic::DiagnosticEngine diagnostics;
    int fileId = diagnostics.registerFile(opts.inputPath);

    easybasic::Lexer lexer(source, fileId, diagnostics);
    std::vector<easybasic::Token> tokens = lexer.tokenize();
    if (diagnostics.hasErrors()) {
        diagnostics.printAll(std::cerr);
        return 1;
    }

    easybasic::MacroExpander macroExpander(diagnostics);
    tokens = macroExpander.expand(tokens);
    if (diagnostics.hasErrors()) {
        diagnostics.printAll(std::cerr);
        return 1;
    }

    easybasic::Parser parser(std::move(tokens), diagnostics);
    std::unique_ptr<easybasic::ast::Module> module = parser.parseModule();
    if (diagnostics.hasErrors()) {
        diagnostics.printAll(std::cerr);
        return 1;
    }

    easybasic::Sema sema(diagnostics);
    if (!sema.analyze(*module) || diagnostics.hasErrors()) {
        diagnostics.printAll(std::cerr);
        return 1;
    }

    easybasic::Codegen codegen(*module, sema, opts.debugMode);
    std::string cppSource = codegen.generate();

    std::string cppPath = opts.outputPath + ".generated.cpp";
    {
        std::ofstream out(cppPath, std::ios::binary);
        out << cppSource;
    }

    std::vector<std::string> cxxArgs = {
        "-std=c++20",
        "-pthread", // needed for std::thread/std::mutex/std::counting_semaphore (M7a) on GCC/Clang toolchains
        "-I" + runtimeIncludeDir(),
    };
    std::optional<std::vector<std::string>> gtkLibFlags;
    if (sema.usesGuiLibrary()) {
        // Only added for a program that actually calls a GUI builtin (see
        // Sema::usesGuiLibrary()'s own doc comment) - every other program's
        // build stays completely unaffected, with no GTK3 toolchain
        // requirement at all. Compile flags go in now (before the source
        // file); the matching link flags are appended after it below.
        auto gtkCflags = pkgConfigFlags("--cflags", "gtk+-3.0");
        gtkLibFlags = pkgConfigFlags("--libs", "gtk+-3.0");
        if (!gtkCflags || !gtkLibFlags) {
            std::cerr << "pbcxx: this program uses GUI commands, but 'pkg-config gtk+-3.0' failed - install "
                         "GTK3's development package (e.g. libgtk-3-dev on Debian/Ubuntu, gtk3_devel on Haiku) "
                         "and make sure pkg-config can find it\n";
            return 1;
        }
        cxxArgs.insert(cxxArgs.end(), gtkCflags->begin(), gtkCflags->end());
    }
    cxxArgs.push_back(cppPath);
    if (gtkLibFlags) {
        cxxArgs.insert(cxxArgs.end(), gtkLibFlags->begin(), gtkLibFlags->end());
    }
    cxxArgs.push_back("-o");
    cxxArgs.push_back(opts.outputPath);
    easybasic::ProcessResult result = easybasic::runProcess(opts.cxxCompiler, cxxArgs);
    if (!result.launched) {
        std::cerr << "pbcxx: could not launch backend compiler '" << opts.cxxCompiler << "'\n";
        return 1;
    }
    return result.exitCode;
}
