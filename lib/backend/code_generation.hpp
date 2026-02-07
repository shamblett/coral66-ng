/*
* Project: coral66
* Author : Copyright S. Hamblett <steve.hamblett@linux.com>
* Date   : 31/01/2026
*/

#pragma once

#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/Module.h>
#include <llvm/Target/TargetMachine.h>

#include "lib/support/Filesystem.hpp"

/**
 * @file CodeGeneration.hpp
 * @brief Defines the entry point and options for LLVM code generation.
 *
 * This file declares the `generateLLVM` function, which orchestrates the translation
 * of the semantic AST (Program) into LLVM IR. It also defines the `Options` struct
 * to configure the code generation process, including optimization levels and debug info.
 */

namespace coral66
{
namespace Semantics
{
class Program;
} // namespace Semantics
namespace CGLLVM
{
enum class DebugEmission
{
    None = 0,
    Line = 1,
    Default = 2,
    Extended = 3
};

struct Options
{
    bool emitAllDecls{};
    DebugEmission debugEmission = DebugEmission::None;
    std::optional<llvm::Reloc::Model> reloc = std::nullopt;
    llvm::CodeGenOptLevel ol = llvm::CodeGenOpt::parseLevel('0').value();
};

std::unique_ptr<llvm::TargetMachine> generateLLVM(llvm::Module& module, const Semantics::Program& program,
                                                  const Options& options = {});
} // namespace CGLLVM
} // namespace coral66
