/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "parser.hpp"

#include <functional>

#include "lib/frontend/common/messages/errors.hpp"
#include "lib/frontend/common/source_object.hpp"
#include "parser_support.hpp"
#include "parser_utilities.hpp"

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

    std::function<bool()> parseGroup;
    parseGroup = [&]() -> bool
    {
        if (begin == end)
            return false;

        auto tType = begin->getTokenType();
        if (tType == lexer::TokenType::Comma || tType == lexer::TokenType::CloseParentheses
            || tType == lexer::TokenType::CloseSquareBracket || tType == lexer::TokenType::SemiColon)
        {
            // Void group
            return true;
        }

        if (tType == lexer::TokenType::Plus || tType == lexer::TokenType::Minus)
        {
            begin++;
            if (begin != end
                && ((begin->getTokenType() == lexer::TokenType::Literal
                     && (begin->getType() == lexer::CToken::Type::Integer
                         || begin->getType() == lexer::CToken::Type::Floating))
                    || begin->getTokenType() == lexer::TokenType::OctalKeyword))
            {
                if (begin->getTokenType() == lexer::TokenType::OctalKeyword)
                {
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
                    begin++;
                }
                return true;
            }
            return false;
        }
        else if ((tType == lexer::TokenType::Literal
                  && (begin->getType() == lexer::CToken::Type::Integer
                      || begin->getType() == lexer::CToken::Type::Floating))
                 || tType == lexer::TokenType::OctalKeyword || tType == lexer::TokenType::StringLiteral)
        {
            if (tType == lexer::TokenType::OctalKeyword)
            {
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
                return true;
            }
            else if (tType == lexer::TokenType::StringLiteral)
            {
                begin++;
                return true;
            }
            else if (tType == lexer::TokenType::Literal && begin->getType() == lexer::CToken::Type::Floating)
            {
                begin++;
                return true;
            }
            else // Integer Literal
            {
                begin++;
                if (begin != end && begin->getTokenType() == lexer::TokenType::OpenParentheses)
                {
                    begin++;
                    while (begin != end)
                    {
                        if (!parseGroup())
                            return false;
                        if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
                        {
                            begin++;
                        }
                        else if (begin != end && begin->getTokenType() == lexer::TokenType::CloseParentheses)
                        {
                            begin++;
                            return true;
                        }
                        else
                        {
                            return false;
                        }
                    }
                    return false;
                }
                return true;
            }
        }
        return false;
    };

    if (type == lexer::TokenType::IntegerKeyword || type == lexer::TokenType::FloatingKeyword
        || type == lexer::TokenType::FixedKeyword)
    {
        auto typeToken = begin;
        begin++;

        if (type == lexer::TokenType::FixedKeyword)
        {
            validateFixedFormat(begin, end, context);
        }

        if (begin != end && begin->getTokenType() == lexer::TokenType::ArrayKeyword)
        {
            begin++; // Consume ARRAY
            bool expectMore = true;
            while (begin != end && expectMore)
            {
                std::vector<lexer::CTokenIterator> ids;
                while (begin != end && begin->getTokenType() == lexer::TokenType::Identifier)
                {
                    ids.push_back(begin);
                    begin++;
                    if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
                    {
                        // Check if the next token is an identifier and the token after is NOT a bracket.
                        // Actually, if it's an Arrayitem, it's Idlist [ Sizelist ].
                        // So we keep gathering identifiers separated by comma as long as we haven't seen '['
                        // Wait, no. "p, q[1:3]" means p and q share [1:3]. So we just unconditionally consume the
                        // comma. But what if it's "p[1:3], q[1:4]"? Then the comma is AFTER the bracket! So if we are
                        // here, we just saw an identifier. Next is a comma. We peek ahead.
                        auto peek = begin + 1;
                        if (peek != end && peek->getTokenType() == lexer::TokenType::Identifier)
                        {
                            begin++; // consume comma
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

                if (ids.empty())
                {
                    context.log(errors::parser::EXPECTED_IDENTIFIER.args(*(begin == end ? (end - 1) : begin),
                                                                         context.getSourceInterface(),
                                                                         *(begin == end ? (end - 1) : begin)));
                    DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_IDENTIFIER);
                    break;
                }

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
                        else
                        {
                            context.log(errors::parser::EXPECTED_COLON.args(*(begin == end ? (end - 1) : begin),
                                                                            context.getSourceInterface(),
                                                                            *(begin == end ? (end - 1) : begin)));
                            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_COLON);
                            break;
                        }

                        if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
                            begin++;
                        else
                            break;
                    }
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
                    }
                }

                result.push_back(std::make_unique<Syntax::ArrayDeclaration>(typeToken, begin, typeToken, std::move(ids),
                                                                            std::move(dims)));

                if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
                {
                    begin++;
                }
                else
                {
                    expectMore = false;
                }
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

        std::vector<lexer::CTokenIterator> presetTokens;
        if (begin != end
            && (begin->getTokenType() == lexer::TokenType::Assignment
                || begin->getTokenType() == lexer::TokenType::PresetKeyword))
        {
            begin++;

            bool parsedAny = false;
            while (begin != end)
            {
                auto groupStart = begin;
                if (!parseGroup())
                {
                    context.log(errors::parser::EXPECTED_EXPRESSION.args(*(begin == end ? (end - 1) : begin),
                                                                         context.getSourceInterface(),
                                                                         *(begin == end ? (end - 1) : begin)));
                    DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_EXPRESSION);

                    // Recover
                    while (begin != end && begin->getTokenType() != lexer::TokenType::SemiColon
                           && begin->getTokenType() != lexer::TokenType::CloseParentheses)
                    {
                        begin++;
                    }
                    parsedAny = true; // Error reported
                    break;
                }
                for (auto it = groupStart; it != begin; ++it)
                    presetTokens.push_back(it);

                parsedAny = true;
                if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
                {
                    presetTokens.push_back(begin);
                    begin++;
                }
                else
                {
                    break;
                }
            }
            if (!parsedAny)
            {
                context.log(errors::parser::EXPECTED_EXPRESSION.args(*(begin == end ? (end - 1) : begin),
                                                                     context.getSourceInterface(),
                                                                     *(begin == end ? (end - 1) : begin)));
                DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_EXPRESSION);
            }
        }

        if (!result.empty() && !presetTokens.empty())
        {
            for (auto& decl : result)
            {
                if (auto* var = dynamic_cast<Syntax::VariableDeclaration*>(decl.get()))
                {
                    var->setPresetTokens(presetTokens);
                }
                else if (auto* arr = dynamic_cast<Syntax::ArrayDeclaration*>(decl.get()))
                {
                    arr->setPresetTokens(presetTokens);
                }
            }
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
                    if (begin->getTokenType() == lexer::TokenType::PresetKeyword)
                    {
                        begin++;

                        bool parsedAny = false;
                        while (begin != end && begin->getTokenType() != lexer::TokenType::CloseSquareBracket)
                        {
                            if (!parseGroup())
                            {
                                context.log(errors::parser::EXPECTED_EXPRESSION.args(
                                    *(begin == end ? (end - 1) : begin), context.getSourceInterface(),
                                    *(begin == end ? (end - 1) : begin)));
                                DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_EXPRESSION);

                                // Recover
                                while (begin != end && begin->getTokenType() != lexer::TokenType::SemiColon
                                       && begin->getTokenType() != lexer::TokenType::CloseSquareBracket)
                                {
                                    begin++;
                                }
                                parsedAny = true; // Error reported
                                break;
                            }
                            parsedAny = true;
                            if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
                            {
                                begin++;
                            }
                            else
                            {
                                break;
                            }
                        }
                        if (!parsedAny)
                        {
                            context.log(errors::parser::EXPECTED_EXPRESSION.args(*(begin == end ? (end - 1) : begin),
                                                                                 context.getSourceInterface(),
                                                                                 *(begin == end ? (end - 1) : begin)));
                            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_EXPRESSION);
                        }
                        break; // PRESET is always the last part of the table body
                    }

                    auto elemStart = begin;
                    if (begin->getTokenType() == lexer::TokenType::Identifier)
                    {
                        auto elemId = begin;
                        begin++;

                        bool isPartWord = false;
                        if (begin != end && begin->getTokenType() == lexer::TokenType::UnsignedKeyword)
                        {
                            isPartWord = true;
                            begin++;
                        }

                        if (begin != end && begin->getTokenType() == lexer::TokenType::OpenParentheses)
                        {
                            isPartWord = true;
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
                        else if (!isPartWord
                                 && (begin->getTokenType() == lexer::TokenType::FloatingKeyword
                                     || begin->getTokenType() == lexer::TokenType::IntegerKeyword
                                     || begin->getTokenType() == lexer::TokenType::FixedKeyword))
                        {
                            auto typeToken = begin->getTokenType();
                            begin++;
                            if (typeToken == lexer::TokenType::FixedKeyword)
                            {
                                validateFixedFormat(begin, end, context);
                            }
                        }
                        else if (!isPartWord)
                        {
                            context.log(errors::parser::EXPECTED_TYPE.args(*(begin == end ? (end - 1) : begin),
                                                                           context.getSourceInterface(),
                                                                           *(begin == end ? (end - 1) : begin)));
                            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_TYPE);
                            break;
                        }

                        // Wordposition
                        if (begin != end
                            && (begin->getTokenType() == lexer::TokenType::Plus
                                || begin->getTokenType() == lexer::TokenType::Minus))
                        {
                            begin++;
                        }
                        if (begin != end && begin->getTokenType() == lexer::TokenType::Literal
                            && begin->getType() == lexer::CToken::Type::Integer)
                        {
                            begin++;
                        }

                        if (isPartWord)
                        {
                            // Bitposition
                            if (begin != end
                                && (begin->getTokenType() == lexer::TokenType::Comma
                                    || begin->getTokenType() == lexer::TokenType::BitKeyword))
                            {
                                begin++;
                                if (begin != end && begin->getTokenType() == lexer::TokenType::Literal
                                    && begin->getType() == lexer::CToken::Type::Integer)
                                {
                                    begin++;
                                }
                            }
                        }

                        elements.push_back(std::make_unique<Syntax::TableElementDeclaration>(elemStart, begin, elemId));

                        if (begin != end && begin->getTokenType() == lexer::TokenType::SemiColon)
                        {
                            begin++;
                        }
                        else if (begin != end && begin->getTokenType() == lexer::TokenType::PresetKeyword)
                        {
                            // Allow loop to catch it next iteration
                        }
                        else
                        {
                            break; // Stop parsing elements
                        }
                    }
                    else
                    {
                        context.log(errors::parser::EXPECTED_IDENTIFIER.args(*(begin == end ? (end - 1) : begin),
                                                                             context.getSourceInterface(),
                                                                             *(begin == end ? (end - 1) : begin)));
                        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_IDENTIFIER);
                        break;
                    }
                }
                if (begin != end && begin->getTokenType() == lexer::TokenType::CloseSquareBracket)
                {
                    begin++;
                }
            }

            std::vector<lexer::CTokenIterator> presetTokens;
            if (begin != end
                && (begin->getTokenType() == lexer::TokenType::Assignment
                    || begin->getTokenType() == lexer::TokenType::PresetKeyword))
            {
                begin++;

                bool parsedAny = false;
                while (begin != end)
                {
                    auto groupStart = begin;
                    if (!parseGroup())
                    {
                        context.log(errors::parser::EXPECTED_EXPRESSION.args(*(begin == end ? (end - 1) : begin),
                                                                             context.getSourceInterface(),
                                                                             *(begin == end ? (end - 1) : begin)));
                        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_EXPRESSION);

                        // Recover
                        while (begin != end && begin->getTokenType() != lexer::TokenType::SemiColon
                               && begin->getTokenType() != lexer::TokenType::CloseParentheses)
                        {
                            begin++;
                        }
                        parsedAny = true; // Error reported
                        break;
                    }
                    for (auto it = groupStart; it != begin; ++it)
                        presetTokens.push_back(it);

                    parsedAny = true;
                    if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
                    {
                        presetTokens.push_back(begin);
                        begin++;
                    }
                    else
                    {
                        break;
                    }
                }
                if (!parsedAny)
                {
                    context.log(errors::parser::EXPECTED_EXPRESSION.args(*(begin == end ? (end - 1) : begin),
                                                                         context.getSourceInterface(),
                                                                         *(begin == end ? (end - 1) : begin)));
                    DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_EXPRESSION);
                }
            }

            result.push_back(std::make_unique<Syntax::TableDeclaration>(
                start, begin, id, std::move(width), std::move(length), std::move(elements), std::move(presetTokens)));
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

            lexer::CTokenIterator offset = base;
            std::optional<lexer::CTokenIterator> offset2 = std::nullopt;
            if (begin != end && begin->getTokenType() == lexer::TokenType::OpenSquareBracket)
            {
                begin++;
                if (begin != end
                    && (begin->getTokenType() == lexer::TokenType::Plus
                        || begin->getTokenType() == lexer::TokenType::Minus))
                {
                    begin++;
                }
                if (begin != end && begin->getTokenType() == lexer::TokenType::Literal
                    && begin->getType() == lexer::CToken::Type::Integer)
                {
                    offset = begin;
                    begin++;
                }

                if (begin != end && begin->getTokenType() == lexer::TokenType::Comma)
                {
                    begin++;
                    if (begin != end
                        && (begin->getTokenType() == lexer::TokenType::Plus
                            || begin->getTokenType() == lexer::TokenType::Minus))
                    {
                        begin++;
                    }
                    if (begin != end && begin->getTokenType() == lexer::TokenType::Literal
                        && begin->getType() == lexer::CToken::Type::Integer)
                    {
                        offset2 = begin;
                        begin++;
                    }
                }

                if (begin != end && begin->getTokenType() == lexer::TokenType::CloseSquareBracket)
                {
                    begin++;
                }
            }

            if (begin != end && begin->getTokenType() == lexer::TokenType::WithKeyword)
            {
                begin++;
            }

            auto decls = parseDeclaration(begin, end, context);
            if (!decls.empty())
            {
                result.push_back(std::make_unique<Syntax::OverlayDeclaration>(start, begin, base, offset, offset2,
                                                                              std::move(decls[0])));
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