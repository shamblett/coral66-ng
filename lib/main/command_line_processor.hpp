/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 10/02/2026
 */

#pragma once

#include <llvm/Support/raw_ostream.h>

/**
 * The command line processor
 *
 * Implemented using the LLVM CommandLine library
 */

namespace coral66
{

class CommandLineProcessor
{
public:
    static void PrintVersion(llvm::raw_ostream& OS);

    static void ParseCommandLine(int argc, char** argv);
};

} // namespace coral66