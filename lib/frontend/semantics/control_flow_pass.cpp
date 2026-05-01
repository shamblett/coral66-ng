/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "control_flow_pass.hpp"
#include "lib/frontend/common/messages/errors.hpp"
#include "semantic_support.hpp"

namespace coral66::semantics
{

ControlFlowPass::ControlFlowPass(SemanticContext& context) : m_context(context) {}

void ControlFlowPass::run(const syntax::Program& program)
{
    visit(program);
}

std::optional<std::int64_t> ControlFlowPass::evaluateConstantInteger(const syntax::Expression& expr)
{
    if (auto* lit = dynamic_cast<const syntax::LiteralExpression*>(&expr))
    {
        if (lit->getLiteral()->getType() == lexer::CToken::Integer
            || lit->getLiteral()->getType() == lexer::CToken::Floating)
        {
            if (auto* intVal = std::get_if<llvm::APSInt>(&lit->getLiteral()->getValue()))
            {
                return intVal->getExtValue();
            }
        }
        if (lit->begin()->getTokenType() == lexer::TokenType::OctalKeyword)
        {
            if (auto* intVal = std::get_if<llvm::APSInt>(&lit->getLiteral()->getValue()))
                return intVal->getExtValue();
        }
    }
    else if (auto* unary = dynamic_cast<const syntax::UnaryExpression*>(&expr))
    {
        auto opType = unary->getOp()->getTokenType();
        if (opType == lexer::TokenType::Plus || opType == lexer::TokenType::Minus)
        {
            auto val = evaluateConstantInteger(unary->getOperand());
            if (val)
            {
                if (opType == lexer::TokenType::Minus)
                    return -(*val);
                return *val;
            }
        }
    }
    else if (auto* binary = dynamic_cast<const syntax::BinaryExpression*>(&expr))
    {
        auto left = evaluateConstantInteger(binary->getLeft());
        auto right = evaluateConstantInteger(binary->getRight());
        if (left && right)
        {
            auto opType = binary->getOp()->getTokenType();
            if (opType == lexer::TokenType::Plus)
                return (*left) + (*right);
            if (opType == lexer::TokenType::Minus)
                return (*left) - (*right);
            if (opType == lexer::TokenType::Asterisk)
                return (*left) * (*right);
            if (opType == lexer::TokenType::Division)
            {
                if (*right != 0)
                    return (*left) / (*right);
            }
        }
    }
    return std::nullopt;
}

void ControlFlowPass::visit(const syntax::Program& node)
{
    if (node.hasBlock())
        visit(node.getBlock());
}

void ControlFlowPass::visit(const syntax::Block& node)
{
    for (const auto& decl : node.getDeclarations())
        visit(*decl);
    for (const auto& stmt : node.getStatements())
        visit(*stmt);
}

void ControlFlowPass::visit(const syntax::Declaration& node)
{
    if (auto* proc = dynamic_cast<const syntax::ProcedureDeclaration*>(&node))
        visit(*proc);
}

void ControlFlowPass::visit(const syntax::ProcedureDeclaration& node)
{
    auto* oldProc = m_currentProcedure;
    bool oldHasAnswer = m_hasAnswer;

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
    for (const auto& el : node.getElements())
    {
        visit(*el.expression);
        if (el.step)
            visit(*el.step);
        if (el.until)
            visit(*el.until);
        if (el.whileCondition)
            visit(*el.whileCondition);
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

    auto start = evaluateConstantInteger(node.getStartBit());
    auto width = evaluateConstantInteger(node.getWidth());

    if (start && width)
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
        auto total = evaluateConstantInteger(*node.getTotalBits());
        auto frac = evaluateConstantInteger(*node.getFractionBits());

        if (total && frac)
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
        auto it = m_context.resolvedIdentifiers.find(&node);
        if (it != m_context.resolvedIdentifiers.end())
        {
            const Usable* usable = it->second;
            const syntax::ArrayDeclaration* arrDecl = nullptr;
            if (auto* arr = usable->tryAs<ArrayDeclaration>())
            {
                arrDecl = dynamic_cast<const syntax::ArrayDeclaration*>(arr->getAstNode());
            }

            if (arrDecl && arrDecl->getDimensions().size() == node.getIndices().size())
            {
                for (size_t i = 0; i < node.getIndices().size(); ++i)
                {
                    auto idxVal = evaluateConstantInteger(*node.getIndices()[i]);
                    auto lowerVal = evaluateConstantInteger(*arrDecl->getDimensions()[i].first);
                    auto upperVal = evaluateConstantInteger(*arrDecl->getDimensions()[i].second);

                    if (idxVal && lowerVal && upperVal)
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
