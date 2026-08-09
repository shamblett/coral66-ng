/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */
#pragma once

#include <map>
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

    struct ProcedureState
    {
        bool isTyped;
        bool hasAnswer;
    };
    std::vector<ProcedureState> m_procedureStack;

public:
    explicit Context(const source_interface& sourceInterface, llvm::raw_ostream* reporter = &llvm::errs());

    void log(const Message& message);
    [[nodiscard]] const source_interface& getSourceInterface() const;
    [[nodiscard]] std::size_t getErrorCount() const;

    void enterProcedure(const bool isTyped)
    {
        m_procedureStack.push_back({.isTyped = isTyped, .hasAnswer = false});
    }
    void exitProcedure()
    {
        m_procedureStack.pop_back();
    }
    bool isInsideProcedure() const
    {
        return !m_procedureStack.empty();
    }
    bool isInsideTypedProcedure() const
    {
        return !m_procedureStack.empty() && m_procedureStack.back().isTyped;
    }
    bool isInsideUntypedProcedure() const
    {
        return !m_procedureStack.empty() && !m_procedureStack.back().isTyped;
    }
    void markAnswerStatementFound()
    {
        if (!m_procedureStack.empty())
            m_procedureStack.back().hasAnswer = true;
    }
    bool isAnswerStatementFound() const
    {
        return !m_procedureStack.empty() && m_procedureStack.back().hasAnswer;
    }
};

/*
 * Main parser entry point
 */
syntax::Program parseProgram(const CSourceObject& sourceObject, llvm::raw_ostream* reporter = &llvm::errs(),
                             bool* errors = nullptr);

/*
 * Program structure parsers
 */

std::unique_ptr<syntax::Block> parseBlock(lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context);

std::vector<std::unique_ptr<syntax::Declaration>> parseDeclaration(
    lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context,
    std::map<lexer::CTokenIterator, std::unique_ptr<syntax::Expression>>* outAbsoluteAddresses = nullptr);
std::unique_ptr<syntax::Statement> parseStatement(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                  Context& context, bool isConsequence = false);
std::unique_ptr<syntax::Expression> parseCondition(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                   Context& context);
std::unique_ptr<syntax::Expression> parseVariable(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                  Context& context);
std::unique_ptr<syntax::Expression> parseWordReference(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                       Context& context);
std::unique_ptr<syntax::Expression> parseExpression(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                    Context& context);
std::unique_ptr<syntax::Communicator> parseCommunicator(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                        Context& context);
std::optional<std::vector<std::unique_ptr<syntax::Declaration>>>
    parseProcedureParameters(lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context);
std::unique_ptr<syntax::ProcedureDeclaration> parseProcedure(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                             Context& context);
} // namespace coral66::parser
