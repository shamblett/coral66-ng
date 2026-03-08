/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "parser.hpp"

#include "lib/frontend/common/messages/errors.hpp"

using namespace coral66::Syntax;

namespace coral66::parser
{

// Forward declarations
std::unique_ptr<Expression> parseConditionalExpression(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                       Context& context);
std::unique_ptr<Expression> parseUnconditionalExpression(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                         Context& context);
std::unique_ptr<Expression> parseCondition(lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context);
std::unique_ptr<Expression> parseSubcondition(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                              Context& context);
std::unique_ptr<Expression> parseComparison(lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context);
std::unique_ptr<Expression> parseSimpleExpression(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                  Context& context);
std::unique_ptr<Expression> parseTerm(lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context);
std::unique_ptr<Expression> parseFactor(lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context);
std::unique_ptr<Expression> parseBitwiseDiffer(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                               Context& context);
std::unique_ptr<Expression> parseBitwiseUnion(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                              Context& context);
std::unique_ptr<Expression> parseBitwiseMask(lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context);
std::unique_ptr<Expression> parseTypedPrimary(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                              Context& context);
std::unique_ptr<Expression> parseWordReference(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                               Context& context);

bool isComparator(lexer::TokenType t)
{
    return t == lexer::TokenType::LessThan || t == lexer::TokenType::LessThanOrEqual || t == lexer::TokenType::Equal
           || t == lexer::TokenType::GreaterThanOrEqual || t == lexer::TokenType::GreaterThan
           || t == lexer::TokenType::NotEqual;
}

bool isAddOperator(lexer::TokenType t)
{
    return t == lexer::TokenType::Plus || t == lexer::TokenType::Minus;
}

bool isMultOperator(lexer::TokenType t)
{
    return t == lexer::TokenType::Asterisk || t == lexer::TokenType::Division;
}

bool isNumberType(lexer::TokenType t)
{
    return t == lexer::TokenType::IntegerKeyword || t == lexer::TokenType::FloatingKeyword
           || t == lexer::TokenType::FixedKeyword;
}

std::unique_ptr<Expression> parseExpression(lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context)
{
    if (begin == end)
    {
        return nullptr;
    }

    if (begin->getTokenType() == lexer::TokenType::IfKeyword)
    {
        return parseConditionalExpression(begin, end, context);
    }
    return parseUnconditionalExpression(begin, end, context);
}

std::unique_ptr<Expression> parseConditionalExpression(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                       Context& context)
{
    auto startToken = begin++; // IF
    auto condition = parseCondition(begin, end, context);
    if (!condition)
        return nullptr;

    if (begin == end || begin->getTokenType() != lexer::TokenType::ThenKeyword)
    {
        return nullptr; // Missing THEN
    }
    begin++; // THEN
    auto thenExpr = parseExpression(begin, end, context);
    if (!thenExpr)
        return nullptr;

    if (begin == end || begin->getTokenType() != lexer::TokenType::ElseKeyword)
    {
        return nullptr; // Missing ELSE
    }
    begin++; // ELSE
    auto elseExpr = parseExpression(begin, end, context);
    if (!elseExpr)
        return nullptr;

    return std::make_unique<ConditionalExpression>(startToken, elseExpr->end(), std::move(condition),
                                                   std::move(thenExpr), std::move(elseExpr));
}

std::unique_ptr<Expression> parseUnconditionalExpression(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                         Context& context)
{
    if (begin->getTokenType() == lexer::TokenType::StringLiteral)
    {
        auto t = begin++;
        return std::make_unique<LiteralExpression>(t, begin, t);
    }
    return parseSimpleExpression(begin, end, context);
}

std::unique_ptr<Expression> parseCondition(lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context)
{
    auto expr = parseSubcondition(begin, end, context);
    if (!expr)
        return nullptr;
    while (begin != end && begin->getTokenType() == lexer::TokenType::OrKeyword)
    {
        auto op = begin++;
        auto right = parseSubcondition(begin, end, context);
        if (!right)
            return nullptr;
        expr = std::make_unique<BinaryExpression>(expr->begin(), right->end(), std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> parseSubcondition(lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context)
{
    auto expr = parseComparison(begin, end, context);
    if (!expr)
        return nullptr;
    while (begin != end && begin->getTokenType() == lexer::TokenType::AndKeyword)
    {
        auto op = begin++;
        auto right = parseComparison(begin, end, context);
        if (!right)
            return nullptr;
        expr = std::make_unique<BinaryExpression>(expr->begin(), right->end(), std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> parseComparison(lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context)
{
    auto expr = parseSimpleExpression(begin, end, context);
    if (!expr)
        return nullptr;
    if (begin != end && isComparator(begin->getTokenType()))
    {
        auto op = begin++;
        auto right = parseSimpleExpression(begin, end, context);
        if (!right)
            return nullptr;
        expr = std::make_unique<BinaryExpression>(expr->begin(), right->end(), std::move(expr), op, std::move(right));
    }
    else
    {
        // Coral 66 requires a comparator for a comparison
        return nullptr;
    }
    return expr;
}

std::unique_ptr<Expression> parseSimpleExpression(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                  Context& context)
{
    std::unique_ptr<Expression> expr;
    if (begin != end && isAddOperator(begin->getTokenType()))
    {
        auto op = begin++;
        auto right = parseTerm(begin, end, context);
        if (!right)
            return nullptr;
        expr = std::make_unique<UnaryExpression>(op, right->end(), op, std::move(right));
    }
    else
    {
        expr = parseTerm(begin, end, context);
    }
    if (!expr)
        return nullptr;
    while (begin != end && isAddOperator(begin->getTokenType()))
    {
        auto op = begin++;
        auto right = parseTerm(begin, end, context);
        if (!right)
            return nullptr;
        expr = std::make_unique<BinaryExpression>(expr->begin(), right->end(), std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> parseTerm(lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context)
{
    auto expr = parseFactor(begin, end, context);
    if (!expr)
        return nullptr;
    while (begin != end && isMultOperator(begin->getTokenType()))
    {
        auto op = begin++;
        auto right = parseFactor(begin, end, context);
        if (!right)
            return nullptr;
        expr = std::make_unique<BinaryExpression>(expr->begin(), right->end(), std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> parseFactor(lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context)
{
    if (begin == end)
        return nullptr;

    // Untypedprimary
    if (begin->getTokenType() == lexer::TokenType::OpenParentheses)
    {
        begin++;
        auto expr = parseExpression(begin, end, context);
        if (!expr)
            return nullptr;
        if (begin != end && begin->getTokenType() == lexer::TokenType::CloseParentheses)
        {
            begin++;
        }
        else
        {
            return nullptr; // Missing )
        }
        return expr;
    }
    if (begin->getTokenType() == lexer::TokenType::Literal && begin->getType() != lexer::CToken::Type::Integer)
    {
        auto t = begin++;
        return std::make_unique<LiteralExpression>(t, begin, t);
    }

    return parseBitwiseDiffer(begin, end, context);
}

std::unique_ptr<Expression> parseBitwiseDiffer(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                               Context& context)
{
    auto expr = parseBitwiseUnion(begin, end, context);
    if (!expr)
        return nullptr;
    while (begin != end && begin->getTokenType() == lexer::TokenType::DifferKeyword)
    {
        auto op = begin++;
        auto right = parseBitwiseUnion(begin, end, context);
        if (!right)
            return nullptr;
        expr = std::make_unique<BinaryExpression>(expr->begin(), right->end(), std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> parseBitwiseUnion(lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context)
{
    auto expr = parseBitwiseMask(begin, end, context);
    if (!expr)
        return nullptr;
    while (begin != end && begin->getTokenType() == lexer::TokenType::UnionKeyword)
    {
        auto op = begin++;
        auto right = parseBitwiseMask(begin, end, context);
        if (!right)
            return nullptr;
        expr = std::make_unique<BinaryExpression>(expr->begin(), right->end(), std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> parseBitwiseMask(lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context)
{
    auto expr = parseTypedPrimary(begin, end, context);
    if (!expr)
        return nullptr;
    while (begin != end && begin->getTokenType() == lexer::TokenType::MaskKeyword)
    {
        auto op = begin++;
        auto right = parseTypedPrimary(begin, end, context);
        if (!right)
            return nullptr;
        expr = std::make_unique<BinaryExpression>(expr->begin(), right->end(), std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> parseTypedPrimary(lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context)
{
    if (begin == end)
        return nullptr;
    auto tType = begin->getTokenType();

    if (tType == lexer::TokenType::Literal && begin->getType() == lexer::CToken::Type::Integer)
    {
        auto t = begin++;
        return std::make_unique<LiteralExpression>(t, begin, t);
    }

    if (tType == lexer::TokenType::LocationKeyword)
    {
        auto startToken = begin++;
        if (begin != end && begin->getTokenType() == lexer::TokenType::OpenParentheses)
        {
            begin++;
            auto wordRef = parseWordReference(begin, end, context);
            if (!wordRef)
                return nullptr;
            if (begin != end && begin->getTokenType() == lexer::TokenType::CloseParentheses)
            {
                auto endToken = begin++;
                return std::make_unique<LocationExpression>(startToken, endToken, std::move(wordRef));
            }
        }
        return nullptr;
    }

    if (isNumberType(tType))
    {
        auto typeToken = begin++;
        std::unique_ptr<Expression> totalBits;
        std::unique_ptr<Expression> fractionBits;

        if (tType == lexer::TokenType::FixedKeyword)
        {
            if (begin != end && begin->getTokenType() == lexer::TokenType::OpenParentheses)
            {
                // To support both FIXED(Total, Fraction)(Expression) and FIXED(Expression)
                // Wait, Coral 66 requires Scale for FIXED? "FIXED Scale".
                // We parse Scale if there is a comma inside the parentheses.
                // But it's easier to just parse Expression. If the next token is Comma, it's a Scale!
                // Let's implement this logic:
                begin++; // skip (
                auto firstExpr = parseExpression(begin, end, context);
                if (!firstExpr)
                    return nullptr;
                
                if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
                {
                    begin++; // skip ,
                    fractionBits = parseExpression(begin, end, context);
                    if (!fractionBits) return nullptr;
                    if (begin != end && begin->getTokenType() == lexer::TokenType::CloseParentheses)
                    {
                        begin++; // skip )
                        totalBits = std::move(firstExpr);
                        // Now expect ( Expression )
                        if (begin != end && begin->getTokenType() == lexer::TokenType::OpenParentheses)
                        {
                            begin++; // skip (
                            auto expr = parseExpression(begin, end, context);
                            if (!expr) return nullptr;
                            if (begin != end && begin->getTokenType() == lexer::TokenType::CloseParentheses)
                            {
                                auto endToken = begin++; // skip )
                                return std::make_unique<TypeChangeExpression>(typeToken, endToken, typeToken, std::move(totalBits), std::move(fractionBits), std::move(expr));
                            }
                        }
                        return nullptr;
                    }
                    return nullptr;
                }
                else if (begin != end && begin->getTokenType() == lexer::TokenType::CloseParentheses)
                {
                    // It was FIXED ( Expression )
                    auto endToken = begin++; // skip )
                    return std::make_unique<TypeChangeExpression>(typeToken, endToken, typeToken, std::move(firstExpr));
                }
                return nullptr;
            }
        }
        else
        {
            if (begin != end && begin->getTokenType() == lexer::TokenType::OpenParentheses)
            {
                begin++; // skip (
                auto expr = parseExpression(begin, end, context);
                if (!expr)
                    return nullptr;
                if (begin != end && begin->getTokenType() == lexer::TokenType::CloseParentheses)
                {
                    auto endToken = begin++; // skip )
                    return std::make_unique<TypeChangeExpression>(typeToken, endToken, typeToken, std::move(expr));
                }
            }
        }
        return nullptr;
    }

    if (tType == lexer::TokenType::BitsKeyword)
    {
        auto startToken = begin++;
        if (begin != end && begin->getTokenType() == lexer::TokenType::OpenSquareBracket)
        {
            begin++;
            auto totalBits = parseExpression(begin, end, context);
            if (!totalBits)
                return nullptr;
            if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
            {
                begin++;
                auto bitPos = parseExpression(begin, end, context);
                if (!bitPos)
                    return nullptr;
                if (begin != end && begin->getTokenType() == lexer::TokenType::CloseSquareBracket)
                {
                    begin++;
                    auto typedPrim = parseTypedPrimary(begin, end, context);
                    if (!typedPrim)
                        return nullptr;
                    return std::make_unique<PartWordExpression>(startToken, typedPrim->end(), std::move(typedPrim),
                                                                std::move(bitPos), std::move(totalBits));
                }
            }
        }
        return nullptr;
    }

    if (tType == lexer::TokenType::OpenSquareBracket)
    {
        return parseWordReference(begin, end, context);
    }

    if (tType == lexer::TokenType::Identifier)
    {
        auto lookahead = begin + 1;
        if (lookahead != end && lookahead->getTokenType() == lexer::TokenType::OpenParentheses)
        {
            auto idToken = begin++;
            begin++; // skip (
            std::vector<std::unique_ptr<Expression>> args;
            if (begin != end && begin->getTokenType() != lexer::TokenType::CloseParentheses)
            {
                auto expr = parseExpression(begin, end, context);
                if (!expr)
                    return nullptr;
                args.push_back(std::move(expr));
                while (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
                {
                    begin++; // skip ,
                    auto nextExpr = parseExpression(begin, end, context);
                    if (!nextExpr)
                        return nullptr;
                    args.push_back(std::move(nextExpr));
                }
            }
            if (begin != end && begin->getTokenType() == lexer::TokenType::CloseParentheses)
            {
                auto endToken = begin++;
                return std::make_unique<FunctionCallExpression>(idToken, endToken, idToken, std::move(args));
            }
            return nullptr;
        }
        return parseWordReference(begin, end, context);
    }

    return nullptr;
}

std::unique_ptr<Expression> parseWordReference(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                               Context& context)
{
    if (begin == end)
        return nullptr;

    if (begin->getTokenType() == lexer::TokenType::OpenSquareBracket)
    {
        auto startToken = begin++;
        auto index = parseExpression(begin, end, context);
        if (!index)
            return nullptr;
        if (begin != end && begin->getTokenType() == lexer::TokenType::CloseSquareBracket)
        {
            auto endToken = begin++;
            std::vector<std::unique_ptr<Expression>> indices;
            indices.push_back(std::move(index));
            return std::make_unique<ArrayAccessExpression>(startToken, endToken, nullptr, std::move(indices));
        }
        return nullptr;
    }

    if (begin->getTokenType() == lexer::TokenType::Identifier)
    {
        auto idToken = begin++;
        if (begin != end && begin->getTokenType() == lexer::TokenType::OpenSquareBracket)
        {
            begin++;
            std::vector<std::unique_ptr<Expression>> indices;
            auto index1 = parseExpression(begin, end, context);
            if (!index1)
                return nullptr;
            indices.push_back(std::move(index1));

            if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
            {
                begin++;
                auto index2 = parseExpression(begin, end, context);
                if (!index2)
                    return nullptr;
                indices.push_back(std::move(index2));
            }

            if (begin != end && begin->getTokenType() == lexer::TokenType::CloseSquareBracket)
            {
                auto endToken = begin++;
                return std::make_unique<ArrayAccessExpression>(idToken, endToken, idToken, std::move(indices));
            }
            return nullptr;
        }
        return std::make_unique<IdentifierExpression>(idToken, begin, idToken);
    }
    return nullptr;
}

} // namespace coral66::parser
