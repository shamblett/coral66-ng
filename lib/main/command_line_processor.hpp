/*
* Project: coral66
* Author : Copyright S. Hamblett <steve.hamblett@linux.com>
* Date   : 10/02/2026
*/

#pragma once

/**
 * @brief The command line processor
 *
 * Implemented using the LLVM CommandLine library
 */

namespace coral66 {

class CommandLineProcessor {

public:

  static void ParseCommandLine(int argc, char **argv);

};


} // namespace coral66