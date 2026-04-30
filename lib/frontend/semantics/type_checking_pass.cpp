#include "type_checking_pass.hpp"

#include "lib/frontend/common/messages/errors.hpp"
#include "semantic_support.hpp"

namespace coral66::semantics
{

static bool isNumericType(const Type& t)
{
    if (auto p = t.tryAs<PrimitiveType>())
        return p->getKind() == PrimitiveType::Integer || p->getKind() == PrimitiveType::Floating
               || p->getKind() == PrimitiveType::Bits;
    return t.is<FixedType>();
}

TypeCheckingPass::TypeCheckingPass(SemanticContext& context) : m_context(context) {}

void TypeCheckingPass::run(const syntax::Program& program)
{
    visit(program);
}

void TypeCheckingPass::visit(const syntax::Program& node)
{
    if (node.hasBlock())
    {
        visit(node.getBlock());
    }
}

void TypeCheckingPass::visit(const syntax::Block& node)
{
    for (const auto& declaration : node.getDeclarations())
    {
        visit(*declaration);
    }
    for (const auto& stmt : node.getStatements())
    {
        visit(*stmt);
    }
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
    auto it = m_context.expressionTypes.find(&node.getCondition());
    if (it != m_context.expressionTypes.end() && !it->second->is<ErrorType>())
    {
        auto prim = it->second->tryAs<PrimitiveType>();
        if (!prim || prim->getKind() != PrimitiveType::Integer)
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
    
    auto varIt = m_context.expressionTypes.find(&node.getVariable());
    if (varIt != m_context.expressionTypes.end() && !varIt->second->is<ErrorType>())
    {
        if (!isNumericType(*varIt->second))
        {
            m_context.log(errors::semantics::EXPECTED_NUMERIC_TYPE.args(
                *node.getVariable().begin(), m_context.source, *node.getVariable().begin()));
            DiagnosticCollector::addDiagnostic(errors::semantics::EXPECTED_NUMERIC_TYPE);
        }
    }

    for (const auto& element : node.getElements())
    {
        visit(*element.expression);
        if (element.step)
            visit(*element.step);
        if (element.until)
            visit(*element.until);
        if (element.whileCondition)
        {
            visit(*element.whileCondition);
            auto it = m_context.expressionTypes.find(element.whileCondition.get());
            if (it != m_context.expressionTypes.end() && !it->second->is<ErrorType>())
            {
                auto prim = it->second->tryAs<PrimitiveType>();
                if (!prim || prim->getKind() != PrimitiveType::Integer)
                {
                    m_context.log(errors::semantics::EXPECTED_INTEGER_TYPE.args(
                        *element.whileCondition->begin(), m_context.source, *element.whileCondition->begin()));
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

    // Check type compatibility
    auto itL = m_context.expressionTypes.find(&node.getVariable());
    auto itR = m_context.expressionTypes.find(&node.getExpression());

    if (itL != m_context.expressionTypes.end() && itR != m_context.expressionTypes.end())
    {
        auto lType = itL->second;
        auto rType = itR->second;

        if (!lType->is<ErrorType>() && !rType->is<ErrorType>())
        {
            if (!isNumericType(*lType) || !isNumericType(*rType))
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
        auto it = m_context.expressionTypes.find(node.getIndex());
        if (it != m_context.expressionTypes.end())
        {
            auto prim = it->second->tryAs<PrimitiveType>();
            if (prim && prim->getKind() != PrimitiveType::Integer)
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
            auto it = m_context.expressionTypes.find(&node.getExpression());
            if (it != m_context.expressionTypes.end() && !it->second->is<ErrorType>())
            {
                if (!isNumericType(*it->second))
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
void TypeCheckingPass::visit(const syntax::VariableDeclaration& /*node*/) {}

void TypeCheckingPass::visit(const syntax::ArrayDeclaration& node)
{
    for (const auto& dim : node.getDimensions())
    {
        visit(*dim.first);
        visit(*dim.second);

        for (const auto* expr : {dim.first.get(), dim.second.get()})
        {
            auto it = m_context.expressionTypes.find(expr);
            if (it != m_context.expressionTypes.end())
            {
                auto prim = it->second->tryAs<PrimitiveType>();
                if (prim && prim->getKind() != PrimitiveType::Integer)
                {
                    m_context.log(errors::semantics::EXPECTED_INTEGER_TYPE.args(*expr->begin(), m_context.source,
                                                                                *expr->begin()));
                    DiagnosticCollector::addDiagnostic(errors::semantics::EXPECTED_INTEGER_TYPE);
                }
            }
        }
    }
}

void TypeCheckingPass::visit(const syntax::TableDeclaration& node)
{
    for (const auto* expr : {node.getWidth(), node.getLength()})
    {
        if (expr)
        {
            visit(*expr);
            auto it = m_context.expressionTypes.find(expr);
            if (it != m_context.expressionTypes.end())
            {
                auto prim = it->second->tryAs<PrimitiveType>();
                if (prim && prim->getKind() != PrimitiveType::Integer)
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
}

void TypeCheckingPass::visit(const syntax::OverlayDeclaration& node)
{
    visit(node.getOverlayDeclaration());
}

// ReSharper disable once CppMemberFunctionMayBeConst
void TypeCheckingPass::visit(const syntax::ParameterDeclaration& /*node*/) {}

void TypeCheckingPass::visit(const syntax::ProcedureDeclaration& node)
{
    auto* oldProc = m_currentProcedure;
    m_currentProcedure = &node;

    for (const auto& param : node.getParameters())
    {
        visit(*param);
    }
    visit(node.getBody());

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
    auto it = m_context.resolvedIdentifiers.find(&node);
    if (it != m_context.resolvedIdentifiers.end())
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
            m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue<Type>(LabelType{}));
        }
        else if (usable->is<SwitchDeclaration>())
        {
            m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue<Type>(SwitchType{}));
        }
        else
        {
            m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue<Type>(ErrorType{}));
        }
    }
    else
    {
        m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue<Type>(ErrorType{}));
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
void TypeCheckingPass::visit(const syntax::LiteralExpression& node)
{
    if (node.getLiteral()->getType() == lexer::CToken::Floating)
        m_context.expressionTypes.insert_or_assign(&node,
                                                   IntrusiveVariantValue<Type>(PrimitiveType{PrimitiveType::Floating}));
    else
        m_context.expressionTypes.insert_or_assign(&node,
                                                   IntrusiveVariantValue<Type>(PrimitiveType{PrimitiveType::Integer}));
}

IntrusiveVariantValue<Type> TypeCheckingPass::getCommonType(const Type& t1, const Type& t2) const
{
    if (t1.is<ErrorType>() || t2.is<ErrorType>())
        return {(ErrorType{})};

    if (isNumericType(t1) && isNumericType(t2))
    {
        auto p1 = t1.tryAs<PrimitiveType>();
        auto p2 = t2.tryAs<PrimitiveType>();

        if ((p1 && p1->getKind() == PrimitiveType::Floating) || (p2 && p2->getKind() == PrimitiveType::Floating))
        {
            return {(PrimitiveType{PrimitiveType::Floating})};
        }
        auto f1 = t1.tryAs<FixedType>();
        if (f1)
            return {*f1};
        auto f2 = t2.tryAs<FixedType>();
        if (f2)
            return {*f2};

        return {(PrimitiveType{PrimitiveType::Integer})};
    }
    return {(ErrorType{})};
}

void TypeCheckingPass::visit(const syntax::BinaryExpression& node)
{
    visit(node.getLeft());
    visit(node.getRight());

    auto it1 = m_context.expressionTypes.find(&node.getLeft());
    auto it2 = m_context.expressionTypes.find(&node.getRight());

    if (it1 != m_context.expressionTypes.end() && it2 != m_context.expressionTypes.end())
    {
        auto common = getCommonType(*it1->second, *it2->second);
        if (common->is<ErrorType>() && !it1->second->is<ErrorType>() && !it2->second->is<ErrorType>())
        {
            m_context.log(errors::semantics::TYPE_MISMATCH.args(*node.getOp(), m_context.source, *node.getOp()));
            DiagnosticCollector::addDiagnostic(errors::semantics::TYPE_MISMATCH);
        }

        auto opType = node.getOp()->getTokenType();
        if (opType == lexer::TokenType::LessThan || opType == lexer::TokenType::LessThanOrEqual
            || opType == lexer::TokenType::Equal || opType == lexer::TokenType::GreaterThanOrEqual
            || opType == lexer::TokenType::GreaterThan || opType == lexer::TokenType::NotEqual)
        {
            m_context.expressionTypes.insert_or_assign(
                &node, IntrusiveVariantValue<Type>(PrimitiveType{PrimitiveType::Integer}));
        }
        else
        {
            m_context.expressionTypes.insert_or_assign(&node, std::move(common));
        }
    }
    else
    {
        m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue<Type>(ErrorType{}));
    }
}

void TypeCheckingPass::visit(const syntax::UnaryExpression& node)
{
    visit(node.getOperand());
    auto it = m_context.expressionTypes.find(&node.getOperand());
    if (it != m_context.expressionTypes.end())
    {
        m_context.expressionTypes.insert_or_assign(&node, it->second);
    }
    else
    {
        m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue<Type>(ErrorType{}));
    }
}

void TypeCheckingPass::visit(const syntax::PartWordExpression& node)
{
    visit(node.getBase());
    visit(node.getStartBit());
    visit(node.getWidth());

    m_context.expressionTypes.insert_or_assign(&node,
                                               IntrusiveVariantValue<Type>(PrimitiveType{PrimitiveType::Integer}));
}

void TypeCheckingPass::visit(const syntax::ConditionalExpression& node)
{
    visit(node.getCondition());
    visit(node.getThenBranch());
    visit(node.getElseBranch());

    auto itThen = m_context.expressionTypes.find(&node.getThenBranch());
    auto itElse = m_context.expressionTypes.find(&node.getElseBranch());

    if (itThen != m_context.expressionTypes.end() && itElse != m_context.expressionTypes.end())
    {
        m_context.expressionTypes.insert_or_assign(&node, getCommonType(*itThen->second, *itElse->second));
    }
    else
    {
        m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue<Type>(ErrorType{}));
    }
}

void TypeCheckingPass::visit(const syntax::FunctionCallExpression& node)
{
    for (const auto& arg : node.getArguments())
    {
        visit(*arg);
    }

    auto it = m_context.resolvedIdentifiers.find(&node);
    if (it != m_context.resolvedIdentifiers.end())
    {
        const Usable* usable = it->second;
        if (auto* proc = usable->tryAs<ProcedureDeclaration>())
        {
            if (auto procType = proc->getType().tryAs<ProcedureType>())
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
                        auto itArg = m_context.expressionTypes.find(arg.get());
                        if (itArg != m_context.expressionTypes.end() && !itArg->second->is<ErrorType>())
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
                                    if (auto* argArr = itArg->second->tryAs<ArrayType>())
                                    {
                                        if (paramArr->getElementType() == argArr->getElementType())
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
                    if (auto p = procType->getReturnType()->tryAs<PrimitiveType>())
                    {
                        m_context.expressionTypes.insert_or_assign(
                            &node, IntrusiveVariantValue<Type>(PrimitiveType{p->getKind()}));
                    }
                    else
                    {
                        m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue<Type>(UntypedType{}));
                    }
                }
                else
                {
                    m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue<Type>(UntypedType{}));
                }
            }
            else
            {
                m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue<Type>(UntypedType{}));
            }
        }
        else
        {
            m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue<Type>(ErrorType{}));
        }
    }
    else
    {
        m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue<Type>(ErrorType{}));
    }
}

void TypeCheckingPass::visit(const syntax::ArrayAccessExpression& node)
{
    for (const auto& idx : node.getIndices())
    {
        visit(*idx);

        auto it = m_context.expressionTypes.find(idx.get());
        if (it != m_context.expressionTypes.end())
        {
            auto prim = it->second->tryAs<PrimitiveType>();
            if (prim && prim->getKind() != PrimitiveType::Integer)
            {
                m_context.log(
                    errors::semantics::EXPECTED_INTEGER_TYPE.args(*idx->begin(), m_context.source, *idx->begin()));
                DiagnosticCollector::addDiagnostic(errors::semantics::EXPECTED_INTEGER_TYPE);
            }
        }
    }

    if (node.getArray() == nullptr)
    {
        m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue<Type>(PrimitiveType{PrimitiveType::Integer}));
        return;
    }

    auto it = m_context.resolvedIdentifiers.find(&node);
    if (it != m_context.resolvedIdentifiers.end())
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
                m_context.log(
                    errors::semantics::INCORRECT_NUMBER_OF_INDICES.args(*node.begin(), m_context.source, *node.begin()));
                DiagnosticCollector::addDiagnostic(errors::semantics::INCORRECT_NUMBER_OF_INDICES);
            }

            if (auto p = arrType->getElementType().tryAs<PrimitiveType>())
            {
                m_context.expressionTypes.insert_or_assign(
                    &node, IntrusiveVariantValue<Type>(PrimitiveType{p->getKind()}));
            }
            else
            {
                m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue<Type>(UntypedType{}));
            }
        }
        else if (auto* tableElement = usable->tryAs<TableElementDeclaration>())
        {
            m_context.expressionTypes.insert_or_assign(&node, tableElement->getType());
        }
        else
        {
            m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue<Type>(ErrorType{}));
        }
    }
    else
    {
        m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue<Type>(ErrorType{}));
    }
}

void TypeCheckingPass::visit(const syntax::LocationExpression& node)
{
    visit(node.getWordReference());
    m_context.expressionTypes.insert_or_assign(&node,
                                               IntrusiveVariantValue<Type>(PrimitiveType{PrimitiveType::Integer}));
}

void TypeCheckingPass::visit(const syntax::TypeChangeExpression& node)
{
    if (node.getTotalBits())
        visit(*node.getTotalBits());
    if (node.getFractionBits())
        visit(*node.getFractionBits());
    visit(node.getExpression());

    auto kind = node.getType()->getTokenType();
    if (kind == lexer::TokenType::IntegerKeyword)
    {
        m_context.expressionTypes.insert_or_assign(&node,
                                                   IntrusiveVariantValue<Type>(PrimitiveType{PrimitiveType::Integer}));
    }
    else if (kind == lexer::TokenType::FloatingKeyword)
    {
        m_context.expressionTypes.insert_or_assign(&node,
                                                   IntrusiveVariantValue<Type>(PrimitiveType{PrimitiveType::Floating}));
    }
    else if (kind == lexer::TokenType::FixedKeyword)
    {
        m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue<Type>(FixedType(24, 0)));
    }
    else
    {
        m_context.expressionTypes.insert_or_assign(&node, IntrusiveVariantValue<Type>(ErrorType{}));
    }
}

} // namespace coral66::semantics
