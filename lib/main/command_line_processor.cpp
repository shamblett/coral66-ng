/*
* Project: coral66
* Author : Copyright S. Hamblett <steve.hamblett@linux.com>
* Date   : 10/02/2026
*/

# include "command_line_processor.hpp"

#include "llvm/Support/CommandLine.h"

inline void coral66::CommandLineProcessor::ParseCommandLine(const int argc, char** argv)
{
    llvm::cl::ParseCommandLineOptions(argc, argv);
}
