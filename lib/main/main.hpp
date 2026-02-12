/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <llvm/Support/raw_ostream.h>

#include <span>
#include <string_view>

namespace coral66
{
int main(int argc, char** argv, llvm::raw_ostream* err = &llvm::errs(),
         llvm::raw_ostream* out = &llvm::outs());
} // namespace coral66
