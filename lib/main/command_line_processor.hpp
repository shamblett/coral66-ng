/*
* Project: coral66
* Author : Copyright S. Hamblett <steve.hamblett@linux.com>
* Date   : 10/02/2026
*/

#pragma once

#include "llvm/Support/CommandLine.h"

#include "command_line_arguments.hpp"

/**
 * @brief The command line processor
 *
 * Implemented using the LLVM CommandLine library
 */

namespace coral66 {

class CommandLineProcessor {

  CommandLineProcessor();

  void ParseCommandLine(int argc, char **argv);

};

}