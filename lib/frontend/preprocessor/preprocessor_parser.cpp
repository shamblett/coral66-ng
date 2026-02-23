/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "preprocessor_parser.hpp"

#include "../common/messages/errors.hpp"
#include "lib/frontend/common/source_object.hpp"

namespace
{
[[maybe_unused]] bool expect(coral66::Lexer::TokenType tokenType, coral66::Lexer::PPTokenIterator& begin,
                             coral66::Lexer::PPTokenIterator const end, coral66::PP::Context& context,
                             const std::optional<coral66::Message>& additional = {})
{
    if (begin == end || begin->getTokenType() != tokenType)
    {
        if (begin == end || begin->getTokenType() == coral66::Lexer::TokenType::Newline)
        {
            context.log(coral66::Errors::Parser::EXPECTED_N.args(*(begin - 1), context.getSourceInterface(), tokenType,
                                                                 *(begin - 1)));
        }
        else
        {
            context.log(coral66::Errors::Parser::EXPECTED_N_INSTEAD_OF_N.args(*begin, context.getSourceInterface(),
                                                                              tokenType, *begin));
        }
        if (additional)
        {
            context.log(*additional);
        }
        return false;
    }
    begin++;
    return true;
}

coral66::Lexer::PPTokenIterator findNewline(coral66::Lexer::PPTokenIterator const begin,
                                            coral66::Lexer::PPTokenIterator const end)
{
    return std::find_if(begin, end,
                        coral66::compose(coral66::bind_front(std::equal_to<>{}, coral66::Lexer::TokenType::Newline),
                                         &coral66::Lexer::PPToken::getTokenType));
}

[[maybe_unused]] void skipLine(coral66::Lexer::PPTokenIterator& begin, const coral66::Lexer::PPTokenIterator end)
{
    begin = findNewline(begin, end);
    begin = begin == end ? begin : begin + 1;
}

} // namespace

coral66::PP::Context::Context(const coral66::source_interface& sourceInterface, llvm::raw_ostream* reporter)
    : m_sourceInterface(sourceInterface), m_reporter(reporter)
{
}

void coral66::PP::Context::log(const Message& message)
{
    if (message.getSeverity() == Severity::Error)
    {
        m_errorCount++;
    }
    if (m_reporter)
    {
        *m_reporter << message;
    }
}

std::size_t coral66::PP::Context::getErrorCount() const
{
    return m_errorCount;
}

const coral66::source_interface& coral66::PP::Context::getSourceInterface() const
{
    return m_sourceInterface;
}

coral66::PP::File coral66::PP::parseMacro(Lexer::PPTokenIterator& begin, Lexer::PPTokenIterator const end,
                                         Context& context)
{
    std::vector<Group> result;
    while (begin != end)
    {
        result.push_back(parseGroup(begin, end, context));
    }
    return File{std::move(result)};
}

coral66::PP::Group coral66::PP::parseGroup(Lexer::PPTokenIterator& begin, Lexer::PPTokenIterator const end,
                                           Context& context, bool const)
{
    CORAL66_ASSERT(begin != end);
    std::vector<GroupPart> parts;
    do
    {
        if (begin->getTokenType() == Lexer::TokenType::Identifier &&
            (begin->getValue() == "DEFINE" || begin->getValue() == "DELETE"))
        {
            parts.emplace_back(parseControlLine(begin, end, context));
            continue;
        }

        // TextBlock
        const auto* start = begin;
        while (begin != end)
        {
            if (begin->getTokenType() == Lexer::TokenType::Identifier &&
                (begin->getValue() == "DEFINE" || begin->getValue() == "DELETE"))
            {
                break;
            }
            begin++;
        }
        if (start != begin)
        {
            parts.emplace_back(TextBlock{{start, begin}});
        }
    } while (begin != end);
    return Group{std::move(parts)};
}

coral66::PP::ControlLine coral66::PP::parseControlLine(Lexer::PPTokenIterator& begin, Lexer::PPTokenIterator const end,
                                                       Context& context)
{
    CORAL66_ASSERT(begin != end && begin->getTokenType() == Lexer::TokenType::Identifier
                   && (begin->getValue() == "DEFINE" || begin->getValue() == "DELETE"));
    const auto* directiveToken = begin;
    bool isDefine = begin->getValue() == "DEFINE";
    begin++;

    // Find SemiColon
    const auto* semi = std::find_if(begin, end, [](const Lexer::PPToken& t)
                                    { return t.getTokenType() == Lexer::TokenType::SemiColon; });

    if (semi == end)
    {
        context.log(coral66::Errors::Parser::EXPECTED_N.args(*(end - 1), context.getSourceInterface(),
                                                             coral66::Lexer::TokenType::SemiColon, *(end - 1)));
        // Consume rest
        begin = end;
        if (isDefine)
            return {DefineDirective{directiveToken, {}}};
        else
            return {directiveToken};
    }

    // Tokens between directive and ;
    std::span<const Lexer::PPToken> tokens(begin, semi);
    begin = semi + 1; // Consume ;

    if (isDefine)
    {
        auto iter = tokens.begin();
        if (iter == tokens.end() || iter->getTokenType() != Lexer::TokenType::Identifier)
        {
            context.log(coral66::Errors::Parser::EXPECTED_N.args(
                iter == tokens.end() ? *semi : *iter, context.getSourceInterface(),
                coral66::Lexer::TokenType::Identifier, iter == tokens.end() ? *semi : *iter));
        }
        return {DefineDirective{directiveToken, tokens}};
    }
    else
    {
        auto iter = tokens.begin();
        if (iter != tokens.end() && iter->getTokenType() == Lexer::TokenType::Identifier)
        {
            const auto* macroName = &(*iter);
            iter++;
            if (iter != tokens.end())
            {
                // Optional: warn/error about extra tokens after Macroname in DELETE
            }
            return {macroName};
        }
        else
        {
            context.log(coral66::Errors::Parser::EXPECTED_N_INSTEAD_OF_N.args(
                iter == tokens.end() ? *semi : *iter, context.getSourceInterface(),
                coral66::Lexer::TokenType::Identifier, iter == tokens.end() ? *semi : *iter));
            return {directiveToken};
        }
    }
}
