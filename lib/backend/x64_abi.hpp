/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include "abi_implementation.hpp"

namespace coral66::CGLLVM
{

class x64_abi final : public abi_implementation
{
public:
    void applyPlatformABI(const semantics::ProcedureType& procedureType, llvm::Type*& returnType,
                          std::vector<llvm::Type*>& parameterTypes) override;
};

} // namespace coral66::CGLLVM
