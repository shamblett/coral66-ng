/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 02/05/2026
 */


#include "lib/frontend/common/messages/errors.hpp"
#include "semantic_support.hpp"

#include "control_flow_pass.hpp"

namespace coral66::semantics
{

ControlFlowPass::ControlFlowPass(SemanticContext& context) : m_context(context) {}

void ControlFlowPass::run(const syntax::Program& program)
{
    visit(program);
}


void ControlFlowPass::visit(const syntax::Program& node)
{
    if (node.hasBlock())
        visit(node.getBlock());
}

void ControlFlowPass::visit(const syntax::Block& node)
{
    for (const auto& declaration : node.getDeclarations())
        visit(*declaration);
    for (const auto& stmt : node.getStatements())
        visit(*stmt);
}

void ControlFlowPass::visit(const syntax::Declaration& node)
{
    if (auto* proc = dynamic_cast<const syntax::ProcedureDeclaration*>(&node))
        visit(*proc);
    else if (auto* arr = dynamic_cast<const syntax::ArrayDeclaration*>(&node))
        visit(*arr);
    else if (auto* tab = dynamic_cast<const syntax::TableDeclaration*>(&node))
        visit(*tab);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void ControlFlowPass::visit(const syntax::ArrayDeclaration& node)
{
    for (const auto& [fst, snd] : node.getDimensions())
    {
        auto lower = evaluateConstantInteger(*fst);

        if (auto upper = evaluateConstantInteger(*snd); lower && upper)
        {
            if (*lower > *upper)
            {
                m_context.log(
                    errors::semantics::INVALID_ARRAY_BOUNDS.args(*node.begin(), m_context.source, *node.begin()));
                DiagnosticCollector::addDiagnostic(errors::semantics::INVALID_ARRAY_BOUNDS);
            }
        }
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
void ControlFlowPass::visit(const syntax::TableDeclaration& node)
{
    if (node.getWidth())
    {
        if (const auto width = evaluateConstantInteger(*node.getWidth()))
        {
            if (*width <= 0)
            {
                m_context.log(
                    errors::semantics::TABLE_INVALID_DIMENSIONS.args(*node.begin(), m_context.source, *node.begin()));
                DiagnosticCollector::addDiagnostic(errors::semantics::TABLE_INVALID_DIMENSIONS);
            }
        }
    }
    if (node.getLength())
    {
        if (const auto length = evaluateConstantInteger(*node.getLength()))
        {
            if (*length <= 0)
            {
                m_context.log(
                    errors::semantics::TABLE_INVALID_DIMENSIONS.args(*node.begin(), m_context.source, *node.begin()));
                DiagnosticCollector::addDiagnostic(errors::semantics::TABLE_INVALID_DIMENSIONS);
            }
        }
    }
}

void ControlFlowPass::visit(const syntax::ProcedureDeclaration& node)
{
    auto* oldProc = m_currentProcedure;
    const bool oldHasAnswer = m_hasAnswer;

    m_currentProcedure = &node;
    m_hasAnswer = false;

    visit(node.getBody());

    if (node.getTypeToken() && !m_hasAnswer)
    {
        m_context.log(
            errors::parser::MISSING_ANSWER_STATEMENT.args(*node.getName(), m_context.source, *node.getName()));
        DiagnosticCollector::addDiagnostic(errors::parser::MISSING_ANSWER_STATEMENT);
    }

    m_currentProcedure = oldProc;
    m_hasAnswer = oldHasAnswer;
}

void ControlFlowPass::visit(const syntax::Statement& node)
{
    if (auto* block = dynamic_cast<const syntax::BlockStatement*>(&node))
        visit(*block);
    else if (auto* comp = dynamic_cast<const syntax::CompoundStatement*>(&node))
        visit(*comp);
    else if (auto* ifStmt = dynamic_cast<const syntax::IfStatement*>(&node))
        visit(*ifStmt);
    else if (auto* forStmt = dynamic_cast<const syntax::ForStatement*>(&node))
        visit(*forStmt);
    else if (auto* assignStmt = dynamic_cast<const syntax::AssignmentStatement*>(&node))
        visit(*assignStmt);
    else if (auto* gotoStmt = dynamic_cast<const syntax::GotoStatement*>(&node))
        visit(*gotoStmt);
    else if (auto* answerStmt = dynamic_cast<const syntax::AnswerStatement*>(&node))
        visit(*answerStmt);
    else if (auto* callStmt = dynamic_cast<const syntax::ProcedureCallStatement*>(&node))
        visit(*callStmt);
}

void ControlFlowPass::visit(const syntax::BlockStatement& node)
{
    if (node.hasBlock())
        visit(node.getBlock());
}

void ControlFlowPass::visit(const syntax::CompoundStatement& node)
{
    for (const auto& stmt : node.getStatements())
        visit(*stmt);
}

void ControlFlowPass::visit(const syntax::IfStatement& node)
{
    visit(node.getCondition());
    visit(node.getThenBranch());
    if (node.getElseBranch())
        visit(*node.getElseBranch());
}

void ControlFlowPass::visit(const syntax::ForStatement& node)
{
    visit(node.getVariable());
    for (const auto& [expression, step, until, whileCondition] : node.getElements())
    {
        visit(*expression);
        if (step)
            visit(*step);
        if (until)
            visit(*until);
        if (whileCondition)
            visit(*whileCondition);
    }
    visit(node.getBody());
}

void ControlFlowPass::visit(const syntax::AssignmentStatement& node)
{
    visit(node.getVariable());
    visit(node.getExpression());
}

void ControlFlowPass::visit(const syntax::GotoStatement& node)
{
    if (node.getIndex())
        visit(*node.getIndex());

    // Gotos into blocks are forbidden. Target label must be at or above our lexical scope.
    if (const auto usable = m_context.resolvedIdentifiers.find(&node); usable != m_context.resolvedIdentifiers.end())
    {
        if (const auto sw = usable->second->tryAs<SwitchDeclaration>())
        {
            if (node.getIndex())
            {
                if (const auto constIdx = evaluateConstantInteger(*node.getIndex()))
                {
                    // Coral 66 switch indices are 1-based
                    if (*constIdx < 1 || *constIdx > static_cast<std::int64_t>(sw->getLabels().size()))
                    {
                         m_context.log(errors::semantics::SWITCH_INDEX_OUT_OF_BOUNDS.args(*node.getIndex()->begin(), m_context.source, *node.getIndex()->begin()));
                         DiagnosticCollector::addDiagnostic(errors::semantics::SWITCH_INDEX_OUT_OF_BOUNDS);
                    }
                }
            }
        }
    }
}

void ControlFlowPass::visit(const syntax::AnswerStatement& node)
{
    visit(node.getExpression());
    if (m_currentProcedure && m_currentProcedure->getTypeToken())
    {
        m_hasAnswer = true;
    }
}

void ControlFlowPass::visit(const syntax::ProcedureCallStatement& node)
{
    visit(node.getExpression());
}

void ControlFlowPass::visit(const syntax::Expression& node)
{
    if (auto* pw = dynamic_cast<const syntax::PartWordExpression*>(&node))
        visit(*pw);
    else if (auto* ty = dynamic_cast<const syntax::TypeChangeExpression*>(&node))
        visit(*ty);
    else if (auto* arr = dynamic_cast<const syntax::ArrayAccessExpression*>(&node))
        visit(*arr);
    else if (auto* bin = dynamic_cast<const syntax::BinaryExpression*>(&node))
        visit(*bin);
    else if (auto* un = dynamic_cast<const syntax::UnaryExpression*>(&node))
        visit(*un);
    else if (auto* cond = dynamic_cast<const syntax::ConditionalExpression*>(&node))
        visit(*cond);
    else if (auto* func = dynamic_cast<const syntax::FunctionCallExpression*>(&node))
        visit(*func);
}

void ControlFlowPass::visit(const syntax::PartWordExpression& node)
{
    visit(node.getBase());
    visit(node.getStartBit());
    visit(node.getWidth());

    const auto start = evaluateConstantInteger(node.getStartBit());

    if (const auto width = evaluateConstantInteger(node.getWidth()); start && width)
    {
        if (*start < 0 || *width < 0 || (*start + *width) > 24) // Assuming 24-bit target word
        {
            m_context.log(errors::semantics::INVALID_BIT_RANGE.args(*node.begin(), m_context.source, *node.begin()));
            DiagnosticCollector::addDiagnostic(errors::semantics::INVALID_BIT_RANGE);
        }
    }
}

void ControlFlowPass::visit(const syntax::TypeChangeExpression& node)
{
    visit(node.getExpression());
    if (node.getTotalBits())
        visit(*node.getTotalBits());
    if (node.getFractionBits())
        visit(*node.getFractionBits());

    if (node.getTotalBits() && node.getFractionBits())
    {
        const auto total = evaluateConstantInteger(*node.getTotalBits());

        if (const auto frac = evaluateConstantInteger(*node.getFractionBits()); total && frac)
        {
            if (*total <= 0 || *frac < 0 || *frac > *total)
            {
                m_context.log(
                    errors::semantics::INVALID_SCALE_FOR_FIXED.args(*node.begin(), m_context.source, *node.begin()));
                DiagnosticCollector::addDiagnostic(errors::semantics::INVALID_SCALE_FOR_FIXED);
            }
        }
    }
}

void ControlFlowPass::visit(const syntax::ArrayAccessExpression& node)
{
    for (const auto& idx : node.getIndices())
        visit(*idx);

    if (node.getArray())
    {
        if (const auto it = m_context.resolvedIdentifiers.find(&node); it != m_context.resolvedIdentifiers.end())
        {
            const Usable* usable = it->second;
            const syntax::ArrayDeclaration* arrDeclaration = nullptr;
            if (auto* arr = usable->tryAs<ArrayDeclaration>())
            {
                arrDeclaration = dynamic_cast<const syntax::ArrayDeclaration*>(arr->getAstNode());
            }

            if (arrDeclaration && arrDeclaration->getDimensions().size() == node.getIndices().size())
            {
                for (size_t i = 0; i < node.getIndices().size(); ++i)
                {
                    auto idxVal = evaluateConstantInteger(*node.getIndices()[i]);
                    auto lowerVal = evaluateConstantInteger(*arrDeclaration->getDimensions()[i].first);

                    if (auto upperVal = evaluateConstantInteger(*arrDeclaration->getDimensions()[i].second);
                        idxVal && lowerVal && upperVal)
                    {
                        if (*idxVal < *lowerVal || *idxVal > *upperVal)
                        {
                            m_context.log(errors::semantics::ARRAY_INDEX_OUT_OF_BOUNDS.args(
                                *node.getIndices()[i]->begin(), m_context.source, *node.getIndices()[i]->begin()));
                            DiagnosticCollector::addDiagnostic(errors::semantics::ARRAY_INDEX_OUT_OF_BOUNDS);
                        }
                    }
                }
            }
        }
    }
}

void ControlFlowPass::visit(const syntax::BinaryExpression& node)
{
    visit(node.getLeft());
    visit(node.getRight());
}

void ControlFlowPass::visit(const syntax::UnaryExpression& node)
{
    visit(node.getOperand());
}

void ControlFlowPass::visit(const syntax::ConditionalExpression& node)
{
    visit(node.getCondition());
    visit(node.getThenBranch());
    visit(node.getElseBranch());
}

void ControlFlowPass::visit(const syntax::FunctionCallExpression& node)
{
    for (const auto& arg : node.getArguments())
        visit(*arg);
}

} // namespace coral66::semantics
