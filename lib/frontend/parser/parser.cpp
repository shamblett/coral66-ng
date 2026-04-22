/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "parser.hpp"

#include "lib/support/vendor/ctrack/include/ctrack.hpp"
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
    CTRACK_NAME("Parse");
    Context context(sourceObject, reporter);
    auto begin = sourceObject.data().data();
    auto end = sourceObject.data().data() + sourceObject.data().size();

    // PROGRAM/SEGMENT must come first
    lexer::CTokenIterator name = nullptr;
    bool isSegment = false;
    if (begin != end && begin->getTokenType() == lexer::TokenType::ProgramKeyword)
    {
        begin++;
        if (begin != end && begin->getTokenType() == lexer::TokenType::Identifier)
        {
            name = begin;
        }
        else
        {
            context.log(errors::parser::EXPECTED_IDENTIFIER.args(*(begin == end ? (end - 1) : begin),
                                                                 context.getSourceInterface(),
                                                                 *(begin == end ? (end - 1) : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_IDENTIFIER);
            return {sourceObject.data().data(), end, nullptr, nullptr, begin, false};
        }
    }
    else if (begin != end && begin->getTokenType() == lexer::TokenType::SegmentKeyword)
    {
        begin++;
        if (begin != end && begin->getTokenType() == lexer::TokenType::Identifier)
        {
            name = begin;
            isSegment = true;
        }
        else
        {
            context.log(errors::parser::EXPECTED_IDENTIFIER.args(*(begin == end ? (end - 1) : begin),
                                                                 context.getSourceInterface(),
                                                                 *(begin == end ? (end - 1) : begin)));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_IDENTIFIER);
            return {sourceObject.data().data(), end, nullptr, nullptr, begin, false};
        }
    }
    else
    {
        context.log(errors::parser::EXPECTED_PROGRAM_SEGMENT.args(*begin, context.getSourceInterface(), *begin));
        DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_PROGRAM_SEGMENT);
        if (errors)
        {
            *errors = context.getErrorCount() > 0;
        }
        return {sourceObject.data().data(), end, nullptr, nullptr, name, false};
    }

    // Communicator
    begin++;
    std::unique_ptr<Syntax::Communicator> communicator = parseCommunicator(begin, end, context);
    if (!communicator && context.getErrorCount() > 0)
    {
        if (begin != end)
        {
            context.log(errors::parser::MALFORMED_COMMUNICATOR.args(*begin, context.getSourceInterface(), *begin));
            DiagnosticCollector::addDiagnostic(errors::parser::MALFORMED_COMMUNICATOR);
            if (errors)
            {
                *errors = context.getErrorCount() > 0;
            }
            return {sourceObject.data().data(), end, nullptr, nullptr, begin, false};
        }
    }

    // Block
    auto block = parseBlock(begin, end, context);
    if (!block && context.getErrorCount() == 0)
    {
        if (begin != end)
        {
            context.log(errors::parser::EXPECTED_BEGIN.args(*begin, context.getSourceInterface(), *begin));
            DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_BEGIN);
        }
    }

    if (errors)
    {
        *errors = context.getErrorCount() > 0;
    }

    return {sourceObject.data().data(), end, std::move(block), std::move(communicator), name, isSegment};
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

    bool declarationsAllowed = true;
    bool firstItem = true;

    while (begin != end && begin->getTokenType() != lexer::TokenType::EndKeyword)
    {
        if (!firstItem)
        {
            if (begin != end && begin->getTokenType() == lexer::TokenType::SemiColon)
            {
                begin++;
                if (begin != end && begin->getTokenType() == lexer::TokenType::EndKeyword)
                {
                    break;
                }
            }
            else
            {
                context.log(errors::parser::EXPECTED_SEMICOLON.args(*(begin == end ? (end - 1) : begin),
                                                                    context.getSourceInterface(),
                                                                    *(begin == end ? (end - 1) : begin)));
                DiagnosticCollector::addDiagnostic(errors::parser::EXPECTED_SEMICOLON);
                while (begin != end && begin->getTokenType() != lexer::TokenType::SemiColon
                       && begin->getTokenType() != lexer::TokenType::EndKeyword)
                {
                    begin++;
                }
                if (begin != end && begin->getTokenType() == lexer::TokenType::SemiColon)
                {
                    begin++;
                    if (begin != end && begin->getTokenType() == lexer::TokenType::EndKeyword)
                    {
                        break;
                    }
                }
                else
                {
                    continue;
                }
            }
        }

        if (declarationsAllowed)
        {
            if (auto procedure = parseProcedure(begin, end, context))
            {
                declarations.push_back(std::move(procedure));
                firstItem = false;
                continue;
            }
            if (auto newDeclarations = parseDeclaration(begin, end, context); !newDeclarations.empty())
            {
                for (auto& d : newDeclarations)
                    declarations.push_back(std::move(d));
                firstItem = false;
                continue;
            }
            if (auto placeReference = parsePlaceReference(begin, end, context))
            {
                placeReferences.push_back(std::move(placeReference));
                firstItem = false;
                continue;
            }

            declarationsAllowed = false;
        }

        if (auto stmt = parseStatement(begin, end, context))
        {
            statements.push_back(std::move(stmt));
        }
        else
        {
            while (begin != end && begin->getTokenType() != lexer::TokenType::SemiColon
                   && begin->getTokenType() != lexer::TokenType::EndKeyword)
            {
                begin++;
            }
        }
        firstItem = false;
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

    return std::make_unique<Syntax::Block>(blockStart, begin, std::move(declarations), std::move(statements),
                                           std::move(placeReferences));
}

} // namespace coral66::parser
