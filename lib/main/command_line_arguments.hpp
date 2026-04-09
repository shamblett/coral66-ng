/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 10/02/2026
 */

#pragma once

#include "llvm/Support/CommandLine.h"

/*
 * Command line arguments
 *
 * -o <file>           Path of the output file
 * -c                  Stop after compiling object files
 * -s                  Stop after compiling and output assembly files
 * -e                  Preprocess to stdout
 * -emit-llvm          Emit LLVM IR instead of machine code and assembly
 * -O<level>           Optimization level, 1,2 or 3
 * -L<dir>             Library directory path
 * -fPIE               Build a position independent executable
 * -fPIC               Build position independent code
 * --help              Display all options
 * --v                 Print version
 * -g1                 Lines only
 * -g2                 Generated debugging info
 * -g3                 Generated extended debugging info
 * -syntax-only        Stop after semantic analysis and produce no output file
 * -parse-only         Stop after parsing and produce no output file
 * -time               Output time taken for each stage
 * -noact              Check command line only, take no further action
 */

namespace coral66::cl
{
enum OptLevel
{
    None,
    O1,
    O2,
    O3
};

enum DebugLevel
{
    NoDebug,
    g1,
    g2,
    g3
};

inline llvm::cl::opt<std::string> OutputFilename("o", llvm::cl::desc("Output filename"),
                                                 llvm::cl::value_desc("filename"));
inline llvm::cl::opt<bool> CompileOnly("c", llvm::cl::desc("Stop after compiling object files"));
inline llvm::cl::opt<bool> CompileAndAssembly("s", llvm::cl::desc("Stop after compiling and output assembly files"));
inline llvm::cl::opt<bool> PreprocessOutput("e", llvm::cl::desc("Preprocess to stdout or output file"));
inline llvm::cl::opt<bool> EmitLLVM("emit-llvm", llvm::cl::desc("Emit LLVM IR instead of machine code and assembly"));
inline llvm::cl::opt<OptLevel> OptimizationLevel(llvm::cl::desc("Optimization level:"),
                                                 llvm::cl::values(clEnumVal(O1, "Enable trivial optimizations"),
                                                                  clEnumVal(O2, "Enable default optimizations"),
                                                                  clEnumVal(O3, "Enable expensive optimizations")));
inline llvm::cl::opt<std::string> LibraryDirectory("L", llvm::cl::desc("Library directory path"),
                                                   llvm::cl::value_desc("dirpath"));
inline llvm::cl::opt<bool> Pie("pie", llvm::cl::desc("Build a position independent executable"));
inline llvm::cl::opt<bool> Pic("pic", llvm::cl::desc("Build position independent code"));
inline llvm::cl::opt<bool> Version("v", llvm::cl::desc("Version"));
inline llvm::cl::opt<DebugLevel> DebugLevel(llvm::cl::desc("Debug level:"),
                                            llvm::cl::values(clEnumVal(g1, "Lines only"),
                                                             clEnumVal(g2, "Generate debugging info"),
                                                             clEnumVal(g3, "Generated extended debugging info")));
inline llvm::cl::opt<bool> SyntaxOnly("syntax-only",
                                      llvm::cl::desc("Stop after semantic analysis and produce no output file"));
inline llvm::cl::opt<bool> ParseOnly("parse-only", llvm::cl::desc("Stop after parsing and produce no output file"));
inline llvm::cl::opt<bool> Time("time", llvm::cl::desc("Output time taken for each stage"));
inline llvm::cl::opt<bool> NoAction("noact",
                                    llvm::cl::desc("Check command line parameter validity, take no further action"));

// The source file name
inline llvm::cl::opt<std::string> SourceFilename(llvm::cl::Positional, llvm::cl::desc("<input source file>"));

} // namespace coral66::cl
