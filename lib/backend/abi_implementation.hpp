/*
* Project: coral66
* Author : Copyright S. Hamblett <steve.hamblett@linux.com>
* Date   : 31/01/2026
*/

#pragma once

#include <llvm/IR/IRBuilder.h>
#include "lib/frontend/compiler/Semantics.hpp"

namespace coral66::CGLLVM
{

class abi_implementation
{
public:
    virtual ~abi_implementation() = default;

    // Placeholder for platform-specific ABI handling
    // For Coral 66, we might need simple calling conventions initially.
    
    virtual void applyPlatformABI(const Semantics::ProcedureType& procedureType, llvm::Type*& returnType,
                                  std::vector<llvm::Type*>& parameterTypes) = 0;
};

} // namespace coral66::CGLLVM
