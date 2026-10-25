/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 02/05/2026
 */

#include "type_checking_pass.hpp"

#include <cmath>

#include "control_flow_pass.hpp"
#include "lib/frontend/common/messages/errors.hpp"
#include "semantic_support.hpp"

namespace coral66::semantics
{

static const Type& resolveExprType(const Type& t)
{
    if (const auto p = t.tryAs<ProcedureType>())
    {
        if (p->getReturnType())
            return *p->getReturnType();
    }
    return t;
}

static bool isNumericType(const Type& t)
{
    const Type& rt = resolveExprType(t);
    if (const auto p = rt.tryAs<PrimitiveType>())
        return p->getKind() == PrimitiveType::Integer || p->getKind() == PrimitiveType::Floating;
    return rt.is<FixedType>();
}

static bool isBitsType(const Type& t)
{
    const Type& rt = resolveExprType(t);
    if (const auto p = rt.tryAs<PrimitiveType>())
        return p->getKind() == PrimitiveType::Bits;
    return false;
}

static bool isPointerType(const Type& t)
{
    const Type& rt = resolveExprType(t);
    if (const auto p = rt.tryAs<PrimitiveType>())
        return p->getKind() == PrimitiveType::Pointer;
    return false;
}

static bool isSwitchUsable(const Usable* usable)
{
    if (!usable)
        return false;
    if (usable->is<SwitchDeclaration>())
        return true;
    if (auto* var = usable->tryAs<VariableDeclaration>())
        return var->getType().is<SwitchType>();
    return false;
}

static bool isSwitch(const syntax::Expression& expr, const SemanticContext& ctx)
{
    if (auto* id = dynamic_cast<const syntax::IdentifierExpression*>(&expr))
    {
        if (const auto it = ctx.resolvedIdentifiers.find(id); it != ctx.resolvedIdentifiers.end())
        {
            return isSwitchUsable(it->second);
        }
        return false;
    }
    if (auto* arr = dynamic_cast<const syntax::ArrayAccessExpression*>(&expr))
    {
        if (const auto it = ctx.resolvedIdentifiers.find(arr); it != ctx.resolvedIdentifiers.end())
        {
            return isSwitchUsable(it->second);
        }
        return false;
    }
    if (auto* pw = dynamic_cast<const syntax::PartWordExpression*>(&expr))
    {
        return isSwitch(pw->getBase(), ctx);
    }
    return false;
}

static bool isLValue(const syntax::Expression& expr, const SemanticContext& ctx)
{
    if (auto* id = dynamic_cast<const syntax::IdentifierExpression*>(&expr))
    {
        if (const auto it = ctx.resolvedIdentifiers.find(id); it != ctx.resolvedIdentifiers.end())
        {
            const Usable* usable = it->second;
            if (isSwitchUsable(usable))
                return false;
            return usable->is<VariableDeclaration>() || usable->is<ArrayDeclaration>() || usable->is<TableDeclaration>()
                   || usable->is<TableElementDeclaration>();
        }
        return false;
    }
    if (auto* arr = dynamic_cast<const syntax::ArrayAccessExpression*>(&expr))
    {
        if (arr->getArray() == nullptr)
            return true;
        if (const auto it = ctx.resolvedIdentifiers.find(arr); it != ctx.resolvedIdentifiers.end())
        {
            const Usable* usable = it->second;
            if (isSwitchUsable(usable))
                return false;
            return usable->is<VariableDeclaration>() || usable->is<ArrayDeclaration>() || usable->is<TableDeclaration>()
                   || usable->is<TableElementDeclaration>();
        }
        return false;
    }
    if (auto* pw = dynamic_cast<const syntax::PartWordExpression*>(&expr))
        return isLValue(pw->getBase(), ctx);
    return false;
}

TypeCheckingPass::TypeCheckingPass(SemanticContext& context) : m_context(context) {}

void TypeCheckingPass::run(const syntax::Program& program)
{
    visit(program);
}

void TypeCheckingPass::visit(const syntax::Program& node)
{
    for (const auto& comm : node.getCommunicators())
    {
        if (auto* commonCommunicator = dynamic_cast<const syntax::CommonCommunicator*>(comm.get()))
        {
            for (const auto& item : commonCommunicator->getItems())
            {
                visit(*item);
            }
        }
    }

    if (node.hasBlock())
    {
        visit(node.getBlock());
    }
}

void TypeCheckingPass::visit(const syntax::Block& node)
{
    m_blockDepth++;
    for (const auto& declaration : node.getDeclarations())
    {
        visit(*declaration);
    }
    for (const auto& stmt : node.getStatements())
    {
        visit(*stmt);
    }
    m_blockDepth--;
}

void TypeCheckingPass::visit(const syntax::Statement& node)
{
    if (auto* blockStmt = dynamic_cast<const syntax::BlockStatement*>(&node))
        visit(*blockStmt);
    else if (auto* compStmt = dynamic_cast<const syntax::CompoundStatement*>(&node))
        visit(*compStmt);
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

void TypeCheckingPass::visit(const syntax::BlockStatement& node)
{
    if (node.hasBlock())
        visit(node.getBlock());
}

void TypeCheckingPass::visit(const syntax::CompoundStatement& node)
{
    for (const auto& stmt : node.getStatements())
        visit(*stmt);
}

void TypeCheckingPass::visit(const syntax::IfStatement& node)
{
    visit(node.getCondition());
    if (const auto it = m_context.expressionTypes.find(&node.getCondition());
        it != m_context.expressionTypes.end() && !it->second->is<ErrorType>())
    {
        if (const auto prim = resolveExprType(*it->second).tryAs<PrimitiveType>();
            !prim || prim->getKind() != PrimitiveType::Integer)
        {
            m_context.log(errors::semantics::EXPECTED_INTEGER_TYPE.args(*node.getCondition().begin(), m_context.source,
                                                                        *node.getCondition().begin()));
            DiagnosticCollector::addDiagnostic(errors::semantics::EXPECTED_INTEGER_TYPE);
        }
    }

    visit(node.getThenBranch());
    if (node.getElseBranch())
        visit(*node.getElseBranch());
}

void TypeCheckingPass::visit(const syntax::ForStatement& node)
{
    visit(node.getVariable());

    if (const auto varIt = m_context.expressionTypes.find(&node.getVariable());
        varIt != m_context.expressionTypes.end() && !varIt->second->is<ErrorType>())
    {
        if (!isNumericType(*varIt->second))
        {
            m_context.log(errors::semantics::EXPECTED_NUMERIC_TYPE.args(*node.getVariable().begin(), m_context.source,
                                                                        *node.getVariable().begin()));
            DiagnosticCollector::addDiagnostic(errors::semantics::EXPECTED_NUMERIC_TYPE);
        }
    }

    for (const auto& [expression, step, until, whileCondition] : node.getElements())
    {
        visit(*expression);
        if (step)
        {
            visit(*step);
            if (auto itStep = m_context.expressionTypes.find(step.get());
                itStep != m_context.expressionTypes.end() && !itStep->second->is<ErrorType>())
            {
                if (!isNumericType(*itStep->second))
                {
                    m_context.log(errors::semantics::EXPECTED_NUMERIC_TYPE.args(*step->begin(), m_context.source,
                                                                                *step->begin()));
                    DiagnosticCollector::addDiagnostic(errors::semantics::EXPECTED_NUMERIC_TYPE);
                }
            }
        }
        if (until)
        {
            visit(*until);
            if (auto itUntil = m_context.expressionTypes.find(until.get());
                itUntil != m_context.expressionTypes.end() && !itUntil->second->is<ErrorType>())
            {
                if (!isNumericType(*itUntil->second))
                {
                    m_context.log(errors::semantics::EXPECTED_NUMERIC_TYPE.args(*until->begin(), m_context.source,
                                                                                *until->begin()));
                    DiagnosticCollector::addDiagnostic(errors::semantics::EXPECTED_NUMERIC_TYPE);
                }
            }
        }
        if (whileCondition)
        {
            visit(*whileCondition);
            if (auto it = m_context.expressionTypes.find(whileCondition.get());
                it != m_context.expressionTypes.end() && !it->second->is<ErrorType>())
            {
                if (const auto prim = resolveExprType(*it->second).tryAs<PrimitiveType>();
                    !prim || prim->getKind() != PrimitiveType::Integer)
                {
                    m_context.log(errors::semantics::EXPECTED_INTEGER_TYPE.args(
                        *whileCondition->begin(), m_context.source, *whileCondition->begin()));
                    DiagnosticCollector::addDiagnostic(errors::semantics::EXPECTED_INTEGER_TYPE);
                }
            }
        }
    }
    visit(node.getBody());
}

void TypeCheckingPass::visit(const syntax::AssignmentStatement& node)
{
    visit(node.getVariable());
    visit(node.getExpression());

    if (isSwitch(node.getVariable(), m_context))
    {
        m_context.log(errors::semantics::CANNOT_ASSIGN_TO_SWITCH.args(*node.getVariable().begin(), m_context.source,
                                                                      *node.getVariable().begin()));
        DiagnosticCollector::addDiagnostic(errors::semantics::CANNOT_ASSIGN_TO_SWITCH);
        return;
    }

    if (!isLValue(node.getVariable(), m_context))
    {
        m_context.log(errors::semantics::CANNOT_ASSIGN_TO_CONST.args(*node.getVariable().begin(), m_context.source,
                                                                     *node.getVariable().begin()));
        DiagnosticCollector::addDiagnostic(errors::semantics::CANNOT_ASSIGN_TO_CONST);
    }

    // Check type compatibility
    const auto itL = m_context.expressionTypes.find(&node.getVariable());

    if (const auto itR = m_context.expressionTypes.find(&node.getExpression());
        itL != m_context.expressionTypes.end() && itR != m_context.expressionTypes.end())
    {
        auto lType = itL->second;
        auto rType = itR->second;

        if (lType->isValue())
        {
            m_context.log(errors::semantics::CANNOT_ASSIGN_TO_CONST.args(*node.getVariable().begin(), m_context.source,
                                                                         *node.getVariable().begin()));
            DiagnosticCollector::addDiagnostic(errors::semantics::CANNOT_ASSIGN_TO_CONST);
        }

        if (!lType->is<ErrorType>() && !rType->is<ErrorType>())
        {
            bool mismatch = false;

            if (isBitsType(*lType) && isBitsType(*rType))
            {
                // Match
            }
            else if (isPointerType(*lType))
            {
                // Accept Pointer to Pointer or Integer to Pointer
                if (isPointerType(*rType)
                    || (resolveExprType(*rType).tryAs<PrimitiveType>()
                        && resolveExprType(*rType).as<PrimitiveType>().getKind() == PrimitiveType::Integer))
                {
                    // Match
                }
                else
                {
                    mismatch = true;
                }
            }
            else if (isPointerType(*rType))
            {
                mismatch = true;
            }
            else if (isBitsType(*lType) && isNumericType(*rType))
            {
                if (const auto p = resolveExprType(*rType).tryAs<PrimitiveType>();
                    !p || p->getKind() == PrimitiveType::Floating)
                    mismatch = true;
            }
            else if (isBitsType(*rType) && isNumericType(*lType))
            {
                if (const auto p = resolveExprType(*lType).tryAs<PrimitiveType>();
                    !p || p->getKind() == PrimitiveType::Floating)
                    mismatch = true;
            }
            else if (isBitsType(*lType) || isBitsType(*rType))
            {
                mismatch = true;
            }
            else if (!isNumericType(*lType) || !isNumericType(*rType))
            {
                mismatch = true;
            }
            else if (const auto lf = resolveExprType(*lType).tryAs<FixedType>(); lf)
            {
                if (const auto rf = resolveExprType(*rType).tryAs<FixedType>(); rf)
                {
                    if (lf->getTotalBits() != rf->getTotalBits() || lf->getFractionalBits() != rf->getFractionalBits())
                    {
                        if (dynamic_cast<const syntax::ArrayAccessExpression*>(&node.getVariable()) != nullptr)
                        {
                            mismatch = true;
                        }
                    }
                }
            }

            if (mismatch)
            {
                m_context.log(errors::semantics::TYPE_MISMATCH.args(*node.getVariable().begin(), m_context.source,
                                                                    *node.getVariable().begin()));
                DiagnosticCollector::addDiagnostic(errors::semantics::TYPE_MISMATCH);
            }
        }
    }
}

void TypeCheckingPass::visit(const syntax::GotoStatement& node)
{
    if (node.getIndex())
    {
        visit(*node.getIndex());
        if (const auto it = m_context.expressionTypes.find(node.getIndex()); it != m_context.expressionTypes.end())
        {
            if (const auto prim = resolveExprType(*it->second).tryAs<PrimitiveType>();
                prim && prim->getKind() != PrimitiveType::Integer)
            {
                m_context.log(errors::semantics::EXPECTED_INTEGER_TYPE.args(*node.getIndex()->begin(), m_context.source,
                                                                            *node.getIndex()->begin()));
                DiagnosticCollector::addDiagnostic(errors::semantics::EXPECTED_INTEGER_TYPE);
            }
        }
    }
}

void TypeCheckingPass::visit(const syntax::AnswerStatement& node)
{
    visit(node.getExpression());

    if (m_currentProcedure)
    {
        if (!m_currentProcedure->getTypeToken())
        {
            m_context.log(errors::semantics::TYPE_MISMATCH.args(*node.getExpression().begin(), m_context.source,
                                                                *node.getExpression().begin()));
            DiagnosticCollector::addDiagnostic(errors::semantics::TYPE_MISMATCH);
        }
        else
        {
            if (const auto it = m_context.expressionTypes.find(&node.getExpression());
                it != m_context.expressionTypes.end() && !it->second->is<ErrorType>())
            {
                if (!isNumericType(*it->second) && !isPointerType(*it->second))
                {
                    m_context.log(errors::semantics::TYPE_MISMATCH.args(*node.getExpression().begin(), m_context.source,
                                                                        *node.getExpression().begin()));
                    DiagnosticCollector::addDiagnostic(errors::semantics::TYPE_MISMATCH);
                }
            }
        }
    }
}

void TypeCheckingPass::visit(const syntax::ProcedureCallStatement& node)
{
    visit(node.getExpression());
}

void TypeCheckingPass::visit(const syntax::Declaration& node)
{
    if (auto* variableDeclaration = dynamic_cast<const syntax::VariableDeclaration*>(&node))
        visit(*variableDeclaration);
    else if (auto* procedureDeclaration = dynamic_cast<const syntax::ProcedureDeclaration*>(&node))
        visit(*procedureDeclaration);
    else if (auto* arrayDeclaration = dynamic_cast<const syntax::ArrayDeclaration*>(&node))
        visit(*arrayDeclaration);
    else if (auto* tableDeclaration = dynamic_cast<const syntax::TableDeclaration*>(&node))
        visit(*tableDeclaration);
    else if (auto* overlayDeclaration = dynamic_cast<const syntax::OverlayDeclaration*>(&node))
        visit(*overlayDeclaration);
    else if (auto* parameterDeclaration = dynamic_cast<const syntax::ParameterDeclaration*>(&node))
        visit(*parameterDeclaration);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void TypeCheckingPass::visit(const syntax::VariableDeclaration& node)
{
    if (node.hasPreset())
    {
        if (m_currentProcedure && m_currentProcedure->isRecursive())
        {
            m_context.log(
                errors::semantics::PRESET_IN_RECURSIVE_PROCEDURE.args(*node.begin(), m_context.source, *node.begin()));
            DiagnosticCollector::addDiagnostic(errors::semantics::PRESET_IN_RECURSIVE_PROCEDURE);
        }
        else if (m_blockDepth > 1)
        {
            m_context.log(
                errors::semantics::PRESET_IN_INNER_BLOCK.args(*node.begin(), m_context.source, *node.begin()));
            DiagnosticCollector::addDiagnostic(errors::semantics::PRESET_IN_INNER_BLOCK);
        }

        IntrusiveVariantValue type{PrimitiveType{PrimitiveType::Integer}};
        if (node.getType()->getTokenType() == lexer::TokenType::FloatingKeyword)
            type = IntrusiveVariantValue(PrimitiveType{PrimitiveType::Floating});
        else if (node.getType()->getTokenType() == lexer::TokenType::FixedKeyword)
            type = IntrusiveVariantValue(FixedType(24, 0));

        validatePresets(node.getPresetTokens(), *type, node.getFixedTotalBits(), node.getFixedFractionBits());
    }
}

void TypeCheckingPass::visit(const syntax::ArrayDeclaration& node)
{
    for (const auto& [fst, snd] : node.getDimensions())
    {
        visit(*fst);
        visit(*snd);

        for (const auto* expr : {fst.get(), snd.get()})
        {
            if (auto it = m_context.expressionTypes.find(expr); it != m_context.expressionTypes.end())
            {
                if (const auto prim = resolveExprType(*it->second).tryAs<PrimitiveType>();
                    prim && prim->getKind() != PrimitiveType::Integer)
                {
                    m_context.log(errors::semantics::EXPECTED_INTEGER_TYPE.args(*expr->begin(), m_context.source,
                                                                                *expr->begin()));
                    DiagnosticCollector::addDiagnostic(errors::semantics::EXPECTED_INTEGER_TYPE);
                }
            }
        }
    }

    if (node.hasPreset())
    {
        if (m_currentProcedure && m_currentProcedure->isRecursive())
        {
            m_context.log(
                errors::semantics::PRESET_IN_RECURSIVE_PROCEDURE.args(*node.begin(), m_context.source, *node.begin()));
            DiagnosticCollector::addDiagnostic(errors::semantics::PRESET_IN_RECURSIVE_PROCEDURE);
        }
        else if (m_blockDepth > 1)
        {
            m_context.log(
                errors::semantics::PRESET_IN_INNER_BLOCK.args(*node.begin(), m_context.source, *node.begin()));
            DiagnosticCollector::addDiagnostic(errors::semantics::PRESET_IN_INNER_BLOCK);
        }

        std::uint64_t totalElements = 1;
        for (const auto& [fst, snd] : node.getDimensions())
        {
            auto lower = evaluateConstantInteger(*fst);
            if (auto upper = evaluateConstantInteger(*snd); lower && upper && *lower <= *upper)
                totalElements *= static_cast<std::uint64_t>(*upper - *lower + 1);
        }

        // Count comma separated preset items
        std::uint64_t presetCount = 0;
        bool inPreset = false;
        for (const auto token : node.getPresetTokens())
        {
            if (token->getTokenType() == lexer::TokenType::Comma)
            {
                if (inPreset)
                    presetCount++;
                inPreset = false;
            }
            else
            {
                inPreset = true;
            }
        }
        if (inPreset)
            presetCount++;

        if (presetCount > totalElements)
        {
            m_context.log(
                errors::semantics::INCORRECT_PRESET_COUNT.args(*node.begin(), m_context.source, *node.begin()));
            DiagnosticCollector::addDiagnostic(errors::semantics::INCORRECT_PRESET_COUNT);
        }

        IntrusiveVariantValue type{PrimitiveType{PrimitiveType::Integer}};
        if (node.getType()->getTokenType() == lexer::TokenType::FloatingKeyword)
            type = IntrusiveVariantValue(PrimitiveType{PrimitiveType::Floating});
        else if (node.getType()->getTokenType() == lexer::TokenType::FixedKeyword)
            type = IntrusiveVariantValue(FixedType(24, 0));

        validatePresets(node.getPresetTokens(), *type, node.getFixedTotalBits(), node.getFixedFractionBits());
    }
}

void TypeCheckingPass::visit(const syntax::TableDeclaration& node)
{
    for (const auto* expr : {node.getWidth(), node.getLength()})
    {
        if (expr)
        {
            visit(*expr);
            if (auto it = m_context.expressionTypes.find(expr); it != m_context.expressionTypes.end())
            {
                if (const auto prim = resolveExprType(*it->second).tryAs<PrimitiveType>();
                    prim && prim->getKind() != PrimitiveType::Integer)
                {
                    m_context.log(errors::semantics::EXPECTED_INTEGER_TYPE.args(*expr->begin(), m_context.source,
                                                                                *expr->begin()));
                    DiagnosticCollector::addDiagnostic(errors::semantics::EXPECTED_INTEGER_TYPE);
                }
            }
        }
    }

    for (const auto& element : node.getElements())
    {
        visit(*element);
    }

    if (node.hasPreset())
    {
        if (m_currentProcedure && m_currentProcedure->isRecursive())
        {
            m_context.log(
                errors::semantics::PRESET_IN_RECURSIVE_PROCEDURE.args(*node.begin(), m_context.source, *node.begin()));
            DiagnosticCollector::addDiagnostic(errors::semantics::PRESET_IN_RECURSIVE_PROCEDURE);
        }
        else if (m_blockDepth > 1)
        {
            m_context.log(
                errors::semantics::PRESET_IN_INNER_BLOCK.args(*node.begin(), m_context.source, *node.begin()));
            DiagnosticCollector::addDiagnostic(errors::semantics::PRESET_IN_INNER_BLOCK);
        }

        std::uint64_t totalElements = 1;
        if (const auto w = evaluateConstantInteger(*node.getWidth()))
            totalElements *= static_cast<std::uint64_t>(*w);
        if (const auto l = evaluateConstantInteger(*node.getLength()))
            totalElements *= static_cast<std::uint64_t>(*l);

        // In a table element preset list, there is a preset value per field per row
        const std::uint64_t totalFields = node.getElements().size();

        std::uint64_t presetCount = 0;
        bool inPreset = false;
        for (const auto token : node.getPresetTokens())
        {
            if (token->getTokenType() == lexer::TokenType::Comma)
            {
                if (inPreset)
                    presetCount++;
                else
                    presetCount++; // Void elements still count towards total
                inPreset = false;
            }
            else
            {
                inPreset = true;
            }
        }
        if (inPreset)
            presetCount++;

        if (presetCount > totalElements * totalFields)
        {
            m_context.log(
                errors::semantics::INCORRECT_PRESET_COUNT.args(*node.begin(), m_context.source, *node.begin()));
            DiagnosticCollector::addDiagnostic(errors::semantics::INCORRECT_PRESET_COUNT);
        }

        // We should validate each item against the corresponding element type
        // This is complex, so we will skip detailed type checking for table presets for now
        // IntrusiveVariantValue type{PrimitiveType{PrimitiveType::Integer}};
        // validatePresets(node.getPresetTokens(), *type, nullptr, nullptr);
    }
}

void TypeCheckingPass::visit(const syntax::OverlayDeclaration& node)
{
    visit(node.getOverlayDeclaration());
}

// ReSharper disable once CppMemberFunctionMayBeConst
// ReSharper disable once CppMemberFunctionMayBeStatic
void TypeCheckingPass::visit(const syntax::ParameterDeclaration& /*node*/) {}

void TypeCheckingPass::visit(const syntax::ProcedureDeclaration& node)
{
    auto* oldProc = m_currentProcedure;
    m_currentProcedure = &node;

    const int oldBlockDepth = m_blockDepth;
    m_blockDepth = 0;

    for (const auto& param : node.getParameters())
    {
        visit(*param);
    }
    visit(node.getBody());

    m_blockDepth = oldBlockDepth;
    m_currentProcedure = oldProc;
}

void TypeCheckingPass::visit(const syntax::Expression& node)
{
    if (auto* idExpr = dynamic_cast<const syntax::IdentifierExpression*>(&node))
        visit(*idExpr);
    else if (auto* litExpr = dynamic_cast<const syntax::LiteralExpression*>(&node))
        visit(*litExpr);
    else if (auto* binExpr = dynamic_cast<const syntax::BinaryExpression*>(&node))
        visit(*binExpr);
    else if (auto* unExpr = dynamic_cast<const syntax::UnaryExpression*>(&node))
        visit(*unExpr);
    else if (auto* pwExpr = dynamic_cast<const syntax::PartWordExpression*>(&node))
        visit(*pwExpr);
    else if (auto* condExpr = dynamic_cast<const syntax::ConditionalExpression*>(&node))
        visit(*condExpr);
    else if (auto* callExpr = dynamic_cast<const syntax::FunctionCallExpression*>(&node))
        visit(*callExpr);
    else if (auto* arrExpr = dynamic_cast<const syntax::ArrayAccessExpression*>(&node))
        visit(*arrExpr);
    else if (auto* locExpr = dynamic_cast<const syntax::LocationExpression*>(&node))
        visit(*locExpr);
    else if (auto* tyExpr = dynamic_cast<const syntax::TypeChangeExpression*>(&node))
        visit(*tyExpr);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void TypeCheckingPass::visit(const syntax::IdentifierExpression& node)
{
    if (const auto it = m_context.resolvedIdentifiers.find(&node); it != m_context.resolvedIdentifiers.end())
    {
        const Usable* usable = it->second;
        if (auto* var = usable->tryAs<VariableDeclaration>())
        {
            m_context.expressionTypes.insert_or_assign(&node, var->getType());
        }
        else if (auto* arr = usable->tryAs<ArrayDeclaration>())
        {
            m_context.expressionTypes.insert_or_assign(&node, arr->getType());
        }
        else if (auto* proc = usable->tryAs<ProcedureDeclaration>())
        {
            if (m_currentProcedure && proc->getAstNode() == m_currentProcedure)
            {
                if (!m_currentProcedure->isRecursive())
                {
                    m_context.log(errors::semantics::RECURSIVE_CALL_ON_NON_RECURSIVE_PROCEDURE.args(
                        *node.getIdentifier(), m_context.source, *node.getIdentifier()));
                    DiagnosticCollector::addDiagnostic(errors::semantics::RECURSIVE_CALL_ON_NON_RECURSIVE_PROCEDURE);
                }
            }
            m_context.expressionTypes.insert_or_assign(&node, proc->getType());
        }
        else if (auto* table = usable->tryAs<TableDeclaration>())
        {
            m_context.expressionTypes.insert_or_assign(&node, table->getType());
        }
        else if (auto* tableElement = usable->tryAs<TableElementDeclaration>())
        {
            m_context.expressionTypes.insert_or_assign(&node, tableElement->getType());
        }
        else if (usable->is<LabelDeclaration>())
        {
            m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue(LabelType{}));
        }
        else if (usable->is<SwitchDeclaration>())
        {
            m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue(SwitchType{}));
        }
        else
        {
            m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue(ErrorType{}));
        }
    }
    else
    {
        m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue(ErrorType{}));
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
void TypeCheckingPass::visit(const syntax::LiteralExpression& node)
{
    if (node.getLiteral()->getType() == lexer::CToken::Floating)
        m_context.expressionTypes.insert_or_assign(&node,
                                                   IntrusiveVariantValue(PrimitiveType{PrimitiveType::Floating}));
    else if (node.begin()->getTokenType() == lexer::TokenType::OctalKeyword
             || node.begin()->getTokenType() == lexer::TokenType::LiteralKeyword)
        m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue(PrimitiveType{PrimitiveType::Bits}));
    else
        m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue(PrimitiveType{PrimitiveType::Integer}));
}

// ReSharper disable once CppMemberFunctionMayBeStatic
IntrusiveVariantValue<Type> TypeCheckingPass::getCommonType(const Type& t1, const Type& t2) const
{
    const Type& rt1 = resolveExprType(t1);
    const Type& rt2 = resolveExprType(t2);
    if (rt1.is<ErrorType>() || rt2.is<ErrorType>())
        return {ErrorType{}};

    if (isNumericType(rt1) && isNumericType(rt2))
    {
        const auto p1 = rt1.tryAs<PrimitiveType>();

        if (const auto p2 = rt2.tryAs<PrimitiveType>();
            (p1 && p1->getKind() == PrimitiveType::Floating) || (p2 && p2->getKind() == PrimitiveType::Floating))
        {
            return {PrimitiveType{PrimitiveType::Floating}};
        }
        auto f1 = rt1.tryAs<FixedType>();
        auto f2 = rt2.tryAs<FixedType>();
        if (f1 && f2)
        {
            return f1->getFractionalBits() >= f2->getFractionalBits() ? IntrusiveVariantValue{*f1} :
                                                                        IntrusiveVariantValue{*f2};
        }
        if (f1)
            return {*f1};
        if (f2)
            return {*f2};

        return {PrimitiveType{PrimitiveType::Integer}};
    }

    if (isBitsType(t1) && isBitsType(t2))
    {
        return {PrimitiveType{PrimitiveType::Bits}};
    }

    return {ErrorType{}};
}

void TypeCheckingPass::visit(const syntax::BinaryExpression& node)
{
    visit(node.getLeft());
    visit(node.getRight());

    const auto it1 = m_context.expressionTypes.find(&node.getLeft());

    if (const auto it2 = m_context.expressionTypes.find(&node.getRight());
        it1 != m_context.expressionTypes.end() && it2 != m_context.expressionTypes.end())
    {
        auto type1 = it1->second;
        auto type2 = it2->second;
        const auto opType = node.getOp()->getTokenType();

        const bool isBitwiseOp = opType == lexer::TokenType::DifferKeyword || opType == lexer::TokenType::UnionKeyword
                                 || opType == lexer::TokenType::MaskKeyword;

        bool operandsValid = true;
        if (isBitwiseOp)
        {
            auto isBitsOrInt = [](const Type& t)
            {
                const Type& rt = resolveExprType(t);
                if (isBitsType(rt) || isPointerType(rt) || rt.is<FixedType>())
                    return true;
                if (const auto p = rt.tryAs<PrimitiveType>())
                    return p->getKind() == PrimitiveType::Integer;
                return false;
            };
            if (!isBitsOrInt(*type1) || !isBitsOrInt(*type2))
                operandsValid = false;
        }
        else if (opType == lexer::TokenType::LessThan || opType == lexer::TokenType::LessThanOrEqual
                 || opType == lexer::TokenType::Equal || opType == lexer::TokenType::GreaterThanOrEqual
                 || opType == lexer::TokenType::GreaterThan || opType == lexer::TokenType::NotEqual)
        {
            if (isBitsType(*type1) && isBitsType(*type2))
            {
                // Fine
            }
            else if ((isBitsType(*type1) && isNumericType(*type2)) || (isNumericType(*type1) && isBitsType(*type2)))
            {
                // Coral 66 allows mixing bits and integers in some contexts, but let's just let it pass here
            }
            else if (isBitsType(*type1) || isBitsType(*type2))
            {
                operandsValid = false;
            }
        }
        else
        {
            if (isBitsType(*type1) || isBitsType(*type2))
                operandsValid = false;
        }

        auto common = getCommonType(*type1, *type2);
        if (isBitwiseOp)
        {
            common = IntrusiveVariantValue(PrimitiveType{PrimitiveType::Integer});
        }
        else if (opType == lexer::TokenType::LessThan || opType == lexer::TokenType::LessThanOrEqual
                 || opType == lexer::TokenType::Equal || opType == lexer::TokenType::GreaterThanOrEqual
                 || opType == lexer::TokenType::GreaterThan || opType == lexer::TokenType::NotEqual)
        {
            if (isBitsType(*type1) || isBitsType(*type2))
            {
                common = IntrusiveVariantValue(PrimitiveType{PrimitiveType::Bits});
            }
        }

        if ((common->is<ErrorType>() || !operandsValid) && !type1->is<ErrorType>() && !type2->is<ErrorType>())
        {
            m_context.log(errors::semantics::TYPE_MISMATCH.args(*node.getOp(), m_context.source, *node.getOp()));
            DiagnosticCollector::addDiagnostic(errors::semantics::TYPE_MISMATCH);
        }

        if (opType == lexer::TokenType::LessThan || opType == lexer::TokenType::LessThanOrEqual
            || opType == lexer::TokenType::Equal || opType == lexer::TokenType::GreaterThanOrEqual
            || opType == lexer::TokenType::GreaterThan || opType == lexer::TokenType::NotEqual)
        {
            m_context.expressionTypes.insert_or_assign(&node,
                                                       IntrusiveVariantValue(PrimitiveType{PrimitiveType::Integer}));
        }
        else if (isBitwiseOp)
        {
            m_context.expressionTypes.insert_or_assign(&node,
                                                       IntrusiveVariantValue(PrimitiveType{PrimitiveType::Integer}));
        }
        else
        {
            m_context.expressionTypes.insert_or_assign(&node, std::move(common));
        }
    }
    else
    {
        m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue(ErrorType{}));
    }
}

void TypeCheckingPass::visit(const syntax::UnaryExpression& node)
{
    visit(node.getOperand());
    if (const auto it = m_context.expressionTypes.find(&node.getOperand()); it != m_context.expressionTypes.end())
    {
        if (!it->second->is<ErrorType>() && !isNumericType(*it->second) && !isBitsType(*it->second))
        {
            m_context.log(errors::semantics::TYPE_MISMATCH.args(*node.getOp(), m_context.source, *node.getOp()));
            DiagnosticCollector::addDiagnostic(errors::semantics::TYPE_MISMATCH);
        }
        m_context.expressionTypes.insert_or_assign(&node, it->second);
    }
    else
    {
        m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue(ErrorType{}));
    }
}

void TypeCheckingPass::visit(const syntax::PartWordExpression& node)
{
    visit(node.getBase());
    visit(node.getStartBit());
    visit(node.getWidth());

    if (const auto baseIt = m_context.expressionTypes.find(&node.getBase());
        baseIt != m_context.expressionTypes.end() && !baseIt->second->is<ErrorType>())
    {
        if (!isNumericType(*baseIt->second) && !isBitsType(*baseIt->second) && !isPointerType(*baseIt->second))
        {
            m_context.log(errors::semantics::TYPE_MISMATCH.args(*node.getBase().begin(), m_context.source,
                                                                *node.getBase().begin()));
            DiagnosticCollector::addDiagnostic(errors::semantics::TYPE_MISMATCH);
        }
    }

    for (const auto* expr : {&node.getStartBit(), &node.getWidth()})
    {
        if (auto it = m_context.expressionTypes.find(expr);
            it != m_context.expressionTypes.end() && !it->second->is<ErrorType>())
        {
            if (const auto prim = resolveExprType(*it->second).tryAs<PrimitiveType>();
                !prim || prim->getKind() != PrimitiveType::Integer)
            {
                m_context.log(
                    errors::semantics::EXPECTED_INTEGER_TYPE.args(*expr->begin(), m_context.source, *expr->begin()));
                DiagnosticCollector::addDiagnostic(errors::semantics::EXPECTED_INTEGER_TYPE);
            }
        }
    }

    m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue(PrimitiveType{PrimitiveType::Integer}));
}

void TypeCheckingPass::visit(const syntax::ConditionalExpression& node)
{
    visit(node.getCondition());
    if (const auto itCond = m_context.expressionTypes.find(&node.getCondition());
        itCond != m_context.expressionTypes.end() && !itCond->second->is<ErrorType>())
    {
        if (const auto prim = resolveExprType(*itCond->second).tryAs<PrimitiveType>();
            !prim || prim->getKind() != PrimitiveType::Integer)
        {
            m_context.log(errors::semantics::EXPECTED_INTEGER_TYPE.args(*node.getCondition().begin(), m_context.source,
                                                                        *node.getCondition().begin()));
            DiagnosticCollector::addDiagnostic(errors::semantics::EXPECTED_INTEGER_TYPE);
        }
    }

    visit(node.getThenBranch());
    visit(node.getElseBranch());

    const auto itThen = m_context.expressionTypes.find(&node.getThenBranch());

    if (const auto itElse = m_context.expressionTypes.find(&node.getElseBranch());
        itThen != m_context.expressionTypes.end() && itElse != m_context.expressionTypes.end())
    {
        m_context.expressionTypes.insert_or_assign(&node, getCommonType(*itThen->second, *itElse->second));
    }
    else
    {
        m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue(ErrorType{}));
    }
}

void TypeCheckingPass::visit(const syntax::FunctionCallExpression& node)
{
    for (const auto& arg : node.getArguments())
    {
        visit(*arg);
    }

    if (const auto it = m_context.resolvedIdentifiers.find(&node); it != m_context.resolvedIdentifiers.end())
    {
        const Usable* usable = it->second;
        if (auto* proc = usable->tryAs<ProcedureDeclaration>())
        {
            if (m_currentProcedure && proc->getAstNode() == m_currentProcedure)
            {
                if (!m_currentProcedure->isRecursive())
                {
                    m_context.log(errors::semantics::RECURSIVE_CALL_ON_NON_RECURSIVE_PROCEDURE.args(
                        *node.getFunction(), m_context.source, *node.getFunction()));
                    DiagnosticCollector::addDiagnostic(errors::semantics::RECURSIVE_CALL_ON_NON_RECURSIVE_PROCEDURE);
                }
            }
            if (const auto procType = proc->getType().tryAs<ProcedureType>())
            {
                // Check argument count
                if (procType->getParameters().size() != node.getArguments().size())
                {
                    m_context.log(errors::semantics::PROCEDURE_ARGUMENT_COUNT_MISMATCH.args(
                        *node.getFunction(), m_context.source, *node.getFunction()));
                    DiagnosticCollector::addDiagnostic(errors::semantics::PROCEDURE_ARGUMENT_COUNT_MISMATCH);
                }
                else
                {
                    // Check argument types
                    for (size_t i = 0; i < procType->getParameters().size(); ++i)
                    {
                        auto& param = procType->getParameters()[i];
                        auto& arg = node.getArguments()[i];

                        const bool isPlaceOrProcedure = param.type->is<LabelType>() || param.type->is<SwitchType>()
                                                        || param.type->is<ProcedureType>();
                        if (!param.isValue && !isPlaceOrProcedure)
                        {
                            if (!isLValue(*arg, m_context))
                            {
                                m_context.log(errors::semantics::LOCATION_PARAMETER_REQUIRES_LVALUE.args(
                                    *arg->begin(), m_context.source, *arg->begin()));
                                DiagnosticCollector::addDiagnostic(
                                    errors::semantics::LOCATION_PARAMETER_REQUIRES_LVALUE);
                            }
                        }

                        if (auto itArg = m_context.expressionTypes.find(arg.get());
                            itArg != m_context.expressionTypes.end() && !itArg->second->is<ErrorType>())
                        {
                            bool isCompatible = false;
                            if (param.isValue)
                            {
                                if (isNumericType(*param.type) && isNumericType(*itArg->second))
                                    isCompatible = true;
                            }
                            if (!isCompatible && *param.type == *itArg->second)
                                isCompatible = true;

                            if (!isCompatible)
                            {
                                if (auto* paramArr = param.type->tryAs<ArrayType>())
                                {
                                    if (const auto* argArr = itArg->second->tryAs<ArrayType>())
                                    {
                                        if (paramArr->getElementType() == argArr->getElementType())
                                            isCompatible = true;
                                    }
                                }

                                if (auto* paramTab = param.type->tryAs<TableType>())
                                {
                                    if (const auto* argTab = itArg->second->tryAs<TableType>())
                                    {
                                        const auto* paramDef = std::get_if<TableDefinition>(&paramTab->getInfo().type);
                                        const auto* argDef = std::get_if<TableDefinition>(&argTab->getInfo().type);
                                        if (paramDef && argDef && paramDef->getWidth() == argDef->getWidth()
                                            && paramDef->getLength() == argDef->getLength())
                                        {
                                            isCompatible = true;
                                        }
                                    }
                                }

                                if (auto* paramPointer = param.type->tryAs<PrimitiveType>())
                                {
                                    if (const auto* argPointer = resolveExprType(*itArg->second).tryAs<PrimitiveType>())
                                    {
                                        if (paramPointer->getKind() == PrimitiveType::Pointer
                                            && (argPointer->getKind() == PrimitiveType::Integer
                                                || argPointer->getKind() == PrimitiveType::Pointer))
                                            isCompatible = true;
                                    }
                                    if (!param.isValue && paramPointer->getKind() == PrimitiveType::Integer)
                                    {
                                        if (itArg->second->is<TableType>() || itArg->second->is<ArrayType>())
                                            isCompatible = true;
                                    }
                                }
                            }

                            if (!isCompatible)
                            {
                                m_context.log(errors::semantics::PROCEDURE_ARGUMENT_TYPE_MISMATCH.args(
                                    *arg->begin(), m_context.source, *arg->begin()));
                                DiagnosticCollector::addDiagnostic(errors::semantics::PROCEDURE_ARGUMENT_TYPE_MISMATCH);
                            }
                        }
                    }
                }

                if (procType->getReturnType())
                {
                    if (const auto p = procType->getReturnType()->tryAs<PrimitiveType>())
                    {
                        m_context.expressionTypes.insert_or_assign(&node,
                                                                   IntrusiveVariantValue(PrimitiveType{p->getKind()}));
                    }
                    else if (const auto f = procType->getReturnType()->tryAs<FixedType>())
                    {
                        m_context.expressionTypes.insert_or_assign(
                            &node, IntrusiveVariantValue(FixedType{f->getTotalBits(), f->getFractionalBits()}));
                    }
                    else
                    {
                        m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue(UntypedType{}));
                    }
                }
                else
                {
                    m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue(UntypedType{}));
                }
            }
            else
            {
                m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue(UntypedType{}));
            }
        }
        else if (auto* var = usable->tryAs<VariableDeclaration>(); var && var->getType().is<ProcedureType>())
        {
            const auto* procType = var->getType().tryAs<ProcedureType>();
            if (procType && procType->getReturnType())
            {
                if (const auto p = procType->getReturnType()->tryAs<PrimitiveType>())
                {
                    m_context.expressionTypes.insert_or_assign(&node,
                                                               IntrusiveVariantValue(PrimitiveType{p->getKind()}));
                }
                else if (const auto f = procType->getReturnType()->tryAs<FixedType>())
                {
                    m_context.expressionTypes.insert_or_assign(
                        &node, IntrusiveVariantValue(FixedType{f->getTotalBits(), f->getFractionalBits()}));
                }
                else
                {
                    m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue(UntypedType{}));
                }
            }
            else
            {
                m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue(UntypedType{}));
            }
        }
        else
        {
            m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue(ErrorType{}));
        }
    }
    else
    {
        m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue(ErrorType{}));
    }
}

void TypeCheckingPass::visit(const syntax::ArrayAccessExpression& node)
{
    for (const auto& idx : node.getIndices())
    {
        visit(*idx);

        if (auto it = m_context.expressionTypes.find(idx.get()); it != m_context.expressionTypes.end())
        {
            if (const auto prim = resolveExprType(*it->second).tryAs<PrimitiveType>();
                !prim || prim->getKind() != PrimitiveType::Integer)
            {
                m_context.log(
                    errors::semantics::EXPECTED_INTEGER_TYPE.args(*idx->begin(), m_context.source, *idx->begin()));
                DiagnosticCollector::addDiagnostic(errors::semantics::EXPECTED_INTEGER_TYPE);
            }
        }
    }

    if (node.getArray() == nullptr)
    {
        m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue(PrimitiveType{PrimitiveType::Integer}));
        return;
    }

    if (const auto it = m_context.resolvedIdentifiers.find(&node); it != m_context.resolvedIdentifiers.end())
    {
        const Usable* usable = it->second;
        const ArrayType* arrType = nullptr;
        if (auto* arr = usable->tryAs<ArrayDeclaration>())
        {
            arrType = arr->getType().tryAs<ArrayType>();
        }
        else if (auto* var = usable->tryAs<VariableDeclaration>())
        {
            arrType = var->getType().tryAs<ArrayType>();
        }

        if (arrType)
        {
            if (!arrType->getDimensions().empty() && node.getIndices().size() != arrType->getDimensions().size())
            {
                m_context.log(errors::semantics::INCORRECT_NUMBER_OF_INDICES.args(*node.begin(), m_context.source,
                                                                                  *node.begin()));
                DiagnosticCollector::addDiagnostic(errors::semantics::INCORRECT_NUMBER_OF_INDICES);
            }

            m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue(arrType->getElementType()));
        }
        else if (auto* tableElement = usable->tryAs<TableElementDeclaration>())
        {
            m_context.expressionTypes.insert_or_assign(&node, tableElement->getType());
        }
        else if (usable->is<TableDeclaration>())
        {
            m_context.expressionTypes.insert_or_assign(&node,
                                                       IntrusiveVariantValue(PrimitiveType{PrimitiveType::Integer}));
        }
        else if (usable->is<SwitchDeclaration>()
                 || (usable->is<VariableDeclaration>() && usable->as<VariableDeclaration>().getType().is<SwitchType>()))
        {
            if (node.getIndices().size() != 1)
            {
                m_context.log(errors::semantics::INCORRECT_NUMBER_OF_INDICES.args(*node.begin(), m_context.source,
                                                                                  *node.begin()));
                DiagnosticCollector::addDiagnostic(errors::semantics::INCORRECT_NUMBER_OF_INDICES);
            }
            m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue(LabelType{}));
        }
        else
        {
            m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue(ErrorType{}));
        }
    }
    else
    {
        m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue(ErrorType{}));
    }
}

void TypeCheckingPass::visit(const syntax::LocationExpression& node)
{
    visit(node.getWordReference());
    if (!isLValue(node.getWordReference(), m_context))
    {
        m_context.log(errors::semantics::LOCATION_PARAMETER_REQUIRES_LVALUE.args(
            *node.getWordReference().begin(), m_context.source, *node.getWordReference().begin()));
        DiagnosticCollector::addDiagnostic(errors::semantics::LOCATION_PARAMETER_REQUIRES_LVALUE);
    }
    m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue(PrimitiveType{PrimitiveType::Pointer}));
}

void TypeCheckingPass::visit(const syntax::TypeChangeExpression& node)
{
    if (node.getTotalBits())
    {
        visit(*node.getTotalBits());
        if (const auto it = m_context.expressionTypes.find(node.getTotalBits());
            it != m_context.expressionTypes.end() && !it->second->is<ErrorType>())
        {
            if (const auto prim = resolveExprType(*it->second).tryAs<PrimitiveType>();
                !prim || prim->getKind() != PrimitiveType::Integer)
            {
                m_context.log(errors::semantics::EXPECTED_INTEGER_TYPE.args(
                    *node.getTotalBits()->begin(), m_context.source, *node.getTotalBits()->begin()));
                DiagnosticCollector::addDiagnostic(errors::semantics::EXPECTED_INTEGER_TYPE);
            }
        }
    }
    if (node.getFractionBits())
    {
        visit(*node.getFractionBits());
        if (const auto it = m_context.expressionTypes.find(node.getFractionBits());
            it != m_context.expressionTypes.end() && !it->second->is<ErrorType>())
        {
            if (const auto prim = resolveExprType(*it->second).tryAs<PrimitiveType>();
                !prim || prim->getKind() != PrimitiveType::Integer)
            {
                m_context.log(errors::semantics::EXPECTED_INTEGER_TYPE.args(
                    *node.getFractionBits()->begin(), m_context.source, *node.getFractionBits()->begin()));
                DiagnosticCollector::addDiagnostic(errors::semantics::EXPECTED_INTEGER_TYPE);
            }
        }
    }

    visit(node.getExpression());
    if (const auto exprIt = m_context.expressionTypes.find(&node.getExpression());
        exprIt != m_context.expressionTypes.end() && !exprIt->second->is<ErrorType>())
    {
        if (!isNumericType(*exprIt->second) && !isBitsType(*exprIt->second))
        {
            m_context.log(errors::semantics::TYPE_MISMATCH.args(*node.getExpression().begin(), m_context.source,
                                                                *node.getExpression().begin()));
            DiagnosticCollector::addDiagnostic(errors::semantics::TYPE_MISMATCH);
        }
    }

    if (const auto kind = node.getType()->getTokenType(); kind == lexer::TokenType::IntegerKeyword)
    {
        m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue(PrimitiveType{PrimitiveType::Integer}));
    }
    else if (kind == lexer::TokenType::FloatingKeyword)
    {
        m_context.expressionTypes.insert_or_assign(&node,
                                                   IntrusiveVariantValue(PrimitiveType{PrimitiveType::Floating}));
    }
    else if (kind == lexer::TokenType::FixedKeyword)
    {
        std::uint8_t total = 24;
        std::uint8_t frac = 0;
        if (node.getTotalBits())
        {
            if (const auto tVal = evaluateConstantInteger(*node.getTotalBits()); tVal && *tVal > 0)
                total = static_cast<std::uint8_t>(*tVal);
        }
        if (node.getFractionBits())
        {
            if (const auto fVal = evaluateConstantInteger(*node.getFractionBits()); fVal && *fVal >= 0)
                frac = static_cast<std::uint8_t>(*fVal);
        }
        m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue(FixedType(total, frac)));
    }
    else
    {
        m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue(ErrorType{}));
    }
}

void TypeCheckingPass::validatePresets(const std::vector<lexer::CTokenIterator>& tokens, const Type& baseType,
                                       const syntax::Expression* fixedTotal,
                                       const syntax::Expression* fixedFraction) const
{
    const bool isFixed = baseType.is<FixedType>();
    bool isInteger = false;
    bool isFloating = false;

    if (const auto* pType = baseType.tryAs<PrimitiveType>())
    {
        isInteger = pType->getKind() == PrimitiveType::Integer;
        isFloating = pType->getKind() == PrimitiveType::Floating;
    }

    if (!isFixed && !isInteger && !isFloating)
        return;

    std::optional<std::int64_t> totalBits;
    std::optional<std::int64_t> fractionBits;
    if (fixedTotal)
        totalBits = evaluateConstantInteger(*fixedTotal);
    if (fixedFraction)
        fractionBits = evaluateConstantInteger(*fixedFraction);

    bool isNegative = false;
    for (const auto& token : tokens)
    {
        if (token->getTokenType() == lexer::TokenType::Minus)
        {
            isNegative = !isNegative;
            continue;
        }
        if (token->getTokenType() == lexer::TokenType::Plus || token->getTokenType() == lexer::TokenType::OctalKeyword
            || token->getTokenType() == lexer::TokenType::OpenParentheses)
        {
            continue;
        }
        if (token->getTokenType() == lexer::TokenType::Comma
            || token->getTokenType() == lexer::TokenType::CloseParentheses)
        {
            isNegative = false;
            continue;
        }

        if (token->getTokenType() == lexer::TokenType::StringLiteral)
        {
            if (isInteger || isFixed || isFloating)
            {
                m_context.log(errors::semantics::PRESET_TYPE_MISMATCH.args(*token, m_context.source, *token));
                DiagnosticCollector::addDiagnostic(errors::semantics::PRESET_TYPE_MISMATCH);
            }
            isNegative = false;
            continue;
        }

        if (token->getTokenType() == lexer::TokenType::Literal)
        {
            const auto tokenType = token->getType();
            if (isInteger)
            {
                if (tokenType == lexer::CToken::Floating)
                {
                    m_context.log(errors::semantics::PRESET_TYPE_MISMATCH.args(*token, m_context.source, *token));
                    DiagnosticCollector::addDiagnostic(errors::semantics::PRESET_TYPE_MISMATCH);
                }
            }
            else if (isFixed)
            {
                if (totalBits && fractionBits)
                {
                    double val = 0.0;
                    if (auto* intVal = std::get_if<llvm::APSInt>(&token->getValue()))
                        val = static_cast<double>(intVal->getExtValue());
                    else if (auto* floatVal = std::get_if<llvm::APFloat>(&token->getValue()))
                        val = floatVal->convertToDouble();

                    if (isNegative)
                        val = -val;

                    const double maxVal = std::pow(2.0, static_cast<double>(*totalBits - *fractionBits - 1))
                                          - std::pow(2.0, static_cast<double>(-*fractionBits));

                    if (const double minVal = -std::pow(2.0, static_cast<double>(*totalBits - *fractionBits - 1));
                        val > maxVal || val < minVal)
                    {
                        m_context.log(
                            errors::semantics::PRESET_FIXED_POINT_OVERFLOW.args(*token, m_context.source, *token));
                        DiagnosticCollector::addDiagnostic(errors::semantics::PRESET_FIXED_POINT_OVERFLOW);
                    }
                }
            }
            isNegative = false;
        }
    }
}

} // namespace coral66::semantics
