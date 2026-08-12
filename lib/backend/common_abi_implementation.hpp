/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <unordered_map>

#include "abi_implementation.hpp"
#include "type_identity.hpp"

/**
 *  CommonABIImplementation.hpp
 *  Defines a common base class for ABI implementations with caching.
 *
 * This file provides the CommonAbiImplementation template class, which extends AbiImplementation.
 * It implements a caching mechanism for platform-specific ABI adjustments to function
 * signatures, avoiding redundant calculations for repeatedly encountered function types.
 */

namespace llvm
{
class AllocaInst;
} // namespace llvm

namespace coral66::cgllvm
{
template <class T>
class CommonAbiImplementation : public AbiImplementation
{
    struct Cache
    {
        llvm::Type* returnType;
        std::vector<llvm::Type*> arguments;
    };

    std::unordered_map<semantics::FunctionType, std::pair<T, Cache>, TypeHasher<semantics::FunctionType>,
                       TypeEqual<semantics::FunctionType>>
        m_adjustments;

protected:
    CommonAbiImplementation(const llvm::DataLayout& dataLayout) : AbiImplementation(dataLayout) {}

    const T* m_currentFunctionABI = nullptr;
    llvm::AllocaInst* m_returnSlot = nullptr;

    virtual T applyPlatformABIImpl(llvm::Type*& returnType, std::vector<llvm::Type*>& arguments) = 0;

    const T& getAdjustment(const semantics::FunctionType& functionType) const
    {
        auto result = m_adjustments.find(functionType);
        CORAL66_ASSERT(result != m_adjustments.end());
        return result->second.first;
    }

public:
    void applyPlatformABI(const semantics::FunctionType& functionType, llvm::Type*& returnType,
                          std::vector<llvm::Type*>& arguments) final
    {
        auto result = m_adjustments.find(functionType);
        if (result == m_adjustments.end())
        {
            auto adjustments = applyPlatformABIImpl(returnType, arguments);
            adjustments.emplace(functionType, std::make_pair(std::move(adjustments), Cache{returnType, arguments}));
            return;
        }
        returnType = result->second.second.returnType;
        arguments = result->second.second.arguments;
    }
};
} // namespace coral66::cgllvm
