/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/Module.h>
#include <llvm/Target/TargetMachine.h>

#include "lib/frontend/syntax/syntax.hpp"
#include "lib/support/filesystem.hpp"

namespace coral66
{
namespace semantics
{
class Program;
} // namespace semantics
namespace cgllvm
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
    bool emitAllDeclarations{};
    DebugEmission debugEmission = DebugEmission::None;
    std::optional<llvm::Reloc::Model> reloc = std::nullopt;
    llvm::CodeGenOptLevel ol = llvm::CodeGenOpt::parseLevel('0').value();
};

std::unique_ptr<llvm::TargetMachine> generateLLVM(llvm::Module& module, const syntax::Program& tree,
                                                  const semantics::Program& program, const Options& options = {});
} // namespace cgllvm
} // namespace coral66
