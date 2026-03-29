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
 * Compound + Blocks
 *
 * Dummy
 *
 * Conditional
 *
 * For
 */

static bool isAssignmentStatement(lexer::CTokenIterator begin, lexer::CTokenIterator end)
{
    int parens = 0;
    int brackets = 0;
    for (auto it = begin; it != end; ++it)
    {
        if (it->getTokenType() == lexer::TokenType::OpenParentheses)
            parens++;
        else if (it->getTokenType() == lexer::TokenType::CloseParentheses)
            parens--;
        else if (it->getTokenType() == lexer::TokenType::OpenSquareBracket)
            brackets++;
        else if (it->getTokenType() == lexer::TokenType::CloseSquareBracket)
            brackets--;
        else if (parens == 0 && brackets == 0)
        {
            if (it->getTokenType() == lexer::TokenType::Assignment)
                return true;
            if (it->getTokenType() == lexer::TokenType::SemiColon || it->getTokenType() == lexer::TokenType::ThenKeyword
                || it->getTokenType() == lexer::TokenType::ElseKeyword
                || it->getTokenType() == lexer::TokenType::DoKeyword
                || it->getTokenType() == lexer::TokenType::EndKeyword || it->getTokenType() == lexer::TokenType::Comma)
            {
                return false;
            }
        }
    }
    return false;
}

static std::unique_ptr<Syntax::Statement> parseStatementImpl(lexer::CTokenIterator& begin, const lexer::CTokenIterator end,
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
        if (isAssignmentStatement(begin, end))
        {
            if (auto var = parseVariable(begin, end, context))
            {
                if (begin != end && begin->getTokenType() == lexer::TokenType::Assignment)
                {
                    begin++;
                    auto rhs = parseExpression(begin, end, context);
                    if (rhs)
                    {
                        return std::make_unique<Syntax::AssignmentStatement>(start, begin, std::move(var),
                                                                             std::move(rhs));
                    }
                }
            }
            context.log(errors::parser::EXPECTED_ASSIGNMENT.args(*(begin == end ? (end - 1) : begin),
                                                                 context.getSourceInterface(),
                                                                 *(begin == end ? (end - 1) : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_ASSIGNMENT);
            return nullptr;
        }
        else
        {
            if (begin->getTokenType() != lexer::TokenType::Identifier)
            {
                context.log(errors::parser::EXPECTED_ASSIGNMENT.args(*(begin == end ? (end - 1) : begin),
                                                                     context.getSourceInterface(),
                                                                     *(begin == end ? (end - 1) : begin)));
                DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_ASSIGNMENT);
                return nullptr;
            }

            if (auto expr = parseExpression(begin, end, context))
            {
                if (!dynamic_cast<Syntax::IdentifierExpression*>(expr.get())
                    && !dynamic_cast<Syntax::FunctionCallExpression*>(expr.get()))
                {
                    context.log(errors::parser::EXPECTED_PROCEDURE_CALL.args(*(begin == end ? (end - 1) : begin),
                                                                             context.getSourceInterface(),
                                                                             *(begin == end ? (end - 1) : begin)));
                    DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_PROCEDURE_CALL);
                    return nullptr;
                }
                return std::make_unique<Syntax::ProcedureCallStatement>(start, begin, std::move(expr));
            }
            return nullptr;
        }
    }
    if (begin->getTokenType() == lexer::TokenType::GotoKeyword)
    {
        auto start = begin;
        begin++;
        if (begin != end && begin->getTokenType() == lexer::TokenType::Identifier)
        {
            auto label = begin;
            begin++;
            std::unique_ptr<Syntax::Expression> index = nullptr;
            if (begin != end && begin->getTokenType() == lexer::TokenType::OpenSquareBracket)
            {
                begin++;
                index = parseExpression(begin, end, context);
                if (begin != end && begin->getTokenType() == lexer::TokenType::CloseSquareBracket)
                {
                    begin++;
                }
                else
                {
                    context.log(errors::parser::EXPECTED_CLOSE_SQUARE_BRACKET.args(
                        *(begin == end ? (end - 1) : begin), context.getSourceInterface(),
                        *(begin == end ? (end - 1) : begin)));
                    DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_CLOSE_SQUARE_BRACKET);
                    return nullptr;
                }
            }
            return std::make_unique<Syntax::GotoStatement>(start, begin, label, std::move(index));
        }
        context.log(errors::parser::EXPECTED_IDENTIFIER.args(
            *(begin == end ? (end - 1) : begin), context.getSourceInterface(), *(begin == end ? (end - 1) : begin)));
        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_IDENTIFIER);
        return nullptr;
    }

    if (begin->getTokenType() == lexer::TokenType::AnswerKeyword)
    {
        auto start = begin;
        begin++;
        auto expr = parseExpression(begin, end, context);
        
        if (!context.isInsideProcedure()) {
            context.log(errors::parser::ANSWER_OUTSIDE_PROCEDURE.args(
                *start, context.getSourceInterface(), *start));
            DiagnosticCollector::addDiagnostic(errors::parser::ANSWER_OUTSIDE_PROCEDURE);
            return nullptr;
        }
        
        if (context.isInsideUntypedProcedure()) {
            context.log(errors::parser::ANSWER_IN_UNTYPED_PROCEDURE.args(
                *start, context.getSourceInterface(), *start));
            DiagnosticCollector::addDiagnostic(errors::parser::ANSWER_IN_UNTYPED_PROCEDURE);
            return nullptr;
        }

        context.markAnswerStatementFound();

        if (expr)
        {
            return std::make_unique<Syntax::AnswerStatement>(start, begin, std::move(expr));
        }
        context.log(errors::parser::EXPECTED_EXPRESSION.args(
            *(begin == end ? (end - 1) : begin), context.getSourceInterface(), *(begin == end ? (end - 1) : begin)));
        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_EXPRESSION);
        return nullptr;
    }

    if (begin->getTokenType() == lexer::TokenType::CodeKeyword)
    {
        begin++;
        if (begin != end && begin->getTokenType() == lexer::TokenType::BeginKeyword)
        {
            begin++;
            if (begin != end && begin->getTokenType() == lexer::TokenType::StringLiteral)
            {
                auto start = begin;
                begin++;
                if (begin != end && begin->getTokenType() != lexer::TokenType::EndKeyword)
                {
                    context.log(errors::parser::EXPECTED_END.args(*(begin == end ? (end - 1) : begin),
                                                                  context.getSourceInterface(),
                                                                  *(begin == end ? (end - 1) : begin)));
                    DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_END);
                    return nullptr;
                }
                return std::make_unique<Syntax::CodeStatement>(start, begin);
            }
            if (begin != end && begin->getTokenType() != lexer::TokenType::StringLiteral)
            {
                context.log(errors::parser::EXPECTED_STRING_LITERAL.args(*(begin == end ? (end - 1) : begin),
                                                                         context.getSourceInterface(),
                                                                         *(begin == end ? (end - 1) : begin)));
                DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_STRING_LITERAL);
                return nullptr;
            }
        }
        else
        {
            {
                context.log(errors::parser::EXPECTED_BEGIN.args(*(begin == end ? (end - 1) : begin),
                                                                context.getSourceInterface(),
                                                                *(begin == end ? (end - 1) : begin)));
                DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_BEGIN);
                return nullptr;
            }
        }
    }
    if (begin->getTokenType() == lexer::TokenType::IfKeyword)
    {
        auto start = begin;
        begin++;
        auto cond = parseCondition(begin, end, context);
        if (begin != end && begin->getTokenType() == lexer::TokenType::ThenKeyword)
        {
            begin++;
            auto thenBranch = parseStatement(begin, end, context);
            std::unique_ptr<Syntax::Statement> elseBranch = nullptr;
            if (begin != end && begin->getTokenType() == lexer::TokenType::ElseKeyword)
            {
                begin++;
                elseBranch = parseStatement(begin, end, context);
                if (!elseBranch)
                {
                    context.log(errors::parser::EXPECTED_STATEMENT.args(*(begin == end ? (end - 1) : begin),
                                                                        context.getSourceInterface(),
                                                                        *(begin == end ? (end - 1) : begin)));
                    DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_STATEMENT);
                    return nullptr;
                }
            }
            if (!cond)
            {
                context.log(errors::parser::EXPECTED_CONDITION.args(
                    *(begin == end ? (end - 1) : begin), context.getSourceInterface(), lexer::TokenType::IfKeyword,
                    *(begin == end ? (end - 1) : begin)));
                DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_CONDITION);
                return nullptr;
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

    if (begin->getTokenType() == lexer::TokenType::ForKeyword)
    {
        auto start = begin;
        begin++;
        auto var = parseWordReference(begin, end, context);
        if (var)
        {
            if (begin != end && begin->getTokenType() == lexer::TokenType::Assignment)
            {
                begin++;
                std::vector<Syntax::ForElement> elements;
                bool parsingElements = true;
                bool errorOccurred = false;

                while (parsingElements)
                {
                    Syntax::ForElement element;
                    element.expression = parseExpression(begin, end, context);
                    if (!element.expression)
                    {
                        errorOccurred = true;
                        break;
                    }

                    if (begin != end && begin->getTokenType() == lexer::TokenType::StepKeyword)
                    {
                        begin++;
                        element.step = parseExpression(begin, end, context);
                        if (!element.step)
                        {
                            errorOccurred = true;
                            break;
                        }

                        if (begin != end && begin->getTokenType() == lexer::TokenType::UntilKeyword)
                        {
                            begin++;
                            element.until = parseExpression(begin, end, context);
                            if (!element.until)
                            {
                                errorOccurred = true;
                                break;
                            }
                        }
                        else
                        {
                            context.log(errors::parser::EXPECTED_UNTIL.args(*(begin == end ? (end - 1) : begin),
                                                                            context.getSourceInterface(),
                                                                            *(begin == end ? (end - 1) : begin)));
                            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_UNTIL);
                            errorOccurred = true;
                            break;
                        }
                    }
                    else if (begin != end && begin->getTokenType() == lexer::TokenType::WhileKeyword)
                    {
                        begin++;
                        element.whileCondition = parseCondition(begin, end, context);
                        if (!element.whileCondition)
                        {
                            errorOccurred = true;
                            break;
                        }
                    }

                    elements.push_back(std::move(element));

                    if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
                    {
                        begin++;
                    }
                    else
                    {
                        parsingElements = false;
                    }
                }

                if (!errorOccurred && begin != end && begin->getTokenType() == lexer::TokenType::DoKeyword)
                {
                    begin++;
                    if (auto body = parseStatement(begin, end, context); body && !elements.empty())
                    {
                        return std::make_unique<Syntax::ForStatement>(start, begin, std::move(var), std::move(elements),
                                                                      std::move(body));
                    }
                }
                else if (!errorOccurred)
                {
                    context.log(errors::parser::EXPECTED_DO.args(*(begin == end ? (end - 1) : begin),
                                                                 context.getSourceInterface(),
                                                                 *(begin == end ? (end - 1) : begin)));
                    DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_DO);
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
            context.log(errors::parser::EXPECTED_EXPRESSION.args(*(begin == end ? (end - 1) : begin),
                                                                 context.getSourceInterface(),
                                                                 *(begin == end ? (end - 1) : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_EXPRESSION);
        }
        return nullptr;
    }

    if (begin->getTokenType() == lexer::TokenType::SemiColon ||
        begin->getTokenType() == lexer::TokenType::EndKeyword ||
        begin->getTokenType() == lexer::TokenType::ElseKeyword)
    {
        return std::make_unique<Syntax::DummyStatement>(begin, begin);
    }

    if (begin->getTokenType() == lexer::TokenType::BeginKeyword)
    {
        auto blockStart = begin;
        if (auto block = parseBlock(begin, end, context))
        {
            if (block->getDeclarations().empty())
            {
                return std::make_unique<Syntax::CompoundStatement>(blockStart, begin, block->takeStatements());
            }
            return std::make_unique<Syntax::BlockStatement>(blockStart, begin, std::move(block));
        }
        return nullptr;
    }

    context.log(errors::parser::EXPECTED_STATEMENT.args(
        *(begin == end ? (end - 1) : begin), context.getSourceInterface(), *(begin == end ? (end - 1) : begin)));
    DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_STATEMENT);
    return nullptr;
}

std::unique_ptr<Syntax::Statement> parseStatement(lexer::CTokenIterator& begin, const lexer::CTokenIterator end,
                                                  Context& context)
{
    if (begin == end)
    {
        return nullptr;
    }

    std::vector<lexer::CTokenIterator> labels;
    while (begin != end && begin->getTokenType() == lexer::TokenType::Identifier)
    {
        auto next = begin + 1;
        if (next != end && next->getTokenType() == lexer::TokenType::Colon)
        {
            labels.push_back(begin);
            begin += 2;
        }
        else
        {
            break;
        }
    }

    auto stmt = parseStatementImpl(begin, end, context);
    if (stmt)
    {
        stmt->setLabels(std::move(labels));
    }
    return stmt;
}
} // namespace coral66::parser

