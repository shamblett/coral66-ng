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
/*
 * Section 9
 *
 * Coral 66 communicators comprise  :-
 *
 * Common
 *
 * Library
 *
 * External
 *
 * Absolute
 *
 */

std::unique_ptr<Syntax::Communicator> parseCommunicator(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                        Context& context)
{
    if (begin == end)
    {
        return nullptr;
    }

    auto start = begin;

    if (begin->getTokenType() == lexer::TokenType::CommonKeyword)
    {
        std::vector<std::unique_ptr<Syntax::Declaration>> result;
        begin++;
        if (begin != end && begin->getTokenType() != lexer::TokenType::OpenParentheses)
        {
            context.log(errors::parser::EXPECTED_OPEN_PARENTHESIS.args(*(begin == end ? (end - 1) : begin),
                                                                       context.getSourceInterface(),
                                                                       *(begin == end ? (end - 1) : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_OPEN_PARENTHESIS);
            return nullptr;
        }

        begin++;

        while (begin != end && begin->getTokenType() != lexer::TokenType::CloseParentheses)
        {
            auto currentStart = begin;
            
            if (begin->getTokenType() == lexer::TokenType::LabelKeyword)
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
                result.push_back(std::make_unique<Syntax::PlaceSpecification>(currentStart, begin, std::move(ids)));
            }
            else
            {
                auto procLookahead = begin;
                bool isProcedure = false;
                if (procLookahead->getTokenType() == lexer::TokenType::ProcedureKeyword)
                {
                    isProcedure = true;
                }
                else if (procLookahead->getTokenType() == lexer::TokenType::IntegerKeyword || 
                         procLookahead->getTokenType() == lexer::TokenType::FloatingKeyword || 
                         procLookahead->getTokenType() == lexer::TokenType::FixedKeyword)
                {
                    procLookahead++;
                    if (procLookahead != end && procLookahead->getTokenType() == lexer::TokenType::ProcedureKeyword)
                    {
                        isProcedure = true;
                    }
                    else if (procLookahead != end && (currentStart->getTokenType() == lexer::TokenType::FixedKeyword))
                    {
                        if (procLookahead->getTokenType() == lexer::TokenType::OpenParentheses)
                        {
                            int depth = 1;
                            procLookahead++;
                            while (procLookahead != end && depth > 0)
                            {
                                if (procLookahead->getTokenType() == lexer::TokenType::OpenParentheses) depth++;
                                else if (procLookahead->getTokenType() == lexer::TokenType::CloseParentheses) depth--;
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
                    if (begin->getTokenType() == lexer::TokenType::IntegerKeyword ||
                        begin->getTokenType() == lexer::TokenType::FloatingKeyword ||
                        begin->getTokenType() == lexer::TokenType::FixedKeyword)
                    {
                        typeToken = begin;
                        begin++;
                        if (typeToken.value()->getTokenType() == lexer::TokenType::FixedKeyword)
                        {
                            validateFixedFormat(begin, end, context);
                        }
                    }
                    
                    if (begin != end && begin->getTokenType() == lexer::TokenType::ProcedureKeyword)
                    {
                        begin++;
                    }
                    
                    auto name = begin;
                    if (begin != end && begin->getTokenType() == lexer::TokenType::Identifier)
                    {
                        begin++;
                    }
                    else
                    {
                        context.log(errors::parser::EXPECTED_IDENTIFIER.args(*(begin == end ? (end - 1) : begin),
                                                                             context.getSourceInterface(),
                                                                             *(begin == end ? (end - 1) : begin)));
                        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_IDENTIFIER);
                        return nullptr;
                    }
                    
                    auto parametersOpt = parseProcedureParameters(begin, end, context);
                    if (!parametersOpt.has_value())
                    {
                        return nullptr;
                    }
                    
                    result.push_back(std::make_unique<Syntax::ProcedureSpecification>(currentStart, begin, typeToken, name, std::move(parametersOpt.value())));
                }
                else
                {
                    auto declarations = parseDeclaration(begin, end, context);
                    if (!declarations.empty())
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
                context.log(errors::parser::EXPECTED_N.args(*(begin == end ? (end - 1) : begin),
                                                            context.getSourceInterface(),
                                                            lexer::TokenType::SemiColon, *(begin == end ? (end - 1) : begin)));
                DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_N);
                return nullptr;
            }
        }

        if (begin == end || begin->getTokenType() != lexer::TokenType::CloseParentheses)
        {
            context.log(errors::parser::EXPECTED_CLOSE_PARENTHESIS.args(*(begin == end ? (end - 1) : begin),
                                                                        context.getSourceInterface(),
                                                                        *(begin == end ? (end - 1) : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_CLOSE_PARENTHESIS);
            return nullptr;
        }
        
        auto endToken = begin;
        begin++;

        return std::make_unique<Syntax::CommonCommunicator>(start, endToken, std::move(result));
    }

    return nullptr;
}
} // namespace coral66::parser
