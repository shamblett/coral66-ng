/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */
#pragma once

#include <vector>

#include "lib/frontend/common/messages/message.hpp"
#include "lib/frontend/syntax/syntax.hpp"

namespace coral66::parser
{

class Context
{
    const source_interface& m_sourceInterface;
    llvm::raw_ostream* m_reporter;
    std::size_t m_errorCount = 0;

public:
    explicit Context(const source_interface& sourceInterface, llvm::raw_ostream* reporter = &llvm::errs());

    void log(const Message& message);
    [[nodiscard]] const source_interface& getSourceInterface() const;
    [[nodiscard]] std::size_t getErrorCount() const;
};

/*
 * Main parser entry point
 */
Syntax::Program parseProgram(const CSourceObject& sourceObject, llvm::raw_ostream* reporter = &llvm::errs(),
                             bool* errors = nullptr);

/*
 * Program structure parsers
 */

std::unique_ptr<Syntax::Block> parseBlock(lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context);

std::vector<std::unique_ptr<Syntax::Declaration>> parseDeclaration(lexer::CTokenIterator& begin,
                                                                   lexer::CTokenIterator end, Context& context);
std::unique_ptr<Syntax::Statement> parseStatement(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                  Context& context);
std::unique_ptr<Syntax::Expression> parseExpression(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                    Context& context);
std::unique_ptr<Syntax::Communicator> parseCommunicator(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                        Context& context);
} // namespace coral66::parser
