/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "parser.hpp"

#include "lib/frontend/common/messages/errors.hpp"
#include "lib/frontend/common/source_object.hpp"
#include "parser_support.hpp"

/*
 * Section 4
 *
 * Coral 66 declarations comprise  :-
 *
 * Numeric types
 *
 * Simple References
 *
 * Array References
 *
 * Packed Data
 *
 * Storage Allocation
 *
 * Presets
 *
 * Overlay
 *
 */

namespace coral66::parser
{
std::vector<std::unique_ptr<Syntax::Declaration>> parseDeclaration(lexer::CTokenIterator& begin,
                                                                   lexer::CTokenIterator end, Context& context)
{
    std::vector<std::unique_ptr<Syntax::Declaration>> result;
    if (begin == end)
    {
        return result;
    }
    auto type = begin->getTokenType();
    if (type == lexer::TokenType::IntegerKeyword || type == lexer::TokenType::FloatingKeyword
        || type == lexer::TokenType::FixedKeyword)
    {
        auto typeToken = begin;
        begin++;

        if (type == lexer::TokenType::FixedKeyword && begin != end
            && begin->getTokenType() == lexer::TokenType::OpenParentheses)
        {
            begin++;
            while (begin != end && begin->getTokenType() != lexer::TokenType::CloseParentheses)
                begin++;
            if (begin != end)
                begin++;
        }

        if (begin != end && begin->getTokenType() == lexer::TokenType::ArrayKeyword)
        {
            begin++; // Consume ARRAY
            while (begin != end && begin->getTokenType() == lexer::TokenType::Identifier)
            {
                auto id = begin;
                begin++;
                std::vector<std::pair<std::unique_ptr<Syntax::Expression>, std::unique_ptr<Syntax::Expression>>> dims;
                if (begin != end && begin->getTokenType() == lexer::TokenType::OpenSquareBracket)
                {
                    begin++;
                    while (begin != end && begin->getTokenType() != lexer::TokenType::CloseSquareBracket)
                    {
                        auto lower = parseExpression(begin, end, context);
                        if (begin != end && begin->getTokenType() == lexer::TokenType::Colon)
                        {
                            begin++;
                            auto upper = parseExpression(begin, end, context);
                            dims.emplace_back(std::move(lower), std::move(upper));
                        }
                        if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
                            begin++;
                        else
                            break;
                    }
                    if (begin != end)
                        begin++;
                }
                result.push_back(
                    std::make_unique<Syntax::ArrayDeclaration>(typeToken, begin, typeToken, id, std::move(dims)));

                if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
                {
                    if (begin + 1 != end && (begin + 1)->getTokenType() == lexer::TokenType::Identifier)
                        begin++;
                    else
                        break;
                }
                else
                    break;
            }
        }
        else
        {
            std::vector<lexer::CTokenIterator> identifiers;
            while (begin != end && begin->getTokenType() == lexer::TokenType::Identifier)
            {
                identifiers.push_back(begin);
                begin++;
                if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
                {
                    if (begin + 1 != end && (begin + 1)->getTokenType() == lexer::TokenType::Identifier)
                    {
                        begin++;
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
            if (!identifiers.empty())
                result.push_back(
                    std::make_unique<Syntax::VariableDeclaration>(typeToken, begin, typeToken, std::move(identifiers)));
        }
        return result;
    }

    if (type == lexer::TokenType::TableKeyword)
    {
        auto start = begin;
        begin++;
        if (begin != end && begin->getTokenType() == lexer::TokenType::Identifier)
        {
            auto id = begin;
            begin++;
            std::unique_ptr<Syntax::Expression> width, length;
            if (begin != end && begin->getTokenType() == lexer::TokenType::OpenSquareBracket)
            {
                begin++;
                width = parseExpression(begin, end, context);
                if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
                {
                    begin++;
                    length = parseExpression(begin, end, context);
                }
                if (begin != end && begin->getTokenType() == lexer::TokenType::CloseSquareBracket)
                    begin++;
            }

            std::vector<std::unique_ptr<Syntax::Declaration>> elements;
            if (begin != end && begin->getTokenType() == lexer::TokenType::OpenSquareBracket)
            {
                begin++;
                while (begin != end && begin->getTokenType() != lexer::TokenType::CloseSquareBracket)
                {
                    begin++;
                }
                if (begin != end)
                    begin++;
            }

            result.push_back(std::make_unique<Syntax::TableDeclaration>(start, begin, id, std::move(width),
                                                                        std::move(length), std::move(elements)));
        }
        return result;
    }

    if (type == lexer::TokenType::OverlayKeyword)
    {
        auto start = begin;
        begin++;
        if (begin != end && begin->getTokenType() == lexer::TokenType::Identifier)
        {
            auto base = begin;
            begin++;
            if (begin != end && begin->getTokenType() == lexer::TokenType::WithKeyword)
            {
                begin++;
            }

            auto decls = parseDeclaration(begin, end, context);
            if (!decls.empty())
            {
                result.push_back(
                    std::make_unique<Syntax::OverlayDeclaration>(start, begin, base, base, std::move(decls[0])));
            }
            else
            {
                context.log(errors::parser::EXPECTED_DECLARATION.args(*(begin == end ? (end - 1) : begin),
                                                                      context.getSourceInterface(),
                                                                      *(begin == end ? (end - 1) : begin)));
                DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_DECLARATION);
                begin = start;
            }
        }
        else
        {
            begin = start;
        }
        return result;
    }

    return result;
}
} // namespace coral66::parser