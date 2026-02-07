/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "x64_abi.hpp"

#include <algorithm>
#include <numeric>
#include <tuple>
#include <variant>

#include "lib/frontend/compiler/Semantics.hpp"

namespace coral66::CGLLVM
{

void x64_abi::applyPlatformABI(const Semantics::ProcedureType&, llvm::Type*&, std::vector<llvm::Type*>&)
{
    // Stub implementation
}

} // namespace coral66::CGLLVM
