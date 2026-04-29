/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include "lib/frontend/syntax/syntax.hpp"
#include "semantic_context.hpp"

namespace coral66::semantics
{

/*
 * Structure for the first pass. This pass will walk the syntax::Program,
 * establish scopes in the symbol table, and resolve/bind identifiers to their
 * declarations without worrying about expression type rules.
 */

class SymbolResolutionPass
{
    SemanticContext& m_context;

public:
    explicit SymbolResolutionPass(SemanticContext& context);

    // Entry point for the symbol resolution pass
    void run(const syntax::Program& program);

private:
    // Statements & Blocks
    void visit(const syntax::Program& node);
    void visit(const syntax::Block& node);
    void visit(const syntax::Statement& node);
    void visit(const syntax::CompoundStatement& node);
    void visit(const syntax::BlockStatement& node);
    void visit(const syntax::IfStatement& node);
    void visit(const syntax::ForStatement& node);
    void visit(const syntax::AssignmentStatement& node);
    void visit(const syntax::GotoStatement& node);
    void visit(const syntax::AnswerStatement& node);
    void visit(const syntax::ProcedureCallStatement& node);

    // Declarations
    void visit(const syntax::Declaration& node);
    void visit(const syntax::VariableDeclaration& node);
    void visit(const syntax::ProcedureDeclaration& node);
    void visit(const syntax::ArrayDeclaration& node);
    void visit(const syntax::TableDeclaration& node);
    void visit(const syntax::TableElementDeclaration& node);
    void visit(const syntax::OverlayDeclaration& node);
    void visit(const syntax::SwitchDeclaration& node);
    void visit(const syntax::ParameterDeclaration& node);

    // Expressions
    void visit(const syntax::Expression& node);
    void visit(const syntax::IdentifierExpression& node);
    void visit(const syntax::BinaryExpression& node);
    void visit(const syntax::UnaryExpression& node);
    void visit(const syntax::PartWordExpression& node);
    void visit(const syntax::ConditionalExpression& node);
    void visit(const syntax::FunctionCallExpression& node);
    void visit(const syntax::ArrayAccessExpression& node);
    void visit(const syntax::LocationExpression& node);
    void visit(const syntax::TypeChangeExpression& node);

    IntrusiveVariantValue<Type> createTypeFromToken(std::optional<lexer::CTokenIterator> token);
};

} // namespace coral66::semantics