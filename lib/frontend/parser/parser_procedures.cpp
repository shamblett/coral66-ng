/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 30/03/2026
 */

#include "parser.hpp"

#include <algorithm>

#include "lib/frontend/common/messages/errors.hpp"
#include "parser_support.hpp"
#include "parser_utilities.hpp"

namespace coral66::parser
{
/*
 * Section 8
 *
 * Procedures
 *
 */

std::optional<std::vector<std::unique_ptr<syntax::Declaration>>>
    parseProcedureParameters(lexer::CTokenIterator& begin, const lexer::CTokenIterator end, Context& context)
{
    std::vector<std::unique_ptr<syntax::Declaration>> parameters;
    if (begin != end && begin->getTokenType() == lexer::TokenType::OpenParentheses)
    {
        begin++;
        while (begin != end && begin->getTokenType() != lexer::TokenType::CloseParentheses)
        {
            if (const auto type = begin->getTokenType();
                type == lexer::TokenType::ValueKeyword || type == lexer::TokenType::LocationKeyword)
            {
                auto startMode = begin;
                begin++;
                auto mode = type == lexer::TokenType::ValueKeyword ?
                                syntax::ParameterDeclaration::ParameterMode::Value :
                                syntax::ParameterDeclaration::ParameterMode::Location;

                if (const auto nextType = begin->getTokenType();
                    nextType == lexer::TokenType::IntegerKeyword || nextType == lexer::TokenType::FloatingKeyword
                    || nextType == lexer::TokenType::BitsKeyword || nextType == lexer::TokenType::BitKeyword
                    || nextType == lexer::TokenType::FixedKeyword)
                {
                    auto paramTypeToken = begin;
                    begin++;

                    std::pair<std::unique_ptr<syntax::Expression>, std::unique_ptr<syntax::Expression>> fixedScale;
                    if (nextType == lexer::TokenType::FixedKeyword)
                    {
                        fixedScale = parseFixedFormat(begin, end, context);
                    }

                    if (begin != end && begin->getTokenType() == lexer::TokenType::ArrayKeyword)
                    {
                        mode = syntax::ParameterDeclaration::ParameterMode::Array;
                        begin++;
                    }
                    else if (begin != end && begin->getTokenType() == lexer::TokenType::ProcedureKeyword)
                    {
                        mode = syntax::ParameterDeclaration::ParameterMode::Procedure;
                        begin++;
                    }

                    std::vector<lexer::CTokenIterator> ids;
                    while (begin != end && begin->getTokenType() == lexer::TokenType::Identifier)
                    {
                        ids.push_back(begin);
                        begin++;
                        if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
                        {
                            begin++;
                        }
                        else
                        {
                            break;
                        }
                    }
                    parameters.push_back(std::make_unique<syntax::ParameterDeclaration>(
                        startMode, begin, mode, paramTypeToken, std::move(ids), std::move(fixedScale.first),
                        std::move(fixedScale.second)));
                }
                else if (nextType == lexer::TokenType::Identifier)
                {
                    auto untypedMode = type == lexer::TokenType::ValueKeyword ?
                                           syntax::ParameterDeclaration::ParameterMode::UntypedValue :
                                           syntax::ParameterDeclaration::ParameterMode::UntypedLocation;
                    std::vector<lexer::CTokenIterator> ids;
                    while (begin != end && begin->getTokenType() == lexer::TokenType::Identifier)
                    {
                        ids.push_back(begin);
                        begin++;
                        if (begin != end && begin->getTokenType() == lexer::TokenType::Colon)
                        {
                            begin++;
                            if (begin != end && begin->getTokenType() == lexer::TokenType::Identifier)
                            {
                                ids.push_back(begin);
                                begin++;
                            }
                        }
                        if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
                        {
                            begin++;
                        }
                        else
                        {
                            break;
                        }
                    }
                    parameters.push_back(std::make_unique<syntax::ParameterDeclaration>(startMode, begin, untypedMode,
                                                                                        std::nullopt, std::move(ids)));
                }
                else
                {
                    begin++;
                }
            }
            else if (type == lexer::TokenType::LabelKeyword || type == lexer::TokenType::SwitchKeyword)
            {
                auto startMode = begin;
                begin++;
                auto mode = type == lexer::TokenType::LabelKeyword ?
                                syntax::ParameterDeclaration::ParameterMode::Label :
                                syntax::ParameterDeclaration::ParameterMode::Switch;
                std::vector<lexer::CTokenIterator> ids;
                while (begin != end && begin->getTokenType() == lexer::TokenType::Identifier)
                {
                    ids.push_back(begin);
                    begin++;
                    if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
                    {
                        begin++;
                    }
                    else
                    {
                        break;
                    }
                }
                parameters.push_back(std::make_unique<syntax::ParameterDeclaration>(startMode, begin, mode,
                                                                                    std::nullopt, std::move(ids)));
            }
            else if (type == lexer::TokenType::ProcedureKeyword)
            {
                auto startMode = begin;
                begin++;
                std::vector<lexer::CTokenIterator> ids;
                while (begin != end && begin->getTokenType() == lexer::TokenType::Identifier)
                {
                    ids.push_back(begin);
                    begin++;
                    if (begin != end && begin->getTokenType() == lexer::TokenType::OpenParentheses)
                    {
                        int depth = 1;
                        begin++;
                        while (begin != end && depth > 0)
                        {
                            if (begin->getTokenType() == lexer::TokenType::OpenParentheses)
                                depth++;
                            else if (begin->getTokenType() == lexer::TokenType::CloseParentheses)
                                depth--;
                            begin++;
                        }
                    }
                    if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
                    {
                        begin++;
                    }
                    else
                    {
                        break;
                    }
                }
                parameters.push_back(std::make_unique<syntax::ParameterDeclaration>(
                    startMode, begin, syntax::ParameterDeclaration::ParameterMode::Procedure, std::nullopt,
                    std::move(ids)));
            }
            else if (type == lexer::TokenType::IntegerKeyword || type == lexer::TokenType::FloatingKeyword
                     || type == lexer::TokenType::BitsKeyword || type == lexer::TokenType::BitKeyword
                     || type == lexer::TokenType::FixedKeyword)
            {
                if (auto declarations = parseDeclaration(begin, end, context); !declarations.empty())
                {
                    for (auto& d : declarations)
                        parameters.push_back(std::move(d));
                }
                else
                {
                    context.log(errors::parser::EXPECTED_DECLARATION.args(*(begin == end ? end - 1 : begin),
                                                                          context.getSourceInterface(),
                                                                          *(begin == end ? end - 1 : begin)));
                    DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_DECLARATION);
                    return std::nullopt;
                }
            }
            else
            {
                if (auto declarations = parseDeclaration(begin, end, context); !declarations.empty())
                {
                    for (auto& d : declarations)
                        parameters.push_back(std::move(d));
                }
                else
                {
                    context.log(errors::parser::EXPECTED_DECLARATION.args(*(begin == end ? end - 1 : begin),
                                                                          context.getSourceInterface(),
                                                                          *(begin == end ? end - 1 : begin)));
                    DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_DECLARATION);
                    return std::nullopt;
                }
            }

            if (begin != end && begin->getTokenType() == lexer::TokenType::SemiColon)
                begin++; // NOLINT(*-branch-clone)
            else if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
                begin++;
            else if (begin != end && begin->getTokenType() != lexer::TokenType::CloseParentheses)
                begin++;
        }
        if (begin != end)
            begin++;
    }
    return parameters;
}

std::unique_ptr<syntax::ProcedureDeclaration> parseProcedure(lexer::CTokenIterator& begin,
                                                             const lexer::CTokenIterator end, Context& context)
{
    if (begin == end)
    {
        return nullptr;
    }

    const auto rollback = begin;
    std::optional<lexer::CTokenIterator> typeToken = std::nullopt;
    bool isTyped = false;

    if (begin != end
        && (begin->getTokenType() == lexer::TokenType::IntegerKeyword
            || begin->getTokenType() == lexer::TokenType::FloatingKeyword
            || begin->getTokenType() == lexer::TokenType::BitsKeyword
            || begin->getTokenType() == lexer::TokenType::BitKeyword
            || begin->getTokenType() == lexer::TokenType::FixedKeyword))
    {
        typeToken = begin;
        isTyped = true;
        begin++;
        if (typeToken.value()->getTokenType() == lexer::TokenType::FixedKeyword)
        {
            validateFixedFormat(begin, end, context);
        }
    }

    bool isRecursive = false;
    if (begin != end && begin->getTokenType() == lexer::TokenType::RecursiveKeyword)
    {
        isRecursive = true;
        begin++;
    }

    bool hasProcedureKeyword = false;
    if (begin != end && begin->getTokenType() == lexer::TokenType::ProcedureKeyword)
    {
        hasProcedureKeyword = true;
        begin++;
    }

    if (isRecursive || hasProcedureKeyword)
    {
        auto start = typeToken.has_value() ? typeToken.value() : isRecursive ? rollback : begin - 1;

        auto name = begin == end ? end - 1 : begin;
        if (begin != end && begin->getTokenType() == lexer::TokenType::Identifier)
        {
            begin++;
        }
        else
        {
            context.log(errors::parser::EXPECTED_IDENTIFIER.args(*start, context.getSourceInterface(), *start));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_IDENTIFIER);
        }

        {
            auto parametersOpt = parseProcedureParameters(begin, end, context);
            if (!parametersOpt.has_value())
            {
                return nullptr;
            }
            std::vector<std::unique_ptr<syntax::Declaration>> parameters = std::move(parametersOpt.value());

            if (begin != end && begin->getTokenType() == lexer::TokenType::SemiColon)
            {
                begin++;
            }

            context.enterProcedure(isTyped);

            if (auto body = parseStatement(begin, end, context))
            {
                if (isTyped && !context.isAnswerStatementFound())
                {
                    context.log(
                        errors::parser::MISSING_ANSWER_STATEMENT.args(*start, context.getSourceInterface(), *start));
                    DiagnosticCollector::addDiagnostic(errors::parser::MISSING_ANSWER_STATEMENT);
                }
                auto result = std::make_unique<syntax::ProcedureDeclaration>(start, begin, name, std::move(parameters),
                                                                             std::move(body), typeToken, isRecursive);
                context.exitProcedure();
                return result;
            }
            context.log(errors::parser::EXPECTED_STATEMENT.args(*start, context.getSourceInterface(), *start));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_STATEMENT);
            auto result = std::make_unique<syntax::ProcedureDeclaration>(
                start, begin, name, std::move(parameters), std::make_unique<syntax::DummyStatement>(begin, begin),
                typeToken, isRecursive);
            context.exitProcedure();
            return result;
        }
    }

    begin = rollback;
    return nullptr;
}
} // namespace coral66::parser
