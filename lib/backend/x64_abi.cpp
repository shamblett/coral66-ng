/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "x64_abi.hpp"

#include "lib/frontend/semantics/semantics.hpp"

namespace coral66::cgllvm
{

void x64_abi::applyPlatformABI(const semantics::ProcedureType&, llvm::Type*&, std::vector<llvm::Type*>&)
{
    // Stub implementation
}

} // namespace coral66::cgllvm
