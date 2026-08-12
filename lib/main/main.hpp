/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <llvm/Support/raw_ostream.h>

/**
 * The main processing functionality.
 *
 * Parses the command line and sanity checks the options before invoking
 * the relevant processing.
 */

namespace coral66
{

// Main compiler entry point
int main(int argc, char** argv, llvm::raw_ostream* reporter = &llvm::errs());

} // namespace coral66
