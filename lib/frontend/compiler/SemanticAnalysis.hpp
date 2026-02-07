/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <functional>

#include "ProgramInterface.hpp"
#include "Syntax.hpp"
#include "Semantics.hpp"

namespace coral66::Semantics
{

class SemanticAnalysis final : public ProgramInterface
{
    const SourceInterface& m_sourceInterface;
    llvm::raw_ostream* m_reporter;
    bool* m_errors;
    std::vector<coral66::IntrVarPtr<Statement>> m_currentBlockStatements;
    coral66::IntrVarPtr<ExpressionBase> m_lastExpression;
    std::vector<coral66::IntrVarPtr<Useable>> m_globalDeclarations;
    bool m_isGlobalScope = true;
    std::unordered_map<std::string_view, const Useable*> m_symbolTable;
    
public:
    SemanticAnalysis(const SourceInterface& sourceInterface, llvm::raw_ostream* reporter = &llvm::errs(), bool* errors = nullptr);

    [[nodiscard]] const LanguageOptions& getLanguageOptions() const override { return m_sourceInterface.getLanguageOptions(); }

    friend Program analyse(const Syntax::Program& parseTree, CSourceObject&& cTokens, llvm::raw_ostream* reporter, bool* errors);

    void visit(const Syntax::Program& node);
    void visit(const Syntax::Block& node);

    // Dispatchers
    void visit(const Syntax::Declaration& node);
    void visit(const Syntax::Statement& node);
    void visit(const Syntax::Expression& node);

    // Specific Visitors - Declarations
    void visit(const Syntax::VariableDeclaration& node);
    void visit(const Syntax::ProcedureDeclaration& node);
    void visit(const Syntax::ArrayDeclaration& node);
    void visit(const Syntax::TableDeclaration& node);
    void visit(const Syntax::OverlayDeclaration& node);

    // Specific Visitors - Statements
    void visit(const Syntax::AssignmentStatement& node);
    void visit(const Syntax::IfStatement& node);
    void visit(const Syntax::GotoStatement& node);
    void visit(const Syntax::ForStatement& node);
    void visit(const Syntax::AnswerStatement& node);
    void visit(const Syntax::CodeStatement& node);
    void visit(const Syntax::ProcedureCallStatement& node);

    // Specific Visitors - Expressions
    void visit(const Syntax::LiteralExpression& node);
    void visit(const Syntax::IdentifierExpression& node);
    void visit(const Syntax::BinaryExpression& node);
    void visit(const Syntax::PartWordExpression& node);
};

} // namespace coral66::Semantics
