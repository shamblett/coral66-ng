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

