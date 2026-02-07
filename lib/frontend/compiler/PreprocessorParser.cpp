/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "PreprocessorParser.hpp"

#include "lib/frontend/common/ErrorMessages.hpp"
#include "lib/frontend/common/SourceObject.hpp"

namespace
{
[[maybe_unused]]bool expect(coral66::Lexer::TokenType tokenType, coral66::Lexer::PPTokenIterator& begin, coral66::Lexer::PPTokenIterator const end,
            coral66::PP::Context& context, const std::optional<coral66::Message> &additional = {})
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

// ReSharper disable once CppDFAUnreachableFunctionCall
coral66::Lexer::PPTokenIterator findNewline(coral66::Lexer::PPTokenIterator const begin, coral66::Lexer::PPTokenIterator const end)
{
    return std::find_if(begin, end,
                        coral66::compose(coral66::bind_front(std::equal_to<>{}, coral66::Lexer::TokenType::Newline),
                                     &coral66::Lexer::PPToken::getTokenType));
}

[[maybe_unused]]void skipLine(coral66::Lexer::PPTokenIterator& begin, coral66::Lexer::PPTokenIterator end)
{
    begin = findNewline(begin, end);
    begin = begin == end ? begin : begin + 1;
}

} // namespace

coral66::PP::Context::Context(const coral66::SourceInterface& sourceInterface, llvm::raw_ostream* reporter)
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

const coral66::SourceInterface& coral66::PP::Context::getSourceInterface() const
{
    return m_sourceInterface;
}

coral66::PP::File coral66::PP::buildTree(const PPSourceObject& sourceObject, llvm::raw_ostream* reporter, bool* errors)
{
    Context context(sourceObject, reporter);
    const auto* begin = sourceObject.data().data();
    auto file = parseFile(begin, sourceObject.data().data() + sourceObject.data().size(), context);
    if (errors)
    {
        *errors = context.getErrorCount();
    }
    return file;
}

coral66::PP::File coral66::PP::parseFile(Lexer::PPTokenIterator& begin, Lexer::PPTokenIterator const end, Context& context)
{
    std::vector<Group> result;
    while (begin != end)
    {
        result.push_back(parseGroup(begin, end, context));
    }
    return File{std::move(result)};
}

coral66::PP::Group coral66::PP::parseGroup(Lexer::PPTokenIterator& begin, Lexer::PPTokenIterator const end, Context& context,
                                   bool const)
{
    CORAL66_ASSERT(begin != end);
    std::vector<GroupPart> parts;
    do
    {
        if (begin->getTokenType() == Lexer::TokenType::Identifier && begin->getValue() == "DEFINE")
        {
            parts.emplace_back(parseControlLine(begin, end, context));
            continue;
        }
        
        // TextBlock
        const auto* start = begin;
        while (begin != end)
        {
            if (begin->getTokenType() == Lexer::TokenType::Identifier && begin->getValue() == "DEFINE")
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
    CORAL66_ASSERT(begin != end && begin->getTokenType() == Lexer::TokenType::Identifier && begin->getValue() == "DEFINE");
    const auto* defineToken = begin;
    begin++;
    
    // Find SemiColon
    const auto* semi = std::find_if(begin, end, [](const Lexer::PPToken& t) { 
        return t.getTokenType() == Lexer::TokenType::SemiColon; 
    });
    
    if (semi == end) {
        context.log(coral66::Errors::Parser::EXPECTED_N.args(*(end - 1), context.getSourceInterface(), coral66::Lexer::TokenType::SemiColon, *(end-1)));
        // Consume rest
        begin = end;
        return {DefineDirective{defineToken, {}}};
    }
    
    // Tokens between DEFINE and ;
    std::span<const Lexer::PPToken> tokens(begin, semi);
    begin = semi + 1; // Consume ;
    
    return {DefineDirective{defineToken, tokens}};
}

// Unused functions removed (parseIfSection, parseElseGroup, parseElIfGroup, parseIfGroup)
coral66::PP::IfSection coral66::PP::parseIfSection(Lexer::PPTokenIterator&, Lexer::PPTokenIterator const, Context&) { CORAL66_UNREACHABLE; }
coral66::PP::ElseGroup coral66::PP::parseElseGroup(Lexer::PPTokenIterator&, Lexer::PPTokenIterator const, Context&) { CORAL66_UNREACHABLE; }
coral66::PP::ElIfGroup coral66::PP::parseElIfGroup(Lexer::PPTokenIterator&, Lexer::PPTokenIterator const, Context&) { CORAL66_UNREACHABLE; }
coral66::PP::IfGroup coral66::PP::parseIfGroup(Lexer::PPTokenIterator&, Lexer::PPTokenIterator const, Context&) { CORAL66_UNREACHABLE; }
