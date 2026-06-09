/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 02/05/2026
 */

#pragma once

#include <optional>

#include "lib/frontend/syntax/syntax.hpp"
#include "semantic_context.hpp"

namespace coral66::semantics
{

/*
 * Structure for the third pass. This checks for valid goto label boundaries, unreachable code,
 * bounds checking for array access, part word accesses, fixed scaling, and procedure return paths.
 */


std::optional<std::int64_t> evaluateConstantInteger(const syntax::Expression& expr);

class ControlFlowPass
{
    SemanticContext& m_context;
    const syntax::ProcedureDeclaration* m_currentProcedure = nullptr;
    bool m_hasAnswer = false;


public:
    explicit ControlFlowPass(SemanticContext& context);

    // Entry point for the control flow validation pass
    void run(const syntax::Program& program);

    void visit(const syntax::Program& node);
    void visit(const syntax::Block& node);
    void visit(const syntax::Statement& node);
    void visit(const syntax::BlockStatement& node);
    void visit(const syntax::CompoundStatement& node);
    void visit(const syntax::IfStatement& node);
    void visit(const syntax::ForStatement& node);
    void visit(const syntax::AssignmentStatement& node);
    void visit(const syntax::GotoStatement& node);
    void visit(const syntax::AnswerStatement& node);
    void visit(const syntax::CodeStatement& node);
    void visit(const syntax::ProcedureCallStatement& node);

    void visit(const syntax::Declaration& node);
    void visit(const syntax::ProcedureDeclaration& node);
    void visit(const syntax::ArrayDeclaration& node);
    void visit(const syntax::TableDeclaration& node);

    void visit(const syntax::Expression& node);
    void visit(const syntax::PartWordExpression& node);
    void visit(const syntax::TypeChangeExpression& node);
    void visit(const syntax::ArrayAccessExpression& node);
    void visit(const syntax::BinaryExpression& node);
    void visit(const syntax::UnaryExpression& node);
    void visit(const syntax::ConditionalExpression& node);
    void visit(const syntax::FunctionCallExpression& node);
};

} // namespace coral66::semantics
