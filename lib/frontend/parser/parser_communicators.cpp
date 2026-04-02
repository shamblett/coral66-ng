/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "parser.hpp"

#include "lib/frontend/common/messages/errors.hpp"
#include "parser_support.hpp"

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

    std::vector<lexer::CTokenIterator> result;
    if (begin->getTokenType() == lexer::TokenType::CommonKeyword)
    {
        begin++;
        if (begin != end && begin->getTokenType() != lexer::TokenType::OpenParentheses)
        {
            context.log(errors::parser::EXPECTED_OPEN_PARENTHESIS.args(*(begin == end ? (end - 1) : begin),
                                                                       context.getSourceInterface(),
                                                                       *(begin == end ? (end - 1) : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_OPEN_PARENTHESIS);
            return nullptr;
        }

        // TODO process common communicator

        begin++;
        if (begin != end && begin->getTokenType() != lexer::TokenType::CloseParentheses)
        {
            context.log(errors::parser::EXPECTED_CLOSE_PARENTHESIS.args(*(begin == end ? (end - 1) : begin),
                                                                        context.getSourceInterface(),
                                                                        *(begin == end ? (end - 1) : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_CLOSE_PARENTHESIS);
            return nullptr;
        }

        return std::make_unique<Syntax::CommonCommunicator>(start, begin, result);
    }

    return nullptr;
}
} // namespace coral66::parser
