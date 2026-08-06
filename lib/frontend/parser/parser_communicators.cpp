/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "parser.hpp"

#include "lib/frontend/common/messages/errors.hpp"
#include "parser_support.hpp"
#include "parser_utilities.hpp"

namespace coral66::parser
{
namespace
{
std::optional<std::vector<std::unique_ptr<syntax::Declaration>>>
    parseAnonymousTypeList(lexer::CTokenIterator& begin, const lexer::CTokenIterator end, Context& context)
{
    std::vector<std::unique_ptr<syntax::Declaration>> result;
    if (begin == end || begin->getTokenType() != lexer::TokenType::OpenParentheses)
        return result;

    begin++; // skip '('

    while (begin != end && begin->getTokenType() != lexer::TokenType::CloseParentheses)
    {
        auto startMode = begin;
        const auto type = begin->getTokenType();

        auto mode = syntax::ParameterDeclaration::ParameterMode::Value;
        std::optional<lexer::CTokenIterator> paramTypeToken = std::nullopt;

        if (type == lexer::TokenType::ValueKeyword || type == lexer::TokenType::LocationKeyword)
        {
            mode = type == lexer::TokenType::ValueKeyword ? syntax::ParameterDeclaration::ParameterMode::Value :
                                                            syntax::ParameterDeclaration::ParameterMode::Location;
            begin++;
            paramTypeToken = begin;
            if (begin != end
                && (begin->getTokenType() == lexer::TokenType::IntegerKeyword
                    || begin->getTokenType() == lexer::TokenType::FloatingKeyword
                    || begin->getTokenType() == lexer::TokenType::BitsKeyword
                    || begin->getTokenType() == lexer::TokenType::BitKeyword
                    || begin->getTokenType() == lexer::TokenType::FixedKeyword))
            {
                const auto currentType = begin->getTokenType();
                begin++;
                if (currentType == lexer::TokenType::FixedKeyword)
                {
                    validateFixedFormat(begin, end, context);
                }
            }
        }
        else if (type == lexer::TokenType::LabelKeyword || type == lexer::TokenType::SwitchKeyword)
        {
            mode = type == lexer::TokenType::LabelKeyword ? syntax::ParameterDeclaration::ParameterMode::Label :
                                                            syntax::ParameterDeclaration::ParameterMode::Switch;
            begin++;
        }
        else if (type == lexer::TokenType::IntegerKeyword || type == lexer::TokenType::FloatingKeyword
                 || type == lexer::TokenType::BitsKeyword || type == lexer::TokenType::BitKeyword
                 || type == lexer::TokenType::FixedKeyword || type == lexer::TokenType::ArrayKeyword)
        {
            paramTypeToken = begin;
            if (type != lexer::TokenType::ArrayKeyword)
            {
                begin++;
            }
            if (type == lexer::TokenType::FixedKeyword)
            {
                validateFixedFormat(begin, end, context);
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
            }
            else
            {
                // Number type with no ARRAY or PROCEDURE is technically invalid per Type list Specifier grammar,
                // but we will parse it gracefully as a value mode type without identifiers
            }
        }
        else if (type == lexer::TokenType::ProcedureKeyword)
        {
            mode = syntax::ParameterDeclaration::ParameterMode::Procedure;
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
        }
        else if (type == lexer::TokenType::TableKeyword)
        {
            mode = syntax::ParameterDeclaration::ParameterMode::UntypedLocation;
            begin++;
        }
        else
        {
            begin++;
        }

        result.push_back(std::make_unique<syntax::ParameterDeclaration>(startMode, begin, mode, paramTypeToken,
                                                                        std::vector<lexer::CTokenIterator>()));

        if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
        {
            begin++;
        }
        else
        {
            break;
        }
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
        return std::nullopt;
    }

    return result;
}
} // namespace

std::unique_ptr<syntax::Communicator> parseCommunicator(lexer::CTokenIterator& begin, const lexer::CTokenIterator end,
                                                        Context& context)
{
    if (begin == end)
    {
        return nullptr;
    }

    auto start = begin;

    if (begin->getTokenType() == lexer::TokenType::LibraryKeyword)
    {
        begin++;
        if (begin != end && begin->getTokenType() != lexer::TokenType::OpenParentheses)
        {
            context.log(errors::parser::EXPECTED_OPEN_PARENTHESIS.args(
                *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_OPEN_PARENTHESIS);
            return nullptr;
        }
        begin++;

        std::vector<lexer::CTokenIterator> result;
        while (begin != end && begin->getTokenType() != lexer::TokenType::CloseParentheses)
        {
            if (begin->getTokenType() == lexer::TokenType::Identifier)
            {
                result.push_back(begin);
                begin++;
            }
            else
            {
                context.log(errors::parser::EXPECTED_IDENTIFIER.args(*(begin == end ? end - 1 : begin),
                                                                     context.getSourceInterface(),
                                                                     *(begin == end ? end - 1 : begin)));
                DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_IDENTIFIER);
                return nullptr;
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

        if (begin == end || begin->getTokenType() != lexer::TokenType::CloseParentheses)
        {
            context.log(errors::parser::EXPECTED_CLOSE_PARENTHESIS.args(
                *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_CLOSE_PARENTHESIS);
            return nullptr;
        }

        auto endToken = begin;
        begin++;
        return std::make_unique<syntax::LibraryCommunicator>(start, endToken, std::move(result));
    }

    const bool isCommon = begin->getTokenType() == lexer::TokenType::CommonKeyword;
    const bool isAbsolute = begin->getTokenType() == lexer::TokenType::AbsoluteKeyword;

    if (const bool isExternal = begin->getTokenType() == lexer::TokenType::ExternalKeyword;
        isCommon || isExternal || isAbsolute)
    {
        std::vector<std::unique_ptr<syntax::Declaration>> result;
        std::map<lexer::CTokenIterator, std::unique_ptr<syntax::Expression>> absoluteAddresses;
        begin++;
        if (begin != end && begin->getTokenType() != lexer::TokenType::OpenParentheses)
        {
            context.log(errors::parser::EXPECTED_OPEN_PARENTHESIS.args(
                *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_OPEN_PARENTHESIS);
            return nullptr;
        }

        begin++;

        while (begin != end && begin->getTokenType() != lexer::TokenType::CloseParentheses)
        {
            auto currentStart = begin;

            if (begin->getTokenType() == lexer::TokenType::LabelKeyword
                || begin->getTokenType() == lexer::TokenType::SwitchKeyword)
            {
                begin++;
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
                result.push_back(std::make_unique<syntax::PlaceSpecification>(currentStart, begin, std::move(ids)));
            }
            else
            {
                auto procLookahead = begin;
                bool isProcedure = false;
                if (procLookahead->getTokenType() == lexer::TokenType::ProcedureKeyword)
                {
                    isProcedure = true;
                }
                else if (procLookahead->getTokenType() == lexer::TokenType::IntegerKeyword
                         || procLookahead->getTokenType() == lexer::TokenType::FloatingKeyword
                         || procLookahead->getTokenType() == lexer::TokenType::BitsKeyword
                         || procLookahead->getTokenType() == lexer::TokenType::BitKeyword
                         || procLookahead->getTokenType() == lexer::TokenType::FixedKeyword
                         || procLookahead->getTokenType() == lexer::TokenType::ArrayKeyword)
                {
                    if (procLookahead->getTokenType() != lexer::TokenType::ArrayKeyword)
                    {
                        procLookahead++;
                    }
                    if (procLookahead != end && procLookahead->getTokenType() == lexer::TokenType::ProcedureKeyword)
                    {
                        isProcedure = true;
                    }
                    else if (procLookahead != end && currentStart->getTokenType() == lexer::TokenType::FixedKeyword)
                    {
                        if (procLookahead->getTokenType() == lexer::TokenType::OpenParentheses)
                        {
                            int depth = 1;
                            procLookahead++;
                            while (procLookahead != end && depth > 0)
                            {
                                if (procLookahead->getTokenType() == lexer::TokenType::OpenParentheses)
                                    depth++;
                                else if (procLookahead->getTokenType() == lexer::TokenType::CloseParentheses)
                                    depth--;
                                procLookahead++;
                            }
                        }
                        if (procLookahead != end && procLookahead->getTokenType() == lexer::TokenType::ProcedureKeyword)
                        {
                            isProcedure = true;
                        }
                    }
                }

                if (isProcedure)
                {
                    std::optional<lexer::CTokenIterator> typeToken = std::nullopt;
                    if (begin->getTokenType() == lexer::TokenType::IntegerKeyword
                        || begin->getTokenType() == lexer::TokenType::FloatingKeyword
                        || begin->getTokenType() == lexer::TokenType::BitsKeyword
                        || begin->getTokenType() == lexer::TokenType::BitKeyword
                        || begin->getTokenType() == lexer::TokenType::FixedKeyword
                        || begin->getTokenType() == lexer::TokenType::ArrayKeyword)
                    {
                        typeToken = begin;
                        if (begin->getTokenType() != lexer::TokenType::ArrayKeyword)
                        {
                            begin++;
                        }
                        if (typeToken.value()->getTokenType() == lexer::TokenType::FixedKeyword)
                        {
                            validateFixedFormat(begin, end, context);
                        }
                    }

                    if (begin != end && begin->getTokenType() == lexer::TokenType::ProcedureKeyword)
                    {
                        begin++;
                    }

                    while (begin != end)
                    {
                        auto name = begin;
                        if (begin != end && begin->getTokenType() == lexer::TokenType::Identifier)
                        {
                            begin++;
                        }
                        else
                        {
                            context.log(errors::parser::EXPECTED_IDENTIFIER.args(*(begin == end ? end - 1 : begin),
                                                                                 context.getSourceInterface(),
                                                                                 *(begin == end ? end - 1 : begin)));
                            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_IDENTIFIER);
                            return nullptr;
                        }

                        auto parametersOpt = parseAnonymousTypeList(begin, end, context);
                        if (!parametersOpt.has_value())
                        {
                            return nullptr;
                        }

                        result.push_back(std::make_unique<syntax::ProcedureSpecification>(
                            currentStart, begin, typeToken.value_or(&lexer::noneCToken), name,
                            std::move(parametersOpt.value())));

                        if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
                        {
                            begin++;
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                else
                {
                    if (auto declarations =
                            parseDeclaration(begin, end, context, isAbsolute ? &absoluteAddresses : nullptr);
                        !declarations.empty())
                    {
                        for (auto& d : declarations)
                        {
                            result.push_back(std::move(d));
                        }
                    }
                }
            }

            if (begin != end && begin->getTokenType() == lexer::TokenType::SemiColon)
            {
                begin++;
            }
            else if (begin != end && begin->getTokenType() != lexer::TokenType::CloseParentheses)
            {
                context.log(errors::parser::EXPECTED_N.args(*(begin == end ? end - 1 : begin),
                                                            context.getSourceInterface(), lexer::TokenType::SemiColon,
                                                            *(begin == end ? end - 1 : begin)));
                DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_N);
                return nullptr;
            }
        }

        if (begin == end || begin->getTokenType() != lexer::TokenType::CloseParentheses)
        {
            context.log(errors::parser::EXPECTED_CLOSE_PARENTHESIS.args(
                *(begin == end ? end - 1 : begin), context.getSourceInterface(), *(begin == end ? end - 1 : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_CLOSE_PARENTHESIS);
            return nullptr;
        }

        auto endToken = begin;
        begin++;

        if (isCommon)
            return std::make_unique<syntax::CommonCommunicator>(start, endToken, std::move(result));
        if (isAbsolute)
            return std::make_unique<syntax::AbsoluteCommunicator>(start, endToken, std::move(result),
                                                                  std::move(absoluteAddresses));
        return std::make_unique<syntax::ExternalCommunicator>(start, endToken, std::move(result));
    }

    return nullptr;
}
} // namespace coral66::parser
