/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 28/08/2026
 */

#include "command_line_arguments.hpp"

namespace coral66::cl
{

inline llvm::cl::opt<std::string> OutputFilename("o", llvm::cl::desc("Output filename"),
                                                 llvm::cl::value_desc("filename"));
inline llvm::cl::opt<bool> CompileOnly("c", llvm::cl::desc("Stop after compiling object files"));
inline llvm::cl::opt<bool> CompileAndAssembly("s", llvm::cl::desc("Stop after compiling and output assembly files"));
inline llvm::cl::opt<bool> PreprocessOutput("e", llvm::cl::desc("Preprocess to stdout or output file"));
inline llvm::cl::opt<bool> EmitLLVM("emit-llvm", llvm::cl::desc("Emit LLVM IR instead of machine code and assembly"));
inline llvm::cl::opt<TargetEnvironment>
    TargetEnv("target-env", llvm::cl::desc("Target environment:"), llvm::cl::init(TargetEnvironment::OS),
              llvm::cl::values(clEnumValN(TargetEnvironment::OS, "os", "Operating System (default)"),
                               clEnumValN(TargetEnvironment::Baremetal, "baremetal",
                                          "Bare-metal (supports absolute addressing)")));
inline llvm::cl::opt<OptLevel> OptimizationLevel("O", llvm::cl::Prefix, llvm::cl::desc("Optimization level:"),
                                                 llvm::cl::values(clEnumVal(1, "Enable trivial optimizations"),
                                                                  clEnumVal(2, "Enable default optimizations"),
                                                                  clEnumVal(3, "Enable expensive optimizations")));
inline llvm::cl::opt<std::string> LibraryDirectory("L", llvm::cl::desc("Library directory path"),
                                                   llvm::cl::value_desc("dirpath"));
inline llvm::cl::opt<std::string> ExternalDirectory("X", llvm::cl::desc("External code directory path"),
                                                    llvm::cl::value_desc("dirpath"));
inline llvm::cl::opt<bool> Pie("pie", llvm::cl::desc("Build a position independent executable"));
inline llvm::cl::opt<bool> Pic("pic", llvm::cl::desc("Build position independent code"));
inline llvm::cl::opt<DebugLevels> DebugLevel("g", llvm::cl::Prefix, llvm::cl::desc("Debug level:"),
                                             llvm::cl::values(clEnumVal(1, "Lines only"),
                                                              clEnumVal(2, "Generate debugging info"),
                                                              clEnumVal(3, "Generated extended debugging info")));
inline llvm::cl::opt<bool> SyntaxOnly("syntax-only",
                                      llvm::cl::desc("Stop after semantic analysis and produce no output file"));
inline llvm::cl::opt<bool> ParseOnly("parse-only", llvm::cl::desc("Stop after parsing and produce no output file"));
inline llvm::cl::opt<bool> Time("time", llvm::cl::desc("Output time taken for each stage"));
inline llvm::cl::opt<bool> NoAction("noact",
                                    llvm::cl::desc("Check command line parameter validity, take no further action"));

// The source file name and object files
inline llvm::cl::opt<std::string> SourceFilename(llvm::cl::Positional, llvm::cl::desc("<input source file>"));
inline llvm::cl::list<std::string> ObjectFiles(llvm::cl::Positional, llvm::cl::desc("[object files...]"),
                                               llvm::cl::ZeroOrMore);

} // namespace coral66::cl