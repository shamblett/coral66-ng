/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 23/03/2026
 */

#include "parser.hpp"

#include "lib/frontend/common/messages/errors.hpp"
#include "parser_support.hpp"

namespace coral66::parser
{
/*
 * Section 5
 *
 * Coral 66 place references comprise  :-
 *
 * Switches
 *
 */

std::unique_ptr<Syntax::PlaceReference> parsePlaceReference(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                            Context& context)
{
    if (begin == end)
    {
        return nullptr;
    }

    if (begin->getTokenType() == lexer::TokenType::SwitchKeyword)
    {
        auto start = begin;
        begin++;
        if (begin != end && begin->getTokenType() == lexer::TokenType::Identifier)
        {
            std::vector<lexer::CTokenIterator> labels;
            auto name = begin;
            begin++;
            if (begin != end && begin->getTokenType() == lexer::TokenType::SemiColon)
            {
                begin++;
            }
            while (begin != end)
            {
                if (begin != end && begin->getTokenType() == lexer::TokenType::Identifier)
                {
                    auto label = begin;
                    labels.emplace_back(label);
                    begin++;

                    if (begin != end && begin->getTokenType() != lexer::TokenType::Comma)
                    {
                        context.log(errors::parser::EXPECTED_COMMA.args(*(begin == end ? (end - 1) : begin),
                                                                        context.getSourceInterface(),
                                                                        *(begin == end ? (end - 1) : begin)));
                        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_COMMA);
                        return nullptr;
                    }
                    if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
                    {
                        begin++;
                    }
                }
                else
                {
                    context.log(errors::parser::EXPECTED_IDENTIFIER.args(*(begin == end ? (end - 1) : begin),
                                                                         context.getSourceInterface(),
                                                                         *(begin == end ? (end - 1) : begin)));
                    DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_IDENTIFIER);
                    return nullptr;
                }
            }
            if (labels.empty())
            {
                context.log(errors::parser::EXPECTED_LABELS.args(*(begin == end ? (end - 1) : begin),
                                                                 context.getSourceInterface(),
                                                                 *(begin == end ? (end - 1) : begin)));
                DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_LABELS);
                return nullptr;
            }
            return std::make_unique<Syntax::SwitchPlaceReference>(start, begin, name, labels);
        }
        context.log(errors::parser::EXPECTED_IDENTIFIER.args(
            *(begin == end ? (end - 1) : begin), context.getSourceInterface(), *(begin == end ? (end - 1) : begin)));
        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_IDENTIFIER);
        return nullptr;
    }
    return nullptr;
}
} // namespace coral66::parser
