/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "parser.hpp"

#include "lib/frontend/common/messages/errors.hpp"
#include "lib/frontend/common/source_object.hpp"
#include "parser_support.hpp"

namespace coral66::parser
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
            context.log(errors::parser::EXPECTED_N.args(*begin, context.getSourceInterface(),
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

    std::vector<std::unique_ptr<Syntax::Declaration>> declarations;
    std::vector<std::unique_ptr<Syntax::Statement>> statements;
    std::vector<std::unique_ptr<Syntax::PlaceReference>> placeReferences;

    while (begin != end && begin->getTokenType() != lexer::TokenType::EndKeyword)
    {
        if (auto newDeclarations = parseDeclaration(begin, end, context); !newDeclarations.empty())
        {
            for (auto& d : newDeclarations)
                declarations.push_back(std::move(d));
            continue;
        }
        if (auto stmt = parseStatement(begin, end, context))
        {
            statements.push_back(std::move(stmt));
            continue;
        }
        if (auto placeReference = parsePlaceReference(begin, end, context))
        {
            placeReferences.push_back(std::move(placeReference));
            continue;
        }
        begin++;
    }

    if (begin != end && begin->getTokenType() == lexer::TokenType::EndKeyword)
    {
        begin++;
    }
    else
    {
        context.log(errors::parser::EXPECTED_END.args(*(begin == end ? (end - 1) : begin), context.getSourceInterface(),
                                                      *(begin == end ? (end - 1) : begin)));
        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_END);
        return nullptr;
    }

    return std::make_unique<Syntax::Block>(blockStart, begin, std::move(declarations), std::move(statements));
}

} // namespace coral66::parser
