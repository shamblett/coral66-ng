/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 10/02/2026
 */

#include "command_line_processor.hpp"

#include "command_line_arguments.hpp"
#include "version.hpp"

namespace coral66
{

void CommandLineProcessor::PrintVersion(llvm::raw_ostream& OS)
{
    OS << version << "\n";
}

void CommandLineProcessor::ParseCommandLine(const int argc, char** argv)
{
    // Reset all options to their default values before parsing
    // As all these variables are global any users that run this multiple times in
    // a single process such as the unit test suite need this.
    llvm::cl::ResetAllOptionOccurrences();

    cl::OutputFilename = "";
    cl::CompileOnly = false;
    cl::CompileAndAssembly = false;
    cl::PreprocessOutput = false;
    cl::EmitLLVM = false;
    cl::OptimizationLevel = cl::OptLevel::None;
    cl::LibraryDirectory = "";
    cl::Pie = false;
    cl::Pic = false;
    cl::Version = false;
    cl::DebugLevel = cl::DebugLevel::NoDebug;
    cl::SyntaxOnly = false;
    cl::ParseOnly = false;
    cl::Time = false;
    cl::NoAction = false;
    cl::SourceFilename = "";

    llvm::cl::SetVersionPrinter([](llvm::raw_ostream& OS) { PrintVersion(OS); });
    llvm::cl::ParseCommandLineOptions(argc, argv);
}

} // namespace coral66
