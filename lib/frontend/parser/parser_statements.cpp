/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 06/03/2026
 */

#include "parser.hpp"

#include <algorithm>

#include "lib/frontend/common/messages/errors.hpp"

namespace coral66::parser
{
/*
 * Section 5 and 7
 *
 * Coral 66 statements comprise :-
 *
 * Assignments
 *
 * Goto
 *
 * Procedure
 *
 * Answer
 *
 * Code
 *
 * Compound
 *
 * Blocks
 *
 * Dummy
 *
 * Conditional
 *
 * For
 *
 * SWitches
 */

std::unique_ptr<Syntax::Statement> parseStatement(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                  Context& context)
{
    if (begin == end)
    {
        return nullptr;
    }
    if (begin->getTokenType() == lexer::TokenType::Identifier)
    {
        auto start = begin;
        if (auto iter = begin + 1; iter != end && iter->getTokenType() == lexer::TokenType::Assignment)
        {
            auto id = begin;
            begin++;
            begin++;
            auto expr = parseExpression(begin, end, context);
            if (begin != end && begin->getTokenType() == lexer::TokenType::SemiColon)
            {
                begin++;
            }
            if (expr)
            {
                return std::make_unique<Syntax::AssignmentStatement>(id, begin, id, std::move(expr));
            }
        }
        else if (iter != end
                 && (iter->getTokenType() == lexer::TokenType::OpenParentheses
                     || iter->getTokenType() == lexer::TokenType::SemiColon))
        {
            // Call: id(...) or id;
            auto expr = parseExpression(begin, end, context);
            if (begin != end && begin->getTokenType() == lexer::TokenType::SemiColon)
            {
                begin++;
            }
            if (expr)
            {
                return std::make_unique<Syntax::ProcedureCallStatement>(start, begin, std::move(expr));
            }
        }
        return nullptr;
    }
    if (begin->getTokenType() == lexer::TokenType::GotoKeyword)
    {
        auto start = begin;
        begin++;
        if (begin != end && begin->getTokenType() == lexer::TokenType::Identifier)
        {
            auto label = begin;
            begin++;
            if (begin != end && begin->getTokenType() == lexer::TokenType::SemiColon)
            {
                begin++;
            }
            return std::make_unique<Syntax::GotoStatement>(start, begin, label);
        }
        return nullptr;
    }
    if (begin->getTokenType() == lexer::TokenType::IfKeyword)
    {
        auto start = begin;
        begin++;
        auto cond = parseExpression(begin, end, context);
        if (begin != end && begin->getTokenType() == lexer::TokenType::ThenKeyword)
        {
            begin++;
            auto thenBranch = parseStatement(begin, end, context);
            std::unique_ptr<Syntax::Statement> elseBranch = nullptr;
            if (begin != end && begin->getTokenType() == lexer::TokenType::ElseKeyword)
            {
                begin++;
                elseBranch = parseStatement(begin, end, context);
            }
            if (cond && thenBranch)
            {
                return std::make_unique<Syntax::IfStatement>(start, begin, std::move(cond), std::move(thenBranch),
                                                             std::move(elseBranch));
            }
        }
        return nullptr;
    }
    if (begin->getTokenType() == lexer::TokenType::CodeKeyword)
    {
        auto start = begin;
        begin++;
        if (begin != end && begin->getTokenType() == lexer::TokenType::StringLiteral)
        {
            begin++;
            if (begin != end && begin->getTokenType() == lexer::TokenType::SemiColon)
            {
                begin++;
            }
            return std::make_unique<Syntax::CodeStatement>(start, begin);
        }
    }

    if (begin->getTokenType() == lexer::TokenType::ForKeyword)
    {
        auto start = begin;
        begin++;
        if (begin != end && begin->getTokenType() == lexer::TokenType::Identifier)
        {
            auto var = begin;
            begin++;
            if (begin != end && begin->getTokenType() == lexer::TokenType::Assignment)
            {
                begin++;
                auto startExpr = parseExpression(begin, end, context);
                if (begin != end && begin->getTokenType() == lexer::TokenType::StepKeyword)
                {
                    begin++;
                    auto stepExpr = parseExpression(begin, end, context);
                    if (begin != end && begin->getTokenType() == lexer::TokenType::UntilKeyword)
                    {
                        begin++;
                        auto untilExpr = parseExpression(begin, end, context);
                        if (begin != end && begin->getTokenType() == lexer::TokenType::DoKeyword)
                        {
                            begin++;
                            auto body = parseStatement(begin, end, context);
                            if (startExpr && stepExpr && untilExpr && body)
                            {
                                return std::make_unique<Syntax::ForStatement>(start, begin, var, std::move(startExpr),
                                                                              std::move(stepExpr), std::move(untilExpr),
                                                                              std::move(body));
                            }
                        }
                    }
                }
            }
        }
        return nullptr;
    }

    if (begin->getTokenType() == lexer::TokenType::AnswerKeyword)
    {
        auto start = begin;
        begin++;
        auto expr = parseExpression(begin, end, context);
        if (begin != end && begin->getTokenType() == lexer::TokenType::SemiColon)
        {
            begin++;
        }
        if (expr)
        {
            return std::make_unique<Syntax::AnswerStatement>(start, begin, std::move(expr));
        }
        return nullptr;
    }

    return nullptr;
}
} // namespace coral66::parser