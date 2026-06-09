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
 * Structure for the second pass. This pass expects the symbol table to be fully resolved.
 * It will infer expression types, validate function calls
 * (matching arguments), check type conversions, etc.
 */

class TypeCheckingPass
{
    SemanticContext& m_context;
    const syntax::ProcedureDeclaration* m_currentProcedure = nullptr;

public:
    explicit TypeCheckingPass(SemanticContext& context);

    // Entry point for the type checking pass
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
    void visit(const syntax::CodeStatement& node);
    void visit(const syntax::ProcedureCallStatement& node);

    // Declarations
    void visit(const syntax::Declaration& node);
    void visit(const syntax::VariableDeclaration& node);
    void visit(const syntax::ProcedureDeclaration& node);
    void visit(const syntax::ArrayDeclaration& node);
    void visit(const syntax::TableDeclaration& node);
    void visit(const syntax::OverlayDeclaration& node);
    void visit(const syntax::ParameterDeclaration& node);

    // Expressions
    void visit(const syntax::Expression& node);
    void visit(const syntax::LiteralExpression& node);
    void visit(const syntax::IdentifierExpression& node);
    void visit(const syntax::BinaryExpression& node);
    void visit(const syntax::UnaryExpression& node);
    void visit(const syntax::PartWordExpression& node);
    void visit(const syntax::ConditionalExpression& node);
    void visit(const syntax::FunctionCallExpression& node);
    void visit(const syntax::ArrayAccessExpression& node);
    void visit(const syntax::LocationExpression& node);
    void visit(const syntax::TypeChangeExpression& node);

    // Helper functions
    IntrusiveVariantValue<Type> getCommonType(const Type& t1, const Type& t2) const;
    void validatePresets(const std::vector<lexer::CTokenIterator>& tokens, const Type& baseType,
                         const syntax::Expression* fixedTotal, const syntax::Expression* fixedFraction) const;
};

} // namespace coral66::semantics