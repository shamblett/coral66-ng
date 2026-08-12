/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include "preprocessor_syntax.hpp"

/**
 * Defines the Preprocessor Parser interface.
 *
 * This file contains the definitions for parsing preprocessor directives,
 * extracting macro definitions, and forming preprocessor groups from the token stream.
 */

namespace coral66
{
class Message;

namespace pp
{
class Context final
{
    const source_interface& m_sourceInterface;
    llvm::raw_ostream* m_reporter;
    std::size_t m_errorCount = 0;

public:
    explicit Context(const source_interface& sourceInterface, llvm::raw_ostream* reporter = &llvm::errs());

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    Context(Context&&) = delete;
    Context& operator=(Context&&) = delete;

    void log(const Message& message);

    [[nodiscard]] std::size_t getErrorCount() const;

    [[nodiscard]] const source_interface& getSourceInterface() const;
};

File parseMacro(lexer::PPTokenIterator& begin, lexer::PPTokenIterator end, Context& context);

Group parseGroup(lexer::PPTokenIterator& begin, lexer::PPTokenIterator end, Context& context, bool inIf = false);

ControlLine parseControlLine(lexer::PPTokenIterator& begin, lexer::PPTokenIterator end, Context& context);

} // namespace pp
} // namespace coral66
