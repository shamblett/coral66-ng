/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "parser.hpp"

#include "lib/frontend/common/messages/errors.hpp"
#include "lib/frontend/common/source_object.hpp"

namespace coral66::Parser
{

Context::Context(const source_interface& sourceInterface, llvm::raw_ostream* reporter)
    : m_sourceInterface(sourceInterface), m_reporter(reporter)
{
}

void Context::log(const Message& message)
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

const source_interface& Context::getSourceInterface() const
{
    return m_sourceInterface;
}

std::size_t Context::getErrorCount() const
{
    return m_errorCount;
}

Syntax::Program parseProgram(const CSourceObject& sourceObject, llvm::raw_ostream* reporter, bool* errors)
{
    Context context(sourceObject, reporter);
    auto begin = sourceObject.data().data();
    auto end = sourceObject.data().data() + sourceObject.data().size();

    // Consume optional PROGRAM/SEGMENT header
    if (begin != end
        && (begin->getTokenType() == lexer::TokenType::ProgramKeyword
            || begin->getTokenType() == lexer::TokenType::SegmentKeyword))
    {
        begin++;
        if (begin != end && begin->getTokenType() == lexer::TokenType::Identifier)
        {
            begin++;
        }
    }

    auto block = parseBlock(begin, end, context);
    if (!block && context.getErrorCount() == 0)
    {
        if (begin != end)
        {
            context.log(Errors::Parser::EXPECTED_N.args(*begin, context.getSourceInterface(),
                                                        lexer::TokenType::BeginKeyword, *begin));
        }
    }

    if (errors)
    {
        *errors = context.getErrorCount() > 0;
    }

    return {sourceObject.data().data(), end, std::move(block)};
}

std::unique_ptr<Syntax::Block> parseBlock(lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context)
{
    if (begin == end || begin->getTokenType() != lexer::TokenType::BeginKeyword)
    {
        return nullptr;
    }
    auto blockStart = begin;
    begin++;

    std::vector<std::unique_ptr<Syntax::Declaration>> decls;
    std::vector<std::unique_ptr<Syntax::Statement>> stmts;

    while (begin != end && begin->getTokenType() != lexer::TokenType::EndKeyword)
    {
        auto newDecls = parseDeclaration(begin, end, context);
        if (!newDecls.empty())
        {
            for (auto& d : newDecls)
                decls.push_back(std::move(d));
            continue;
        }
        if (auto stmt = parseStatement(begin, end, context))
        {
            stmts.push_back(std::move(stmt));
            continue;
        }
        begin++;
    }

    if (begin != end)
    {
        begin++;
    }

    return std::make_unique<Syntax::Block>(blockStart, begin, std::move(decls), std::move(stmts));
}

} // namespace coral66::Parser
