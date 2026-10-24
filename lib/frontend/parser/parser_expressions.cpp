/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "parser.hpp"

#include "lib/frontend/common/messages/errors.hpp"
#include "parser_support.hpp"

using namespace coral66::syntax;

namespace coral66::parser
{

/*
 *  Section 6
 */

static std::unique_ptr<Expression> parseConditionalExpression(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                              Context& context);
static std::unique_ptr<Expression> parseUnconditionalExpression(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                                Context& context);

static std::unique_ptr<Expression> parseSubcondition(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                     Context& context);
static std::unique_ptr<Expression> parseComparison(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                   Context& context);
static std::unique_ptr<Expression> parseSimpleExpression(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                         Context& context);
static std::unique_ptr<Expression> parseTerm(lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context);
static std::unique_ptr<Expression> parseFactor(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                               Context& context);
static std::unique_ptr<Expression> parseBitwiseDiffer(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                      Context& context);
static std::unique_ptr<Expression> parseBitwiseUnion(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                     Context& context);
static std::unique_ptr<Expression> parseBitwiseMask(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                    Context& context);
static std::unique_ptr<Expression> parseTypedPrimary(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                     Context& context);
std::unique_ptr<Expression> parseWordReference(lexer::CTokenIterator& begin,
                                               lexer::CTokenIterator end, // NOLINT(*-redundant-declaration)
                                               Context& context);

static bool isComparator(const lexer::TokenType t)
{
    return t == lexer::TokenType::LessThan || t == lexer::TokenType::LessThanOrEqual || t == lexer::TokenType::Equal
           || t == lexer::TokenType::GreaterThanOrEqual || t == lexer::TokenType::GreaterThan
           || t == lexer::TokenType::NotEqual;
}

static bool isAddOperator(const lexer::TokenType t)
{
    return t == lexer::TokenType::Plus || t == lexer::TokenType::Minus;
}

static bool isMultOperator(const lexer::TokenType t)
{
    return t == lexer::TokenType::Asterisk || t == lexer::TokenType::Division;
}

static bool isNumberType(const lexer::TokenType t)
{
    return t == lexer::TokenType::IntegerKeyword || t == lexer::TokenType::PointerKeyword
           || t == lexer::TokenType::FloatingKeyword || t == lexer::TokenType::FixedKeyword;
}

std::unique_ptr<Expression> parseExpression(lexer::CTokenIterator& begin, const lexer::CTokenIterator end,
                                            Context& context)
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

std::unique_ptr<Expression> parseConditionalExpression(lexer::CTokenIterator& begin, const lexer::CTokenIterator end,
                                                       Context& context)
{
    auto startToken = begin++; // IF
    auto condition = parseCondition(begin, end, context);
    if (!condition)
    {
        context.log(errors::parser::EXPECTED_CONDITION.args(*(begin == end ? end - 1 : begin),
                                                            context.getSourceInterface(), lexer::TokenType::IfKeyword,
                                                            *(begin == end ? end - 1 : begin)));
        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_CONDITION);
        return nullptr;
    }
    if (begin == end || begin->getTokenType() != lexer::TokenType::ThenKeyword)
    {
        context.log(errors::parser::EXPECTED_THEN.args(*(begin == end ? end - 1 : begin), context.getSourceInterface(),
                                                       *(begin == end ? end - 1 : begin)));
        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_THEN);
        return nullptr; // Missing THEN
    }
    begin++; // THEN
    auto thenExpr = parseExpression(begin, end, context);
    if (!thenExpr)
    {
        context.log(errors::parser::EXPECTED_STATEMENT.args(
            *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_STATEMENT);
        return nullptr;
    }

    if (begin == end || begin->getTokenType() != lexer::TokenType::ElseKeyword)
    {
        context.log(errors::parser::EXPECTED_ELSE.args(*(begin == end ? end - 1 : begin), context.getSourceInterface(),
                                                       *(begin == end ? end - 1 : begin)));
        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_ELSE);
        return nullptr; // Missing ELSE
    }
    begin++; // ELSE
    auto elseExpr = parseExpression(begin, end, context);
    if (!elseExpr)
    {
        context.log(errors::parser::EXPECTED_STATEMENT.args(
            *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_STATEMENT);
        return nullptr;
    }

    return std::make_unique<ConditionalExpression>(startToken, elseExpr->end(), std::move(condition),
                                                   std::move(thenExpr), std::move(elseExpr));
}

std::unique_ptr<Expression> parseUnconditionalExpression(lexer::CTokenIterator& begin, const lexer::CTokenIterator end,
                                                         Context& context)
{
    if (begin->getTokenType() == lexer::TokenType::StringLiteral)
    {
        auto t = begin++;
        return std::make_unique<LiteralExpression>(t, begin, t);
    }
    return parseSimpleExpression(begin, end, context);
}

std::unique_ptr<Expression> parseCondition(lexer::CTokenIterator& begin, const lexer::CTokenIterator end,
                                           Context& context)
{
    auto expr = parseSubcondition(begin, end, context);
    if (!expr)
    {
        context.log(errors::parser::EXPECTED_EXPRESSION.args(
            *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_EXPRESSION);
        return nullptr;
    }
    while (begin != end && begin->getTokenType() == lexer::TokenType::OrKeyword)
    {
        auto op = begin++;
        auto right = parseSubcondition(begin, end, context);
        if (!right)
        {
            context.log(errors::parser::EXPECTED_EXPRESSION.args(
                *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_EXPRESSION);
            return nullptr;
        }
        expr = std::make_unique<BinaryExpression>(expr->begin(), right->end(), std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> parseSubcondition(lexer::CTokenIterator& begin, const lexer::CTokenIterator end,
                                              Context& context)
{
    auto expr = parseComparison(begin, end, context);
    if (!expr)
    {
        context.log(errors::parser::EXPECTED_COMPARATOR.args(
            *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_COMPARATOR);
        return nullptr;
    }
    while (begin != end && begin->getTokenType() == lexer::TokenType::AndKeyword)
    {
        auto op = begin++;
        auto right = parseComparison(begin, end, context);
        if (!right)
        {
            context.log(errors::parser::EXPECTED_COMPARATOR.args(
                *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_COMPARATOR);
            return nullptr;
        }
        expr = std::make_unique<BinaryExpression>(expr->begin(), right->end(), std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> parseComparison(lexer::CTokenIterator& begin, const lexer::CTokenIterator end,
                                            Context& context)
{
    auto expr = parseSimpleExpression(begin, end, context);
    if (!expr)
    {
        context.log(errors::parser::EXPECTED_SIMPLE_EXPRESSION.args(
            *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_SIMPLE_EXPRESSION);
        return nullptr;
    }
    if (begin != end && isComparator(begin->getTokenType()))
    {
        auto op = begin++;
        auto right = parseSimpleExpression(begin, end, context);
        if (!right)
        {
            context.log(errors::parser::EXPECTED_SIMPLE_EXPRESSION.args(
                *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_SIMPLE_EXPRESSION);
            return nullptr;
        }
        expr = std::make_unique<BinaryExpression>(expr->begin(), right->end(), std::move(expr), op, std::move(right));
    }
    else
    {
        // Coral 66 requires a comparator for a comparison
        context.log(errors::parser::EXPECTED_COMPARATOR.args(
            *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_COMPARATOR);
        return nullptr;
    }
    return expr;
}

std::unique_ptr<Expression> parseSimpleExpression(lexer::CTokenIterator& begin, const lexer::CTokenIterator end,
                                                  Context& context)
{
    std::unique_ptr<Expression> expr;
    if (begin != end && isAddOperator(begin->getTokenType()))
    {
        auto op = begin++;
        auto right = parseTerm(begin, end, context);
        if (!right)
        {
            context.log(errors::parser::EXPECTED_TERM.args(
                *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_TERM);
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
        context.log(errors::parser::EXPECTED_TERM.args(*(begin == end ? end - 1 : begin), context.getSourceInterface(),
                                                       *(begin == end ? end - 1 : begin)));
        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_TERM);
        return nullptr;
    }
    while (begin != end && isAddOperator(begin->getTokenType()))
    {
        auto op = begin++;
        auto right = parseTerm(begin, end, context);
        if (!right)
        {
            context.log(errors::parser::EXPECTED_TERM.args(
                *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_TERM);
            return nullptr;
        }
        expr = std::make_unique<BinaryExpression>(expr->begin(), right->end(), std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> parseTerm(lexer::CTokenIterator& begin, const lexer::CTokenIterator end, Context& context)
{
    auto expr = parseFactor(begin, end, context);
    if (!expr)
    {
        context.log(errors::parser::EXPECTED_FACTOR.args(
            *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_FACTOR);
        return nullptr;
    }
    while (begin != end && isMultOperator(begin->getTokenType()))
    {
        auto op = begin++;
        auto right = parseFactor(begin, end, context);
        if (!right)
        {
            context.log(errors::parser::EXPECTED_FACTOR.args(
                *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_FACTOR);
            return nullptr;
        }
        expr = std::make_unique<BinaryExpression>(expr->begin(), right->end(), std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> parseFactor(lexer::CTokenIterator& begin, const lexer::CTokenIterator end, Context& context)
{
    if (begin == end)
        return nullptr;

    if (isAddOperator(begin->getTokenType()))
    {
        auto op = begin++;
        auto right = parseFactor(begin, end, context);
        if (!right)
        {
            context.log(errors::parser::EXPECTED_FACTOR.args(
                *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_FACTOR);
            return nullptr;
        }
        return std::make_unique<UnaryExpression>(op, right->end(), op, std::move(right));
    }

    // Untyped Primary
    std::unique_ptr<Expression> untypedExpr;
    if (begin->getTokenType() == lexer::TokenType::OpenParentheses)
    {
        begin++;
        auto expr = parseExpression(begin, end, context);
        if (!expr)
        {
            context.log(errors::parser::EXPECTED_EXPRESSION.args(
                *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_EXPRESSION);
            return nullptr;
        }
        if (begin != end && begin->getTokenType() == lexer::TokenType::CloseParentheses)
        {
            begin++;
        }
        else
        {
            context.log(errors::parser::EXPECTED_CLOSE_PARENTHESIS.args(
                *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_CLOSE_PARENTHESIS);
            return nullptr; // Missing )
        }
        untypedExpr = std::move(expr);
    }
    else if (begin->getTokenType() == lexer::TokenType::Literal && begin->getType() != lexer::CToken::Type::Integer)
    {
        auto t = begin++;
        untypedExpr = std::make_unique<LiteralExpression>(t, begin, t);
    }
    else if (begin->getTokenType() == lexer::TokenType::OctalKeyword)
    {
        if (const auto openParen = begin + 1;
            openParen != end && openParen->getTokenType() == lexer::TokenType::OpenParentheses)
        {
            if (const auto lit = openParen + 1;
                lit != end && lit->getTokenType() == lexer::TokenType::Literal
                && lit->getType() == lexer::CToken::Type::Floating)
            {
                const auto startToken = begin;
                begin = lit + 1;
                if (begin != end && begin->getTokenType() == lexer::TokenType::CloseParentheses)
                {
                    begin++;
                    untypedExpr = std::make_unique<LiteralExpression>(startToken, begin, lit);
                }
                else
                {
                    context.log(errors::parser::EXPECTED_CLOSE_PARENTHESIS.args(
                        *(begin == end ? end - 1 : begin), context.getSourceInterface(),
                        *(begin == end ? end - 1 : begin)));
                    DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_CLOSE_PARENTHESIS);
                    return nullptr;
                }
            }
        }
    }

    if (untypedExpr)
    {
        if (begin != end
            && (begin->getTokenType() == lexer::TokenType::DifferKeyword
                || begin->getTokenType() == lexer::TokenType::UnionKeyword
                || begin->getTokenType() == lexer::TokenType::MaskKeyword))
        {
            context.log(errors::parser::EXPECTED_TYPED_PRIMARY.args(*begin, context.getSourceInterface(), *begin));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_TYPED_PRIMARY);
            return nullptr;
        }
        return untypedExpr;
    }

    return parseBitwiseDiffer(begin, end, context);
}

std::unique_ptr<Expression> parseBitwiseDiffer(lexer::CTokenIterator& begin, const lexer::CTokenIterator end,
                                               Context& context)
{
    auto expr = parseBitwiseUnion(begin, end, context);
    if (!expr)
    {
        context.log(errors::parser::EXPECTED_UNION.args(*(begin == end ? end - 1 : begin), context.getSourceInterface(),
                                                        *(begin == end ? end - 1 : begin)));
        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_UNION);
        return nullptr;
    }
    while (begin != end && begin->getTokenType() == lexer::TokenType::DifferKeyword)
    {
        auto op = begin++;
        auto right = parseBitwiseUnion(begin, end, context);
        if (!right)
        {
            context.log(errors::parser::EXPECTED_UNION.args(
                *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_UNION);
            return nullptr;
        }
        expr = std::make_unique<BinaryExpression>(expr->begin(), right->end(), std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> parseBitwiseUnion(lexer::CTokenIterator& begin, const lexer::CTokenIterator end,
                                              Context& context)
{
    auto expr = parseBitwiseMask(begin, end, context);
    if (!expr)
    {
        context.log(errors::parser::EXPECTED_MASK.args(*(begin == end ? end - 1 : begin), context.getSourceInterface(),
                                                       *(begin == end ? end - 1 : begin)));
        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_MASK);
        return nullptr;
    }
    while (begin != end && begin->getTokenType() == lexer::TokenType::UnionKeyword)
    {
        auto op = begin++;
        auto right = parseBitwiseMask(begin, end, context);
        if (!right)
        {
            context.log(errors::parser::EXPECTED_MASK.args(
                *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_MASK);
            return nullptr;
        }
        expr = std::make_unique<BinaryExpression>(expr->begin(), right->end(), std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> parseBitwiseMask(lexer::CTokenIterator& begin, const lexer::CTokenIterator end,
                                             Context& context)
{
    auto expr = parseTypedPrimary(begin, end, context);
    if (!expr)
    {
        context.log(errors::parser::EXPECTED_TYPED_PRIMARY.args(
            *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_TYPED_PRIMARY);
        return nullptr;
    }
    while (begin != end && begin->getTokenType() == lexer::TokenType::MaskKeyword)
    {
        auto op = begin++;
        auto right = parseTypedPrimary(begin, end, context);
        if (!right)
        {
            context.log(errors::parser::EXPECTED_TYPED_PRIMARY.args(
                *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_TYPED_PRIMARY);
            return nullptr;
        }
        expr = std::make_unique<BinaryExpression>(expr->begin(), right->end(), std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> parseTypedPrimary(lexer::CTokenIterator& begin, const lexer::CTokenIterator end,
                                              Context& context)
{
    if (begin == end)
        return nullptr;

    const auto tType = begin->getTokenType();

    if (tType == lexer::TokenType::Literal && begin->getType() == lexer::CToken::Type::Integer)
    {
        auto t = begin++;
        return std::make_unique<LiteralExpression>(t, begin, t);
    }

    if (tType == lexer::TokenType::OctalKeyword)
    {
        auto startToken = begin++;
        if (begin != end && begin->getTokenType() == lexer::TokenType::OpenParentheses)
        {
            begin++;
            if (begin != end && begin->getTokenType() == lexer::TokenType::Literal
                && begin->getType() == lexer::CToken::Type::Integer)
            {
                auto t = begin++; // Parse the octal digits list
                if (begin != end && begin->getTokenType() == lexer::TokenType::CloseParentheses)
                {
                    begin++;
                    return std::make_unique<LiteralExpression>(startToken, begin, t);
                }
            }
        }
        return nullptr;
    }

    if (tType == lexer::TokenType::LiteralKeyword)
    {
        auto startToken = begin++;
        if (begin != end && begin->getTokenType() == lexer::TokenType::OpenParentheses)
        {
            begin++;
            if (begin != end && begin->getTokenType() != lexer::TokenType::CloseParentheses
                && begin->getLength() == 1)
            {
                auto t = begin++; // Parse the character
                if (begin != end && begin->getTokenType() == lexer::TokenType::CloseParentheses)
                {
                    begin++;
                    return std::make_unique<LiteralExpression>(startToken, begin, t);
                }
            }
        }
        return nullptr;
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
                context.log(errors::parser::EXPECTED_WORD_REFERENCE.args(*(begin == end ? end - 1 : begin),
                                                                         context.getSourceInterface(),
                                                                         *(begin == end ? end - 1 : begin)));
                DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_WORD_REFERENCE);
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
                    context.log(errors::parser::EXPECTED_EXPRESSION.args(*(begin == end ? end - 1 : begin),
                                                                         context.getSourceInterface(),
                                                                         *(begin == end ? end - 1 : begin)));
                    DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_EXPRESSION);
                    return nullptr;
                }

                if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
                {
                    begin++; // skip ,
                    std::unique_ptr<Expression> fractionBits = parseExpression(begin, end, context);
                    if (!fractionBits)
                    {
                        context.log(errors::parser::EXPECTED_FRACTION_BITS.args(*(begin == end ? end - 1 : begin),
                                                                                context.getSourceInterface(),
                                                                                *(begin == end ? end - 1 : begin)));
                        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_FRACTION_BITS);
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
                                    *(begin == end ? end - 1 : begin), context.getSourceInterface(),
                                    *(begin == end ? end - 1 : begin)));
                                DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_EXPRESSION);
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
                        context.log(errors::parser::EXPECTED_OPEN_PARENTHESIS.args(*(begin == end ? end - 1 : begin),
                                                                                   context.getSourceInterface(),
                                                                                   *(begin == end ? end - 1 : begin)));
                        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_OPEN_PARENTHESIS);
                        return nullptr;
                    }
                    context.log(errors::parser::EXPECTED_CLOSE_PARENTHESIS.args(*(begin == end ? end - 1 : begin),
                                                                                context.getSourceInterface(),
                                                                                *(begin == end ? end - 1 : begin)));
                    DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_CLOSE_PARENTHESIS);
                    return nullptr;
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
                    context.log(errors::parser::EXPECTED_EXPRESSION.args(*(begin == end ? end - 1 : begin),
                                                                         context.getSourceInterface(),
                                                                         *(begin == end ? end - 1 : begin)));
                    DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_EXPRESSION);
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
                context.log(errors::parser::EXPECTED_EXPRESSION.args(*(begin == end ? end - 1 : begin),
                                                                     context.getSourceInterface(),
                                                                     *(begin == end ? end - 1 : begin)));
                DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_EXPRESSION);
                return nullptr;
            }
            if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
            {
                begin++;
                auto bitPos = parseExpression(begin, end, context);
                if (!bitPos)
                {
                    context.log(errors::parser::EXPECTED_EXPRESSION.args(*(begin == end ? end - 1 : begin),
                                                                         context.getSourceInterface(),
                                                                         *(begin == end ? end - 1 : begin)));
                    DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_EXPRESSION);
                    return nullptr;
                }
                if (begin != end && begin->getTokenType() == lexer::TokenType::CloseSquareBracket)
                {
                    begin++;
                    auto typedPrim = parseTypedPrimary(begin, end, context);
                    if (!typedPrim)
                    {
                        context.log(errors::parser::EXPECTED_TYPED_PRIMARY.args(*(begin == end ? end - 1 : begin),
                                                                                context.getSourceInterface(),
                                                                                *(begin == end ? end - 1 : begin)));
                        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_TYPED_PRIMARY);
                        return nullptr;
                    }
                    return std::make_unique<PartWordExpression>(startToken, typedPrim->end(), std::move(typedPrim),
                                                                std::move(bitPos), std::move(totalBits));
                }
            }
            context.log(
                errors::parser::INVALID_BITS_FORMAT.args(*startToken, context.getSourceInterface(), *startToken));
            DiagnosticCollector::addDiagnostic(errors::parser::INVALID_BITS_FORMAT);
            return nullptr;
        }
        context.log(errors::parser::EXPECTED_OPEN_SQUARE_BRACKET.args(
            *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_OPEN_SQUARE_BRACKET);
        return nullptr;
    }

    if (tType == lexer::TokenType::OpenSquareBracket)
    {
        return parseWordReference(begin, end, context);
    }

    if (tType == lexer::TokenType::Identifier)
    {
        if (const auto lookahead = begin + 1;
            lookahead != end && lookahead->getTokenType() == lexer::TokenType::OpenParentheses)
        {
            auto idToken = begin++;
            begin++; // skip (
            std::vector<std::unique_ptr<Expression>> args;
            if (begin != end && begin->getTokenType() != lexer::TokenType::CloseParentheses)
            {
                auto expr = parseExpression(begin, end, context);
                if (!expr)
                {
                    context.log(errors::parser::EXPECTED_EXPRESSION.args(*(begin == end ? end - 1 : begin),
                                                                         context.getSourceInterface(),
                                                                         *(begin == end ? end - 1 : begin)));
                    DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_EXPRESSION);
                    return nullptr;
                }
                args.push_back(std::move(expr));
                while (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
                {
                    begin++; // skip ,
                    auto nextExpr = parseExpression(begin, end, context);
                    if (!nextExpr)
                    {
                        context.log(errors::parser::EXPECTED_EXPRESSION.args(*(begin == end ? end - 1 : begin),
                                                                             context.getSourceInterface(),
                                                                             *(begin == end ? end - 1 : begin)));
                        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_EXPRESSION);
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
            context.log(errors::parser::EXPECTED_CLOSE_PARENTHESIS.args(
                *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_CLOSE_PARENTHESIS);
            return nullptr;
        }
        return parseWordReference(begin, end, context);
    }

    return nullptr;
}

std::unique_ptr<Expression> parseVariable(lexer::CTokenIterator& begin, const lexer::CTokenIterator end,
                                          Context& context)
{
    if (begin == end)
        return nullptr;

    if (begin->getTokenType() == lexer::TokenType::BitsKeyword)
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
                    auto wordRef = parseWordReference(begin, end, context);
                    if (!wordRef)
                    {
                        context.log(errors::parser::EXPECTED_WORD_REFERENCE.args(*(begin == end ? end - 1 : begin),
                                                                                 context.getSourceInterface(),
                                                                                 *(begin == end ? end - 1 : begin)));
                        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_WORD_REFERENCE);
                        return nullptr;
                    }
                    return std::make_unique<PartWordExpression>(startToken, wordRef->end(), std::move(wordRef),
                                                                std::move(bitPos), std::move(totalBits));
                }
            }
            context.log(
                errors::parser::INVALID_BITS_FORMAT.args(*startToken, context.getSourceInterface(), *startToken));
            DiagnosticCollector::addDiagnostic(errors::parser::INVALID_BITS_FORMAT);
            return nullptr;
        }
        context.log(errors::parser::EXPECTED_OPEN_SQUARE_BRACKET.args(
            *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_OPEN_SQUARE_BRACKET);
        return nullptr;
    }

    return parseWordReference(begin, end, context);
}

std::unique_ptr<Expression> parseWordReference(lexer::CTokenIterator& begin, const lexer::CTokenIterator end,
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
            context.log(errors::parser::EXPECTED_EXPRESSION.args(
                *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_EXPRESSION);
            return nullptr;
        }
        if (begin != end && begin->getTokenType() == lexer::TokenType::CloseSquareBracket)
        {
            auto endToken = begin++;
            std::vector<std::unique_ptr<Expression>> indices;
            indices.push_back(std::move(index));
            return std::make_unique<ArrayAccessExpression>(startToken, endToken, nullptr, std::move(indices));
        }
        context.log(errors::parser::EXPECTED_OPEN_SQUARE_BRACKET.args(
            *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_OPEN_SQUARE_BRACKET);
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
                context.log(errors::parser::EXPECTED_EXPRESSION.args(*(begin == end ? end - 1 : begin),
                                                                     context.getSourceInterface(),
                                                                     *(begin == end ? end - 1 : begin)));
                DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_EXPRESSION);
                return nullptr;
            }
            indices.push_back(std::move(index1));

            if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
            {
                begin++;
                auto index2 = parseExpression(begin, end, context);
                if (!index2)
                {
                    context.log(errors::parser::EXPECTED_EXPRESSION.args(*(begin == end ? end - 1 : begin),
                                                                         context.getSourceInterface(),
                                                                         *(begin == end ? end - 1 : begin)));
                    DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_EXPRESSION);
                    return nullptr;
                }
                indices.push_back(std::move(index2));
            }

            if (begin != end && begin->getTokenType() == lexer::TokenType::CloseSquareBracket)
            {
                auto endToken = begin++;
                return std::make_unique<ArrayAccessExpression>(idToken, endToken, idToken, std::move(indices));
            }
            context.log(errors::parser::EXPECTED_OPEN_SQUARE_BRACKET.args(
                *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_OPEN_SQUARE_BRACKET);
            return nullptr;
        }
        return std::make_unique<IdentifierExpression>(idToken, begin, idToken);
    }
    context.log(errors::parser::EXPECTED_IDENTIFIER.args(
        *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
    DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_IDENTIFIER);
    return nullptr;
}

} // namespace coral66::parser
