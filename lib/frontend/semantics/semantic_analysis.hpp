/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <functional>

#include "lib/frontend/syntax/syntax.hpp"
#include "program_interface.hpp"
#include "semantics.hpp"

namespace coral66::Semantics
{

class SemanticAnalysis final : public ProgramInterface
{
    const source_interface& m_sourceInterface;
    llvm::raw_ostream* m_reporter;
    bool* m_errors;
    std::vector<coral66::IntrVarPtr<Statement>> m_currentBlockStatements;
    coral66::IntrVarPtr<ExpressionBase> m_lastExpression;
    std::vector<coral66::IntrVarPtr<Usable>> m_globalDeclarations;
    bool m_isGlobalScope = true;
    std::unordered_map<std::string_view, const Usable*> m_symbolTable;

public:
    SemanticAnalysis(const source_interface& sourceInterface, llvm::raw_ostream* reporter = &llvm::errs(),
                     bool* errors = nullptr);

    friend Program analyse(const syntax::Program& parseTree, CSourceObject&& cTokens, llvm::raw_ostream* reporter,
                           bool* errors);

    void visit(const syntax::Program& node);
    void visit(const syntax::Block& node);

    // Dispatchers
    void visit(const syntax::Declaration& node);
    void visit(const syntax::Statement& node);
    void visit(const syntax::Expression& node);

    // Specific Visitors - Declarations
    void visit(const syntax::VariableDeclaration& node);
    void visit(const syntax::ProcedureDeclaration& node);
    void visit(const syntax::ArrayDeclaration& node);
    void visit(const syntax::TableDeclaration& node);
    void visit(const syntax::OverlayDeclaration& node);

    // Specific Visitors - Statements
    void visit(const syntax::DummyStatement& node);
    void visit(const syntax::BlockStatement& node);
    void visit(const syntax::CompoundStatement& node);
    void visit(const syntax::AssignmentStatement& node);
    void visit(const syntax::IfStatement& node);
    void visit(const syntax::GotoStatement& node);
    void visit(const syntax::ForStatement& node);
    void visit(const syntax::AnswerStatement& node);
    void visit(const syntax::CodeStatement& node);
    void visit(const syntax::ProcedureCallStatement& node);

    // Specific Visitors - Expressions
    void visit(const syntax::LiteralExpression& node);
    void visit(const syntax::IdentifierExpression& node);
    void visit(const syntax::BinaryExpression& node);
    void visit(const syntax::PartWordExpression& node);
};

} // namespace coral66::Semantics
