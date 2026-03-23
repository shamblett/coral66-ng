/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 06/03/2026
 */

#include "parser.hpp"

#include <algorithm>

#include "lib/frontend/common/messages/errors.hpp"
#include "parser_support.hpp"

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
 */

std::unique_ptr<Syntax::Statement> parseStatement(lexer::CTokenIterator& begin, const lexer::CTokenIterator end,
                                                  Context& context)
{
    if (begin == end)
    {
        return nullptr;
    }
    if (begin->getTokenType() == lexer::TokenType::Identifier
        || begin->getTokenType() == lexer::TokenType::OpenSquareBracket
        || begin->getTokenType() == lexer::TokenType::BitsKeyword)
    {
        auto start = begin;
        if (auto expr = parseExpression(begin, end, context))
        {
            if (begin != end && begin->getTokenType() == lexer::TokenType::Assignment)
            {
                begin++;
                auto rhs = parseExpression(begin, end, context);
                if (begin != end && begin->getTokenType() == lexer::TokenType::SemiColon)
                {
                    begin++;
                }
                if (rhs)
                {
                    return std::make_unique<Syntax::AssignmentStatement>(start, begin, std::move(expr), std::move(rhs));
                }
            }
            else if (begin != end && begin->getTokenType() == lexer::TokenType::SemiColon)
            {
                begin++;
                return std::make_unique<Syntax::ProcedureCallStatement>(start, begin, std::move(expr));
            }
        }
        context.log(errors::parser::EXPECTED_ASSIGNMENT.args(
            *(begin == end ? (end - 1) : begin), context.getSourceInterface(), *(begin == end ? (end - 1) : begin)));
        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_ASSIGNMENT);
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
        context.log(errors::parser::EXPECTED_IDENTIFIER.args(
            *(begin == end ? (end - 1) : begin), context.getSourceInterface(), *(begin == end ? (end - 1) : begin)));
        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_IDENTIFIER);
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
        else
        {
            context.log(errors::parser::EXPECTED_THEN.args(*(begin == end ? (end - 1) : begin),
                                                           context.getSourceInterface(),
                                                           *(begin == end ? (end - 1) : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_THEN);
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
        context.log(errors::parser::EXPECTED_STRING_LITERAL.args(
            *(begin == end ? (end - 1) : begin), context.getSourceInterface(), *(begin == end ? (end - 1) : begin)));
        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_STRING_LITERAL);
        return nullptr;
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
                        else
                        {
                            context.log(errors::parser::EXPECTED_DO.args(*(begin == end ? (end - 1) : begin),
                                                                         context.getSourceInterface(),
                                                                         *(begin == end ? (end - 1) : begin)));
                            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_DO);
                        }
                    }
                    else
                    {
                        context.log(errors::parser::EXPECTED_UNTIL.args(*(begin == end ? (end - 1) : begin),
                                                                        context.getSourceInterface(),
                                                                        *(begin == end ? (end - 1) : begin)));
                        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_UNTIL);
                    }
                }
                else
                {
                    context.log(errors::parser::EXPECTED_STEP.args(*(begin == end ? (end - 1) : begin),
                                                                   context.getSourceInterface(),
                                                                   *(begin == end ? (end - 1) : begin)));
                    DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_STEP);
                }
            }
            else
            {
                context.log(errors::parser::EXPECTED_ASSIGNMENT.args(*(begin == end ? (end - 1) : begin),
                                                                     context.getSourceInterface(),
                                                                     *(begin == end ? (end - 1) : begin)));
                DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_ASSIGNMENT);
            }
        }
        else
        {
            context.log(errors::parser::EXPECTED_IDENTIFIER.args(*(begin == end ? (end - 1) : begin),
                                                                 context.getSourceInterface(),
                                                                 *(begin == end ? (end - 1) : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_IDENTIFIER);
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

    if (begin->getTokenType() == lexer::TokenType::BeginKeyword || begin->getTokenType() == lexer::TokenType::SemiColon)
    {
        return nullptr;
    }

    context.log(errors::parser::EXPECTED_STATEMENT.args(
        *(begin == end ? (end - 1) : begin), context.getSourceInterface(), *(begin == end ? (end - 1) : begin)));
    DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_STATEMENT);
    return nullptr;
}
} // namespace coral66::parser
