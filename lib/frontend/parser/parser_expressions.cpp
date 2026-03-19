/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "parser.hpp"

#include "lib/frontend/common/messages/errors.hpp"
#include "parser_support.hpp"

using namespace coral66::Syntax;

namespace coral66::parser
{

/*
 * Parser expression sub parsers
 * */

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
    {
        context.log(errors::parser::EXPECTED_CONDITION.args(*begin, context.getSourceInterface(),
                                                            lexer::TokenType::IfKeyword, *begin));
        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_CONDITION);
        return nullptr;
    }
    if (begin == end || begin->getTokenType() != lexer::TokenType::ThenKeyword)
    {
        context.log(errors::parser::EXPECTED_THEN.args(*begin, context.getSourceInterface(), *begin));
        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_THEN);
        return nullptr; // Missing THEN
    }
    begin++; // THEN
    auto thenExpr = parseExpression(begin, end, context);
    if (!thenExpr)
    {
        context.log(errors::parser::EXPECTED_EXPRESSION.args(*begin, context.getSourceInterface(),
        *begin));
        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_EXPRESSION);
        return nullptr;
    }

    if (begin == end || begin->getTokenType() != lexer::TokenType::ElseKeyword)
    {
        context.log(errors::parser::EXPECTED_ELSE.args(*begin, context.getSourceInterface(), *begin));
        return nullptr; // Missing ELSE
    }
    begin++; // ELSE
    auto elseExpr = parseExpression(begin, end, context);
    if (!elseExpr)
    {
        context.log(errors::parser::EXPECTED_ELSE.args(*begin, context.getSourceInterface(), *begin));
        return nullptr;
    }

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
    {
        context.log(errors::parser::EXPECTED_EXPRESSION.args(*begin, context.getSourceInterface(), *begin));
        return nullptr;
    }
    while (begin != end && begin->getTokenType() == lexer::TokenType::OrKeyword)
    {
        auto op = begin++;
        auto right = parseSubcondition(begin, end, context);
        if (!right)
        {
            context.log(errors::parser::EXPECTED_EXPRESSION.args(*begin, context.getSourceInterface(), *begin));
            return nullptr;
        }
        expr = std::make_unique<BinaryExpression>(expr->begin(), right->end(), std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> parseSubcondition(lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context)
{
    auto expr = parseComparison(begin, end, context);
    if (!expr)
    {
        context.log(errors::parser::EXPECTED_COMPARATOR.args(*begin, context.getSourceInterface(), *begin));
        return nullptr;
    }
    while (begin != end && begin->getTokenType() == lexer::TokenType::AndKeyword)
    {
        auto op = begin++;
        auto right = parseComparison(begin, end, context);
        if (!right)
        {
            context.log(errors::parser::EXPECTED_COMPARATOR.args(*begin, context.getSourceInterface(), *begin));
            return nullptr;
        }
        expr = std::make_unique<BinaryExpression>(expr->begin(), right->end(), std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> parseComparison(lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context)
{
    auto expr = parseSimpleExpression(begin, end, context);
    if (!expr)
    {
        context.log(errors::parser::EXPECTED_SIMPLE_EXPRESSION.args(*begin, context.getSourceInterface(), *begin));
        return nullptr;
    }
    if (begin != end && isComparator(begin->getTokenType()))
    {
        auto op = begin++;
        auto right = parseSimpleExpression(begin, end, context);
        if (!right)
        {
            context.log(errors::parser::EXPECTED_SIMPLE_EXPRESSION.args(*begin, context.getSourceInterface(), *begin));
            return nullptr;
        }
        expr = std::make_unique<BinaryExpression>(expr->begin(), right->end(), std::move(expr), op, std::move(right));
    }
    else
    {
        // Coral 66 requires a comparator for a comparison
        context.log(errors::parser::EXPECTED_COMPARATOR.args(*begin, context.getSourceInterface(), *begin));
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
        {
            context.log(errors::parser::EXPECTED_TERM.args(*begin, context.getSourceInterface(), *begin));
            return nullptr;
        }
        expr = std::make_unique<UnaryExpression>(op, right->end(), op, std::move(right));
    }
    else
    {
        expr = parseTerm(begin, end, context);
    }
    if (!expr)
    {
        context.log(errors::parser::EXPECTED_TERM.args(*begin, context.getSourceInterface(), *begin));
        return nullptr;
    }
    while (begin != end && isAddOperator(begin->getTokenType()))
    {
        auto op = begin++;
        auto right = parseTerm(begin, end, context);
        if (!right)
        {
            context.log(errors::parser::EXPECTED_TERM.args(*begin, context.getSourceInterface(), *begin));
            return nullptr;
        }
        expr = std::make_unique<BinaryExpression>(expr->begin(), right->end(), std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> parseTerm(lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context)
{
    auto expr = parseFactor(begin, end, context);
    if (!expr)
    {
        context.log(errors::parser::EXPECTED_FACTOR.args(*begin, context.getSourceInterface(), *begin));
        return nullptr;
    }
    while (begin != end && isMultOperator(begin->getTokenType()))
    {
        auto op = begin++;
        auto right = parseFactor(begin, end, context);
        if (!right)
        {
            context.log(errors::parser::EXPECTED_FACTOR.args(*begin, context.getSourceInterface(), *begin));
            return nullptr;
        }
        expr = std::make_unique<BinaryExpression>(expr->begin(), right->end(), std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> parseFactor(lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context)
{
    if (begin == end)
        return nullptr;

    // Untyped Primary
    if (begin->getTokenType() == lexer::TokenType::OpenParentheses)
    {
        begin++;
        auto expr = parseExpression(begin, end, context);
        if (!expr)
        {
            context.log(errors::parser::EXPECTED_EXPRESSION.args(*begin, context.getSourceInterface(), *begin));
            return nullptr;
        }
        if (begin != end && begin->getTokenType() == lexer::TokenType::CloseParentheses)
        {
            begin++;
        }
        else
        {
            context.log(errors::parser::EXPECTED_CLOSED_PARENTHESIS.args(*begin, context.getSourceInterface(), *begin));
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
    {
        context.log(errors::parser::EXPECTED_UNION.args(*begin, context.getSourceInterface(), *begin));
        return nullptr;
    }
    while (begin != end && begin->getTokenType() == lexer::TokenType::DifferKeyword)
    {
        auto op = begin++;
        auto right = parseBitwiseUnion(begin, end, context);
        if (!right)
        {
            context.log(errors::parser::EXPECTED_UNION.args(*begin, context.getSourceInterface(), *begin));
            return nullptr;
        }
        expr = std::make_unique<BinaryExpression>(expr->begin(), right->end(), std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> parseBitwiseUnion(lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context)
{
    auto expr = parseBitwiseMask(begin, end, context);
    if (!expr)
    {
        context.log(errors::parser::EXPECTED_MASK.args(*begin, context.getSourceInterface(), *begin));
        return nullptr;
    }
    while (begin != end && begin->getTokenType() == lexer::TokenType::UnionKeyword)
    {
        auto op = begin++;
        auto right = parseBitwiseMask(begin, end, context);
        if (!right)
        {
            context.log(errors::parser::EXPECTED_MASK.args(*begin, context.getSourceInterface(), *begin));
            return nullptr;
        }
        expr = std::make_unique<BinaryExpression>(expr->begin(), right->end(), std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> parseBitwiseMask(lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context)
{
    auto expr = parseTypedPrimary(begin, end, context);
    if (!expr)
    {
        context.log(errors::parser::EXPECTED_TYPED_PRIMARY.args(*begin, context.getSourceInterface(), *begin));
        return nullptr;
    }
    while (begin != end && begin->getTokenType() == lexer::TokenType::MaskKeyword)
    {
        auto op = begin++;
        auto right = parseTypedPrimary(begin, end, context);
        if (!right)
        {
            context.log(errors::parser::EXPECTED_TYPED_PRIMARY.args(*begin, context.getSourceInterface(), *begin));
            return nullptr;
        }
        expr = std::make_unique<BinaryExpression>(expr->begin(), right->end(), std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> parseTypedPrimary(lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context)
{
    if (begin == end)
        return nullptr;

    const auto tType = begin->getTokenType();

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
            {
                context.log(errors::parser::EXPECTED_WORD_REFERENCE.args(*begin, context.getSourceInterface(), *begin));
                return nullptr;
            }
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

        if (tType == lexer::TokenType::FixedKeyword)
        {
            if (begin != end && begin->getTokenType() == lexer::TokenType::OpenParentheses)
            {
                begin++; // skip (
                auto firstExpr = parseExpression(begin, end, context);
                if (!firstExpr)
                {
                    context.log(errors::parser::EXPECTED_EXPRESSION.args(*begin, context.getSourceInterface(), *begin));
                    return nullptr;
                }

                if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
                {
                    begin++; // skip ,
                    std::unique_ptr<Expression> fractionBits = parseExpression(begin, end, context);
                    if (!fractionBits)
                    {
                        context.log(
                            errors::parser::EXPECTED_FRACTION_BITS.args(*begin, context.getSourceInterface(), *begin));
                        return nullptr;
                    }
                    if (begin != end && begin->getTokenType() == lexer::TokenType::CloseParentheses)
                    {
                        begin++; // skip )
                        std::unique_ptr<Expression> totalBits = std::move(firstExpr);
                        // Now expect ( Expression )
                        if (begin != end && begin->getTokenType() == lexer::TokenType::OpenParentheses)
                        {
                            begin++; // skip (
                            auto expr = parseExpression(begin, end, context);
                            if (!expr)
                            {
                                context.log(errors::parser::EXPECTED_EXPRESSION.args(
                                    *begin, context.getSourceInterface(), *begin));
                                return nullptr;
                            }
                            if (begin != end && begin->getTokenType() == lexer::TokenType::CloseParentheses)
                            {
                                auto endToken = begin++; // skip )
                                return std::make_unique<TypeChangeExpression>(typeToken, endToken, typeToken,
                                                                              std::move(totalBits),
                                                                              std::move(fractionBits), std::move(expr));
                            }
                        }
                        context.log(errors::parser::EXPECTED_OPEN_PARENTHESIS.args(*begin, context.getSourceInterface(),
                                                                                   *begin));
                        return nullptr;
                    }
                    context.log(
                        errors::parser::EXPECTED_CLOSED_PARENTHESIS.args(*begin, context.getSourceInterface(), *begin));
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
                {
                    context.log(errors::parser::EXPECTED_EXPRESSION.args(*begin, context.getSourceInterface(), *begin));
                    return nullptr;
                }
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
            {
                context.log(errors::parser::EXPECTED_EXPRESSION.args(*begin, context.getSourceInterface(), *begin));
                return nullptr;
            }
            if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
            {
                begin++;
                auto bitPos = parseExpression(begin, end, context);
                if (!bitPos)
                {
                    context.log(errors::parser::EXPECTED_EXPRESSION.args(*begin, context.getSourceInterface(), *begin));
                    return nullptr;
                }
                if (begin != end && begin->getTokenType() == lexer::TokenType::CloseSquareBracket)
                {
                    begin++;
                    auto typedPrim = parseTypedPrimary(begin, end, context);
                    if (!typedPrim)
                    {
                        context.log(
                            errors::parser::EXPECTED_TYPED_PRIMARY.args(*begin, context.getSourceInterface(), *begin));
                        return nullptr;
                    }
                    return std::make_unique<PartWordExpression>(startToken, typedPrim->end(), std::move(typedPrim),
                                                                std::move(bitPos), std::move(totalBits));
                }
            }
        }
        context.log(errors::parser::EXPECTED_OPEN_SQUARE_BRACKET.args(*begin, context.getSourceInterface(), *begin));
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
                {
                    context.log(errors::parser::EXPECTED_EXPRESSION.args(*begin, context.getSourceInterface(), *begin));
                    return nullptr;
                }
                args.push_back(std::move(expr));
                while (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
                {
                    begin++; // skip ,
                    auto nextExpr = parseExpression(begin, end, context);
                    if (!nextExpr)
                    {
                        context.log(
                            errors::parser::EXPECTED_EXPRESSION.args(*begin, context.getSourceInterface(), *begin));
                        return nullptr;
                    }
                    args.push_back(std::move(nextExpr));
                }
            }
            if (begin != end && begin->getTokenType() == lexer::TokenType::CloseParentheses)
            {
                auto endToken = begin++;
                return std::make_unique<FunctionCallExpression>(idToken, endToken, idToken, std::move(args));
            }
            context.log(errors::parser::EXPECTED_CLOSED_PARENTHESIS.args(*begin, context.getSourceInterface(), *begin));
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
        {
            context.log(errors::parser::EXPECTED_EXPRESSION.args(*begin, context.getSourceInterface(), *begin));
            return nullptr;
        }
        if (begin != end && begin->getTokenType() == lexer::TokenType::CloseSquareBracket)
        {
            auto endToken = begin++;
            std::vector<std::unique_ptr<Expression>> indices;
            indices.push_back(std::move(index));
            return std::make_unique<ArrayAccessExpression>(startToken, endToken, nullptr, std::move(indices));
        }
        context.log(errors::parser::EXPECTED_OPEN_SQUARE_BRACKET.args(*begin, context.getSourceInterface(), *begin));
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
            {
                context.log(errors::parser::EXPECTED_EXPRESSION.args(*begin, context.getSourceInterface(), *begin));
                return nullptr;
            }
            indices.push_back(std::move(index1));

            if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
            {
                begin++;
                auto index2 = parseExpression(begin, end, context);
                if (!index2)
                {
                    context.log(errors::parser::EXPECTED_EXPRESSION.args(*begin, context.getSourceInterface(), *begin));
                    return nullptr;
                }
                indices.push_back(std::move(index2));
            }

            if (begin != end && begin->getTokenType() == lexer::TokenType::CloseSquareBracket)
            {
                auto endToken = begin++;
                return std::make_unique<ArrayAccessExpression>(idToken, endToken, idToken, std::move(indices));
            }
            context.log(
                errors::parser::EXPECTED_OPEN_SQUARE_BRACKET.args(*begin, context.getSourceInterface(), *begin));
            return nullptr;
        }
        return std::make_unique<IdentifierExpression>(idToken, begin, idToken);
    }
    context.log(errors::parser::EXPECTED_IDENTIFIER.args(*begin, context.getSourceInterface(), *begin));
    return nullptr;
}

} // namespace coral66::parser
