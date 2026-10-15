/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 02/05/2026
 */

#include "control_flow_pass.hpp"
#include "lib/frontend/common/messages/errors.hpp"

namespace coral66::semantics
{

std::optional<std::int64_t> evaluateConstantInteger(const syntax::Expression& expr)
{
    if (auto* lit = dynamic_cast<const syntax::LiteralExpression*>(&expr))
    {
        if (lit->getLiteral()->getType() == lexer::CToken::Integer)
        {
            if (auto* intVal = std::get_if<llvm::APSInt>(&lit->getLiteral()->getValue()))
            {
                return intVal->getExtValue();
            }
        }
    }
    else if (auto* unary = dynamic_cast<const syntax::UnaryExpression*>(&expr))
    {
        if (const auto opType = unary->getOp()->getTokenType();
            opType == lexer::TokenType::Plus || opType == lexer::TokenType::Minus)
        {
            if (auto val = evaluateConstantInteger(unary->getOperand()))
            {
                if (opType == lexer::TokenType::Minus)
                    return -*val;
                return *val;
            }
        }
    }
    else if (auto* binary = dynamic_cast<const syntax::BinaryExpression*>(&expr))
    {
        const auto left = evaluateConstantInteger(binary->getLeft());
        if (const auto right = evaluateConstantInteger(binary->getRight()); left && right)
        {
            const auto opType = binary->getOp()->getTokenType();
            if (opType == lexer::TokenType::Plus)
                return *left + *right;
            if (opType == lexer::TokenType::Minus)
                return *left - *right;
            if (opType == lexer::TokenType::Asterisk)
                return *left * *right;
            if (opType == lexer::TokenType::Division)
            {
                if (*right != 0)
                    return *left / *right;
            }
        }
    }
    return std::nullopt;
}

} // namespace coral66::semantics