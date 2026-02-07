/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */
#pragma once

#include <vector>

#include "lib/frontend/common/Message.hpp"

#include "Syntax.hpp"

namespace coral66::Parser
{

class Context
{
    const SourceInterface& m_sourceInterface;
    llvm::raw_ostream* m_reporter;
    std::size_t m_errorCount = 0;

public:
    Context(const SourceInterface& sourceInterface, llvm::raw_ostream* reporter = &llvm::errs());

    void log(const Message& message);
    [[nodiscard]] const SourceInterface& getSourceInterface() const;
    [[nodiscard]] std::size_t getErrorCount() const;
};

Syntax::Program parseProgram(const CSourceObject& sourceObject, llvm::raw_ostream* reporter = &llvm::errs(), bool* errors = nullptr);

std::unique_ptr<Syntax::Block> parseBlock(Lexer::CTokenIterator& begin, Lexer::CTokenIterator end, Context& context);
std::vector<std::unique_ptr<Syntax::Declaration>> parseDeclaration(Lexer::CTokenIterator& begin, Lexer::CTokenIterator end, Context& context);
std::unique_ptr<Syntax::Statement> parseStatement(Lexer::CTokenIterator& begin, Lexer::CTokenIterator end, Context& context);
std::unique_ptr<Syntax::Expression> parseExpression(Lexer::CTokenIterator& begin, Lexer::CTokenIterator end, Context& context);

}
