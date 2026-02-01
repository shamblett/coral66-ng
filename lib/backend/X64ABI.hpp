/*
* Project: coral66
* Author : Copyright S. Hamblett <steve.hamblett@linux.com>
* Date   : 31/01/2026
*/

#pragma once

#include "ABIImplementation.hpp"

namespace coral66::CGLLVM
{

class X64ABI final : public ABIImplementation
{
public:
    void applyPlatformABI(const Semantics::ProcedureType& procedureType, llvm::Type*& returnType,
                          std::vector<llvm::Type*>& parameterTypes) override;
};

} // namespace coral66::CGLLVM
