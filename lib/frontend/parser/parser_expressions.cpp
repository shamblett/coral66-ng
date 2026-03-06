/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "parser.hpp"

#include <llvm/Support/ConvertUTF.h>

#include <algorithm>

#include "lib/frontend/common/messages/errors.hpp"

using namespace coral66::Syntax;

/*
 * Section 6
 *
 * Coral 66 expressions comprise  :-
 *
 * Simple expressions
 *
 * Primaries
 *
 * Word Logic
 *
 * Conditional Expressions
 *
 */

namespace
{
[[maybe_unused]] bool isPostFixOperator(const coral66::lexer::CToken& token)
{
    switch (token.getTokenType())
    {
        case coral66::lexer::TokenType::Dot:
        case coral66::lexer::TokenType::OpenSquareBracket:
        case coral66::lexer::TokenType::OpenParentheses:
        default: break;
    }
    return false;
}
} // namespace

namespace coral66::Parser
{
std::unique_ptr<Syntax::Expression> parseExpression(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                    Context& context)
{
    if (begin == end)
    {
        return nullptr;
    }

    std::unique_ptr<Syntax::Expression> left;

    if (begin->getTokenType() == lexer::TokenType::Identifier)
    {
        auto id = begin;
        begin++;
        // Hack: consume () for procedure calls to allow parsing as expression
        if (begin != end && begin->getTokenType() == lexer::TokenType::OpenParentheses)
        {
            begin++;
            if (begin != end && begin->getTokenType() == lexer::TokenType::CloseParentheses)
            {
                begin++;
            }
        }
        left = std::make_unique<Syntax::IdentifierExpression>(id, begin, id);
    }
    else if (begin->getTokenType() == lexer::TokenType::Literal
             || begin->getTokenType() == lexer::TokenType::StringLiteral)
    {
        auto lit = begin;
        begin++;
        left = std::make_unique<Syntax::LiteralExpression>(lit, begin, lit);
    }
    else if (begin->getTokenType() == lexer::TokenType::BitsKeyword)
    {
        auto start = begin;
        begin++;
        if (begin != end && begin->getTokenType() == lexer::TokenType::OpenParentheses)
        {
            begin++;
            auto s = parseExpression(begin, end, context);
            if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
            {
                begin++;
                auto w = parseExpression(begin, end, context);
                if (begin != end && begin->getTokenType() == lexer::TokenType::CloseParentheses)
                {
                    begin++;
                    auto base = parseExpression(begin, end, context);
                    if (s && w && base)
                    {
                        return std::make_unique<Syntax::PartWordExpression>(start, begin, std::move(base), std::move(s),
                                                                            std::move(w));
                    }
                }
            }
        }
    }

    if (left && begin != end)
    {
        auto type = begin->getTokenType();
        if (type == lexer::TokenType::Plus || type == lexer::TokenType::Minus || type == lexer::TokenType::Asterisk
            || type == lexer::TokenType::Division)
        {
            auto op = begin;
            begin++;
            if (auto right = parseExpression(begin, end, context))
            {
                return std::make_unique<Syntax::BinaryExpression>(left->begin(), right->end(), std::move(left), op,
                                                                  std::move(right));
            }
        }
    }

    return left;
}
} // namespace coral66::Parser