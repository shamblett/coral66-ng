/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/07/2026
 */

#include "code_generator.hpp"
#include "lib/frontend/common/lexer_common.hpp"
#include "lib/frontend/common/semantics_common.hpp"
#include "lib/frontend/semantics/program.hpp"
#include "lib/frontend/syntax/syntax.hpp"
#include "lib/support/vendor/fixed/dynamic_fixed.hpp"

namespace coral66::cgllvm
{
// Process binary expressions where either or both sides are fixed
llvm::Value* CodeGenerator::processFixedBinaryExpression(const syntax::BinaryExpression& expression)
{
    const auto itL = m_program.getContext().expressionTypes.find(&expression.getLeft());
    const auto itR = m_program.getContext().expressionTypes.find(&expression.getRight());

    std::uint32_t leftFraction = 0;
    std::uint32_t rightFraction = 0;
    std::uint32_t leftTotal = 0;
    std::uint32_t rightTotal = 0;

    bool isLeftFixed = false;
    bool isRightFixed = false;
    bool sameFormat = false;

    const auto kind = expression.getOp()->getTokenType();
    const llvm::Value* L = visit(expression.getLeft());

    if (itL != m_program.getContext().expressionTypes.end())
    {
        if (auto* fixedL = itL->second->tryAs<semantics::FixedType>())
        {
            leftFraction = fixedL->getFractionalBits();
            leftTotal = fixedL->getTotalBits();
            isLeftFixed = true;
        }
    }

    if (itR != m_program.getContext().expressionTypes.end())
    {
        if (auto* fixedR = itR->second->tryAs<semantics::FixedType>())
        {
            rightFraction = fixedR->getFractionalBits();
            rightTotal = fixedR->getTotalBits();
            isRightFixed = true;
        }
    }

    if (rightFraction == leftFraction && rightTotal == leftTotal)
    {
        sameFormat = true;
    }

    if (L->getType()->isIntegerTy() && (isLeftFixed || isRightFixed))
    {
        const auto rhs = numeric::fixed::DynamicFixed(itR->second->Value, rightTotal, rightFraction);
        const auto lhs = numeric::fixed::DynamicFixed(itL->second->Value, leftTotal, leftFraction);

        // Both fixed
        if (isLeftFixed && isRightFixed)
        {
            if (kind == lexer::TokenType::Plus)
            {
                const auto val = (rhs + lhs).to_double();
                llvm::LLVMContext& context = L->getContext();
                llvm::Type* fpType = llvm::Type::getFloatTy(context);
                llvm::Constant* fpConstant = llvm::ConstantFP::get(fpType, val);
                return fpConstant;
            }

            if (kind == lexer::TokenType::Minus)
            {
                const auto val = (rhs - lhs).to_double();
                llvm::LLVMContext& context = L->getContext();
                llvm::Type* fpType = llvm::Type::getFloatTy(context);
                llvm::Constant* fpConstant = llvm::ConstantFP::get(fpType, val);
                return fpConstant;
            }

            if (kind == lexer::TokenType::Asterisk)
            {
                const auto val = (rhs * lhs).to_double();
                llvm::LLVMContext& context = L->getContext();
                llvm::Type* fpType = llvm::Type::getFloatTy(context);
                llvm::Constant* fpConstant = llvm::ConstantFP::get(fpType, val);
                return fpConstant;
            }

            if (kind == lexer::TokenType::Division)
            {
                const auto val = (rhs / lhs).to_double();
                llvm::LLVMContext& context = L->getContext();
                llvm::Type* fpType = llvm::Type::getFloatTy(context);
                llvm::Constant* fpConstant = llvm::ConstantFP::get(fpType, val);
                return fpConstant;
            }

            if (kind == lexer::TokenType::Equal)
            {
                if (sameFormat)
                {
                    if (rhs.raw_value == lhs.raw_value)
                    {
                        return llvm::ConstantInt::getTrue(L->getContext());
                    }
                }
                else
                {
                }
                return llvm::ConstantInt::getFalse(L->getContext());
            }
        }

        // Left fixed
        if (isLeftFixed)
        {
        }

        // Right fixed
        if (isRightFixed)
        {
        }
    }

    return nullptr;
}

} // namespace coral66::cgllvm