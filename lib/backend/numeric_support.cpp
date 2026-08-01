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

    bool isLeftFixed = false;
    bool isRightFixed = false;

    const auto kind = expression.getOp()->getTokenType();
    const llvm::Value* L = visit(expression.getLeft());
    const llvm::Value* R = visit(expression.getRight());

    if (!L || !R)
        return nullptr;

    if (itL != m_program.getContext().expressionTypes.end())
    {
        if (auto* fixedL = itL->second->tryAs<semantics::FixedType>())
        {
            leftFraction = fixedL->getFractionalBits();
            isLeftFixed = true;
        }
    }

    if (itR != m_program.getContext().expressionTypes.end())
    {
        if (auto* fixedR = itR->second->tryAs<semantics::FixedType>())
        {
            rightFraction = fixedR->getFractionalBits();
            isRightFixed = true;
        }
    }

    if (L->getType()->isIntegerTy() && R->getType()->isIntegerTy() && (isLeftFixed || isRightFixed))
    {
        // Emit instructions using IRBuilder.
        // Note: If L and R are constants, IRBuilder will automatically constant-fold these operations
        // and return an llvm::ConstantInt* without emitting instructions.
        llvm::Value* scaledL = const_cast<llvm::Value*>(L);
        llvm::Value* scaledR = const_cast<llvm::Value*>(R);

        // Align integer bit widths before operations
        if (scaledL->getType()->getIntegerBitWidth() > scaledR->getType()->getIntegerBitWidth())
        {
            scaledR = m_builder.CreateIntCast(scaledR, scaledL->getType(), true);
        }
        else if (scaledL->getType()->getIntegerBitWidth() < scaledR->getType()->getIntegerBitWidth())
        {
            scaledL = m_builder.CreateIntCast(scaledL, scaledR->getType(), true);
        }

        const bool isAddSubCmp =
            (kind == lexer::TokenType::Plus || kind == lexer::TokenType::Minus || kind == lexer::TokenType::Equal
             || kind == lexer::TokenType::NotEqual || kind == lexer::TokenType::LessThan
             || kind == lexer::TokenType::LessThanOrEqual || kind == lexer::TokenType::GreaterThan
             || kind == lexer::TokenType::GreaterThanOrEqual);

        llvm::Value* alignedL = scaledL;
        llvm::Value* alignedR = scaledR;

        // Align fractions only for Add, Sub, and Comparisons
        if (isAddSubCmp)
        {
            if (leftFraction > rightFraction)
            {
                llvm::Value* shiftVal = llvm::ConstantInt::get(alignedL->getType(), leftFraction - rightFraction, true);
                alignedR = m_builder.CreateShl(alignedR, shiftVal, "align_right_fraction");
            }
            else if (rightFraction > leftFraction)
            {
                llvm::Value* shiftVal = llvm::ConstantInt::get(alignedR->getType(), rightFraction - leftFraction, true);
                alignedL = m_builder.CreateShl(alignedL, shiftVal, "align_left_fraction");
            }
        }

        if (kind == lexer::TokenType::Plus)
        {
            return m_builder.CreateAdd(alignedL, alignedR, "fixed_add");
        }

        if (kind == lexer::TokenType::Minus)
        {
            return m_builder.CreateSub(alignedL, alignedR, "fixed_sub");
        }

        if (kind == lexer::TokenType::Asterisk)
        {
            // Multiply raw bits, then shift right by rightFraction to retain leftFraction format.
            llvm::Value* mult = m_builder.CreateMul(scaledL, scaledR, "fixed_mul_raw");
            llvm::Value* shiftVal = llvm::ConstantInt::get(scaledL->getType(), rightFraction, true);
            return m_builder.CreateAShr(mult, shiftVal, "fixed_mul_scale");
        }

        if (kind == lexer::TokenType::Division)
        {
            // Shift LHS left by rightFraction before dividing by raw RHS to retain leftFraction format.
            llvm::Value* shiftVal = llvm::ConstantInt::get(scaledL->getType(), rightFraction, true);
            llvm::Value* shiftedL = m_builder.CreateShl(scaledL, shiftVal, "fixed_div_scale_up");
            return m_builder.CreateSDiv(shiftedL, scaledR, "fixed_div");
        }

        if (kind == lexer::TokenType::Equal)
        {
            return m_builder.CreateICmpEQ(alignedL, alignedR, "fixed_eq");
        }

        if (kind == lexer::TokenType::NotEqual)
        {
            return m_builder.CreateICmpNE(alignedL, alignedR, "fixed_ne");
        }

        if (kind == lexer::TokenType::LessThan)
        {
            return m_builder.CreateICmpSLT(alignedL, alignedR, "fixed_lt");
        }

        if (kind == lexer::TokenType::LessThanOrEqual)
        {
            return m_builder.CreateICmpSLE(alignedL, alignedR, "fixed_le");
        }

        if (kind == lexer::TokenType::GreaterThan)
        {
            return m_builder.CreateICmpSGT(alignedL, alignedR, "fixed_gt");
        }

        if (kind == lexer::TokenType::GreaterThanOrEqual)
        {
            return m_builder.CreateICmpSGE(alignedL, alignedR, "fixed_ge");
        }
    }

    return nullptr;
}

} // namespace coral66::cgllvm