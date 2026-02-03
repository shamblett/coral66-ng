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

#include "ABIImplementation.hpp"
#include "TypeIdentity.hpp"

/**
 * @file CommonABIImplementation.hpp
 * @brief Defines a common base class for ABI implementations with caching.
 *
 * This file provides the CommonABIImplementation template class, which extends ABIImplementation.
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
class CommonABIImplementation : public ABIImplementation
{
    struct Cache
    {
        llvm::Type* returnType;
        std::vector<llvm::Type*> arguments;
    };

    std::unordered_map<Semantics::FunctionType, std::pair<T, Cache>, TypeHasher<Semantics::FunctionType>,
                       TypeEqual<Semantics::FunctionType>>
        m_adjustments;

protected:
    CommonABIImplementation(const llvm::DataLayout& dataLayout) : ABIImplementation(dataLayout) {}

    const T* m_currentFunctionABI = nullptr;
    llvm::AllocaInst* m_returnSlot = nullptr;

    virtual T applyPlatformABIImpl(llvm::Type*& returnType, std::vector<llvm::Type*>& arguments) = 0;

    const T& getAdjustment(const Semantics::FunctionType& functionType) const
    {
        auto result = m_adjustments.find(functionType);
        CLD_ASSERT(result != m_adjustments.end());
        return result->second.first;
    }

public:
    void applyPlatformABI(const Semantics::FunctionType& functionType, llvm::Type*& returnType,
                          std::vector<llvm::Type*>& arguments) final
    {
        auto result = m_adjustments.find(functionType);
        if (result == m_adjustments.end())
        {
            auto adjustments = applyPlatformABIImpl(returnType, arguments);
            m_adjustments.emplace(functionType, std::make_pair(std::move(adjustments), Cache{returnType, arguments}));
            return;
        }
        returnType = result->second.second.returnType;
        arguments = result->second.second.arguments;
    }
};
} // namespace coral66::CGLLVM
