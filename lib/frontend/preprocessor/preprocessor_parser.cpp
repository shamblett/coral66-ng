/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "preprocessor_parser.hpp"

#include "lib/frontend/common/messages/errors.hpp"
#include "lib/frontend/common/source_object.hpp"

namespace
{
[[maybe_unused]] bool expect(coral66::lexer::TokenType tokenType, coral66::lexer::PPTokenIterator& begin,
                             coral66::lexer::PPTokenIterator const end, coral66::pp::Context& context,
                             const std::optional<coral66::Message>& additional = {})
{
    if (begin == end || begin->getTokenType() != tokenType)
    {
        if (begin == end || begin->getTokenType() == coral66::lexer::TokenType::Newline)
        {
            context.log(coral66::errors::parser::EXPECTED_N.args(*(begin - 1), context.getSourceInterface(), tokenType,
                                                                 *(begin - 1)));
        }
        else
        {
            context.log(coral66::errors::parser::EXPECTED_N_INSTEAD_OF_N.args(*begin, context.getSourceInterface(),
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

coral66::lexer::PPTokenIterator findNewline(coral66::lexer::PPTokenIterator const begin,
                                            coral66::lexer::PPTokenIterator const end)
{
    return std::find_if(begin, end,
                        coral66::compose(coral66::bind_front(std::equal_to<>{}, coral66::lexer::TokenType::Newline),
                                         &coral66::lexer::PPToken::getTokenType));
}

coral66::lexer::PPTokenIterator findSemicolon(coral66::lexer::PPTokenIterator const begin,
                                              coral66::lexer::PPTokenIterator const end)
{
    return std::find_if(begin, end,
                        coral66::compose(coral66::bind_front(std::equal_to<>{}, coral66::lexer::TokenType::SemiColon),
                                         &coral66::lexer::PPToken::getTokenType));
}

[[maybe_unused]] void skipLine(coral66::lexer::PPTokenIterator& begin, const coral66::lexer::PPTokenIterator end)
{
    begin = findNewline(begin, end);
    begin = begin == end ? begin : begin + 1;
}

} // namespace

coral66::pp::Context::Context(const coral66::source_interface& sourceInterface, llvm::raw_ostream* reporter)
    : m_sourceInterface(sourceInterface), m_reporter(reporter)
{
}

void coral66::pp::Context::log(const Message& message)
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

std::size_t coral66::pp::Context::getErrorCount() const
{
    return m_errorCount;
}

const coral66::source_interface& coral66::pp::Context::getSourceInterface() const
{
    return m_sourceInterface;
}

coral66::pp::File coral66::pp::parseMacro(lexer::PPTokenIterator& begin, lexer::PPTokenIterator const end,
                                          Context& context)
{
    std::vector<Group> result;
    while (begin != end)
    {
        result.push_back(parseGroup(begin, end, context));
    }
    return File{std::move(result)};
}

coral66::pp::Group coral66::pp::parseGroup(lexer::PPTokenIterator& begin, lexer::PPTokenIterator const end,
                                           Context& context, bool const)
{
    CORAL66_ASSERT(begin != end);
    std::vector<GroupPart> parts;
    auto startText = begin;

    auto flushText = [&]()
    {
        if (startText != begin)
        {
            parts.emplace_back(TextBlock{{startText, begin}});
            startText = begin;
        }
    };

    while (begin != end)
    {
        if (begin->getTokenType() == lexer::TokenType::Identifier)
        {
            if (auto value = begin->getValue(); value == lexer::tokenName(lexer::TokenType::DefineKeyword)
                || value == tokenName(lexer::TokenType::DeleteKeyword))
            {
                flushText();
                parts.emplace_back(parseControlLine(begin, end, context));
                startText = begin;
                continue;
            }
        }
        ++begin;
    }
    flushText();
    return Group{std::move(parts)};
}

coral66::pp::ControlLine coral66::pp::parseControlLine(lexer::PPTokenIterator& begin, lexer::PPTokenIterator const end,
                                                       Context& context)
{
    CORAL66_ASSERT(begin != end && begin->getTokenType() == lexer::TokenType::Identifier);
    auto value = begin->getValue();
    if (value == lexer::tokenName(lexer::TokenType::DefineKeyword))
    {
        begin++;
        const auto* eol = findSemicolon(begin, end);
        expect(lexer::TokenType::SemiColon, eol, end, context);
        const auto* lineStart = begin;
        begin = eol;
        return {DefineDirective{lineStart - 1, {lineStart, eol - 1}}};
    }
    if (value == lexer::tokenName(lexer::TokenType::DeleteKeyword))
    {
        begin++;
        if (!expect(lexer::TokenType::Identifier, begin, end, context))
        {
            skipLine(begin, end);
            return {};
        }
        const auto* name = begin - 1;
        if (!expect(lexer::TokenType::SemiColon, begin, end, context))
        {
            skipLine(begin, end);
            return {};
        }
        return {name};
    }
    CORAL66_UNREACHABLE;
}
