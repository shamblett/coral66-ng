/*
 * Project: coral66-ng
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
 * -time               Output time taken for each stage produced by CTrack
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

enum DebugLevels
{
    NoDebug,
    g1,
    g2,
    g3
};

enum class TargetEnvironment
{
    OS,
    Baremetal
};

extern llvm::cl::opt<std::string> OutputFilename;
extern llvm::cl::opt<bool> CompileOnly;
extern llvm::cl::opt<bool> CompileAndAssembly;
extern llvm::cl::opt<bool> PreprocessOutput;
extern llvm::cl::opt<bool> EmitLLVM;
extern llvm::cl::opt<TargetEnvironment> TargetEnv;
extern llvm::cl::opt<OptLevel> OptimizationLevel;
extern llvm::cl::opt<std::string> LibraryDirectory;
extern llvm::cl::opt<std::string> ExternalDirectory;
extern llvm::cl::opt<bool> Pie;
extern llvm::cl::opt<bool> Pic;
extern llvm::cl::opt<DebugLevels> DebugLevel;
extern llvm::cl::opt<bool> SyntaxOnly;
extern llvm::cl::opt<bool> ParseOnly;
extern llvm::cl::opt<bool> Time;
extern llvm::cl::opt<bool> NoAction;

// The source file name and object files
extern llvm::cl::opt<std::string> SourceFilename;
extern llvm::cl::list<std::string> ObjectFiles;

} // namespace coral66::cl
