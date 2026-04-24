/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include <unordered_map>

#include "abi_implementation.hpp"
#include "type_identity.hpp"

/**
 *  CommonABIImplementation.hpp
 *  Defines a common base class for ABI implementations with caching.
 *
 * This file provides the common_abi_implementation template class, which extends abi_implementation.
 * It implements a caching mechanism for platform-specific ABI adjustments to function
 * signatures, avoiding redundant calculations for repeatedly encountered function types.
 */

namespace llvm
{
class AllocaInst;
} // namespace llvm

namespace coral66::CGLLVM
{
template <class T>
class common_abi_implementation : public abi_implementation
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
    common_abi_implementation(const llvm::DataLayout& dataLayout) : abi_implementation(dataLayout) {}

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
} // namespace coral66::CGLLVM
