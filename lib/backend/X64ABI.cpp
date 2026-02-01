/*
* Project: coral66
* Author : Copyright S. Hamblett <steve.hamblett@linux.com>
* Date   : 31/01/2026
*/

#pragma once

#include "frontend/compiler/Semantics.hpp"

#include <algorithm>
#include <numeric>
#include <tuple>
#include <variant>

#include "X64ABI.hpp"

namespace coral66::CGLLVM
{

void X64ABI::applyPlatformABI(const Semantics::ProcedureType&, llvm::Type*&, std::vector<llvm::Type*>&)
{
    // Stub implementation
}

} // namespace coral66::CGLLVM
