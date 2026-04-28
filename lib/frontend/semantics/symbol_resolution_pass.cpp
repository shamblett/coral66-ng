#include "lib/frontend/common/diagnostic_collection.hpp"
/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include <functional>
#include <string>

#include "lib/frontend/common/messages/errors.hpp"

#include "semantic_support.hpp"
#include "symbol_resolution_pass.hpp"

namespace coral66::semantics
{

static UntypedType untypedType{};

template <class T, class... Args>
IntrVarPtr<T> makeIntrusiveVariant(Args&&... args)
{
    return IntrVarPtr<T>(new T(std::forward<Args>(args)...));
}

SymbolResolutionPass::SymbolResolutionPass(SemanticContext& context) : m_context(context) {}

void SymbolResolutionPass::run(const syntax::Program& program)
{
    visit(program);
}

void SymbolResolutionPass::visit(const syntax::Program& node)
{
    m_context.symbolTable.enterScope();
    if (node.hasBlock())
    {
        visit(node.getBlock());
    }
    m_context.symbolTable.exitScope();
}

void SymbolResolutionPass::visit(const syntax::Block& node)
{
    m_context.symbolTable.enterScope();

    // Pre-scan all statements for labels to ensure correct scope and forward-referencing
    std::function<void(const syntax::Statement&)> preScanLabels = [&](const syntax::Statement& stmt)
    {
        for (const auto& label : stmt.getLabels())
        {
            auto declaration = makeIntrusiveVariant<LabelDeclaration>(label);
            const auto& name = std::get<std::string>(label->getValue());
            if (m_context.symbolTable.lookupInCurrentScope(name))
            {
                m_context.log(errors::semantics::REDEFINITION_OF_SYMBOL_N.args(*label, m_context.source, *label));
                DiagnosticCollector::addDiagnostic(errors::semantics::REDEFINITION_OF_SYMBOL_N);
            }
            else
            {
                m_context.symbolTable.insert(name, declaration.get());
            }
            m_context.ownedUsables.push_back(std::move(declaration));
        }

        if (auto* compStmt = dynamic_cast<const syntax::CompoundStatement*>(&stmt))
        {
            for (const auto& innerStmt : compStmt->getStatements())
                preScanLabels(*innerStmt);
        }
        else if (auto* ifStmt = dynamic_cast<const syntax::IfStatement*>(&stmt))
        {
            preScanLabels(ifStmt->getThenBranch());
            if (ifStmt->getElseBranch())
                preScanLabels(*ifStmt->getElseBranch());
        }
        else if (auto* forStmt = dynamic_cast<const syntax::ForStatement*>(&stmt))
        {
            preScanLabels(forStmt->getBody());
        }
        // BlockStatements create a new scope, so we do NOT recurse into them during pre-scan.
    };

    for (const auto& stmt : node.getStatements())
    {
        preScanLabels(*stmt);
    }

    for (const auto& declaration : node.getDeclarations())
    {
        visit(*declaration);
    }

    for (const auto& stmt : node.getStatements())
    {
        visit(*stmt);
    }

    m_context.symbolTable.exitScope();
}

void SymbolResolutionPass::visit(const syntax::Statement& node)
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

void SymbolResolutionPass::visit(const syntax::BlockStatement& node)
{
    if (node.hasBlock())
        visit(node.getBlock());
}

void SymbolResolutionPass::visit(const syntax::CompoundStatement& node)
{
    for (const auto& stmt : node.getStatements())
        visit(*stmt);
}

void SymbolResolutionPass::visit(const syntax::IfStatement& node)
{
    visit(node.getCondition());
    visit(node.getThenBranch());
    if (node.getElseBranch())
        visit(*node.getElseBranch());
}

void SymbolResolutionPass::visit(const syntax::ForStatement& node)
{
    visit(node.getVariable());
    for (const auto& element : node.getElements())
    {
        visit(*element.expression);
        if (element.step)
            visit(*element.step);
        if (element.until)
            visit(*element.until);
        if (element.whileCondition)
            visit(*element.whileCondition);
    }
    visit(node.getBody());
}

void SymbolResolutionPass::visit(const syntax::AssignmentStatement& node)
{
    visit(node.getVariable());
    visit(node.getExpression());
}

void SymbolResolutionPass::visit(const syntax::GotoStatement& node)
{
    // Goto target label resolution is typically verified in ControlFlowPass
    if (node.getIndex())
        visit(*node.getIndex());
}

void SymbolResolutionPass::visit(const syntax::AnswerStatement& node)
{
    visit(node.getExpression());
}

void SymbolResolutionPass::visit(const syntax::ProcedureCallStatement& node)
{
    visit(node.getExpression());
}

// ReSharper disable once CppMemberFunctionMayBeStatic
IntrusiveVariantValue<Type> SymbolResolutionPass::createTypeFromToken( // NOLINT(*-convert-member-functions-to-static)
    std::optional<lexer::CTokenIterator> typeToken)                    // NOLINT(*-convert-member-functions-to-static)
{
    if (!typeToken)
        return {(UntypedType{})};

    auto type = typeToken.value()->getTokenType();
    if (type == lexer::TokenType::FloatingKeyword)
        return {(PrimitiveType{PrimitiveType::Floating})};
    if (type == lexer::TokenType::FixedKeyword)
        return {(FixedType{16, 8})};

    return {(PrimitiveType{PrimitiveType::Integer})};
}

void SymbolResolutionPass::visit(const syntax::Declaration& node)
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
    else if (auto* switchDeclaration = dynamic_cast<const syntax::SwitchDeclaration*>(&node))
        visit(*switchDeclaration);
    else if (auto* tableElementDeclaration = dynamic_cast<const syntax::TableElementDeclaration*>(&node))
        visit(*tableElementDeclaration);
}

void SymbolResolutionPass::visit(const syntax::VariableDeclaration& node)
{
    auto type = createTypeFromToken(node.getType());
    for (auto id : node.getIdentifiers())
    {
        auto typeCopy = IntrusiveVariantValue(PrimitiveType{PrimitiveType::Integer});
        if (type->is<PrimitiveType>())
            typeCopy = IntrusiveVariantValue(PrimitiveType{type->as<PrimitiveType>().getKind()});
        else if (type->is<FixedType>())
            typeCopy = IntrusiveVariantValue(
                FixedType{type->as<FixedType>().getTotalBits(), type->as<FixedType>().getFractionalBits()});

        auto declaration = makeIntrusiveVariant<VariableDeclaration>(id, std::move(typeCopy));
        const auto& name = std::get<std::string>(id->getValue());

        if (m_context.symbolTable.lookupInCurrentScope(name))
        {
            m_context.log(errors::semantics::REDEFINITION_OF_SYMBOL_N.args(*id, m_context.source, *id));
            DiagnosticCollector::addDiagnostic(errors::semantics::REDEFINITION_OF_SYMBOL_N);
        }
        else
        {
            m_context.symbolTable.insert(name, declaration.get());
        }
        m_context.ownedUsables.push_back(std::move(declaration));
    }
}

void SymbolResolutionPass::visit(const syntax::ArrayDeclaration& node)
{
    for (auto id : node.getIdentifiers())
    {
        std::vector<std::uint64_t> dims(node.getDimensions().size(), 10); // Dummy dimensions
        auto typeCopy = IntrusiveVariantValue(ArrayType{&untypedType, dims});

        auto declaration = makeIntrusiveVariant<ArrayDeclaration>(id, std::move(typeCopy));
        const auto& name = std::get<std::string>(id->getValue());

        if (m_context.symbolTable.lookupInCurrentScope(name))
        {
            m_context.log(errors::semantics::REDEFINITION_OF_SYMBOL_N.args(*id, m_context.source, *id));
            DiagnosticCollector::addDiagnostic(errors::semantics::REDEFINITION_OF_SYMBOL_N);
        }
        else
        {
            m_context.symbolTable.insert(name, declaration.get());
        }
        m_context.ownedUsables.push_back(std::move(declaration));
    }

    // Visit dimension expressions
    for (const auto& dim : node.getDimensions())
    {
        if (dim.first)
            visit(*dim.first);
        if (dim.second)
            visit(*dim.second);
    }
}

void SymbolResolutionPass::visit(const syntax::TableDeclaration& node)
{
    if (node.getWidth())
        visit(*node.getWidth());
    if (node.getLength())
        visit(*node.getLength());

    auto id = node.getIdentifier();
    auto typeCopy = IntrusiveVariantValue(
        TableType{*new TableInfo{std::get<std::string>(id->getValue()), 0, TableDefinition{{}, 1, 1}}});
    auto declaration = makeIntrusiveVariant<TableDeclaration>(id, std::move(typeCopy));
    const auto& name = std::get<std::string>(id->getValue());

    if (m_context.symbolTable.lookupInCurrentScope(name))
    {
        m_context.log(errors::semantics::REDEFINITION_OF_SYMBOL_N.args(*id, m_context.source, *id));
        DiagnosticCollector::addDiagnostic(errors::semantics::REDEFINITION_OF_SYMBOL_N);
    }
    else
    {
        m_context.symbolTable.insert(name, declaration.get());
    }
    m_context.ownedUsables.push_back(std::move(declaration));

    for (const auto& element : node.getElements())
    {
        visit(*element);
    }
}

void SymbolResolutionPass::visit(const syntax::TableElementDeclaration& node)
{
    auto type = createTypeFromToken(node.getType());
    auto typeCopy = IntrusiveVariantValue<Type>(PrimitiveType{PrimitiveType::Integer}); // Default for now
    if (type->is<PrimitiveType>())
        typeCopy = IntrusiveVariantValue(PrimitiveType{type->as<PrimitiveType>().getKind()});
    else if (type->is<FixedType>())
        typeCopy = IntrusiveVariantValue(
            FixedType{type->as<FixedType>().getTotalBits(), type->as<FixedType>().getFractionalBits()});

    std::optional<std::uint32_t> wordPos;
    if (node.getWordPosition())
    {
        if (auto* i = std::get_if<llvm::APSInt>(&node.getWordPosition().value()->getValue()))
            wordPos = static_cast<std::uint32_t>(i->getZExtValue());
    }

    std::optional<std::uint32_t> bitPos;
    if (node.getBitPosition())
    {
        if (auto* i = std::get_if<llvm::APSInt>(&node.getBitPosition().value()->getValue()))
            bitPos = static_cast<std::uint32_t>(i->getZExtValue());
    }

    auto id = node.getIdentifier();
    auto declaration = makeIntrusiveVariant<TableElementDeclaration>(id, std::move(typeCopy), node.isUnsigned(), wordPos, bitPos);
    const auto& name = std::get<std::string>(id->getValue());

    if (m_context.symbolTable.lookupInCurrentScope(name))
    {
        m_context.log(errors::semantics::REDEFINITION_OF_SYMBOL_N.args(*id, m_context.source, *id));
        DiagnosticCollector::addDiagnostic(errors::semantics::REDEFINITION_OF_SYMBOL_N);
    }
    else
    {
        m_context.symbolTable.insert(name, declaration.get());
    }
    m_context.ownedUsables.push_back(std::move(declaration));
}

void SymbolResolutionPass::visit(const syntax::SwitchDeclaration& node)
{
    auto id = node.getIdentifier();

    for (const auto& labelToken : node.getLabels())
    {
        const auto& labelName = std::get<std::string>(labelToken->getValue());
        const Usable* usable = m_context.symbolTable.lookup(labelName);
        if (!usable)
        {
            m_context.log(errors::semantics::UNDECLARED_IDENTIFIER_N.args(*labelToken, m_context.source, *labelToken));
            DiagnosticCollector::addDiagnostic(errors::semantics::UNDECLARED_IDENTIFIER_N);
        }
        else if (!usable->is<LabelDeclaration>())
        {
            m_context.log(errors::semantics::LABEL_NOT_FOUND_N.args(*labelToken, m_context.source, *labelToken));
            DiagnosticCollector::addDiagnostic(errors::semantics::LABEL_NOT_FOUND_N);
        }
    }

    auto declaration = makeIntrusiveVariant<SwitchDeclaration>(id, std::vector<const LabelStatement*>{});
    const auto& name = std::get<std::string>(id->getValue());

    if (m_context.symbolTable.lookupInCurrentScope(name))
    {
        m_context.log(errors::semantics::REDEFINITION_OF_SYMBOL_N.args(*id, m_context.source, *id));
        DiagnosticCollector::addDiagnostic(errors::semantics::REDEFINITION_OF_SYMBOL_N);
    }
    else
    {
        m_context.symbolTable.insert(name, declaration.get());
    }
    m_context.ownedUsables.push_back(std::move(declaration));
}

void SymbolResolutionPass::visit(const syntax::OverlayDeclaration& node)
{
    auto baseToken = node.getBase();
    const auto& baseName = std::get<std::string>(baseToken->getValue());
    const Usable* usable = m_context.symbolTable.lookup(baseName);
    
    if (!usable)
    {
        m_context.log(errors::semantics::UNDECLARED_IDENTIFIER_N.args(*baseToken, m_context.source, *baseToken));
        DiagnosticCollector::addDiagnostic(errors::semantics::UNDECLARED_IDENTIFIER_N);
    }

    // Evaluate overlay target expressions
    visit(node.getOverlayDeclaration());
}

void SymbolResolutionPass::visit(const syntax::ProcedureDeclaration& node)
{
    auto id = node.getName();
    auto type = createTypeFromToken(node.getTypeToken());
    auto typeCopy = IntrusiveVariantValue<Type>(ProcedureType{nullptr, {}}); // Dummy procedure type

    auto declaration = makeIntrusiveVariant<ProcedureDeclaration>(id, std::move(typeCopy));
    const auto& name = std::get<std::string>(id->getValue());

    auto registerProcedure = [&]()
    {
        if (m_context.symbolTable.lookupInCurrentScope(name))
        {
            m_context.log(errors::semantics::REDEFINITION_OF_SYMBOL_N.args(*id, m_context.source, *id));
            DiagnosticCollector::addDiagnostic(errors::semantics::REDEFINITION_OF_SYMBOL_N);
        }
        else
        {
            m_context.symbolTable.insert(name, declaration.get());
        }
    };

    if (node.isRecursive())
    {
        registerProcedure();
    }

    m_context.symbolTable.enterScope();
    for (const auto& param : node.getParameters())
    {
        visit(*param);
    }
    visit(node.getBody());
    m_context.symbolTable.exitScope();

    if (!node.isRecursive())
    {
        registerProcedure();
    }

    m_context.ownedUsables.push_back(std::move(declaration));
}

void SymbolResolutionPass::visit(const syntax::ParameterDeclaration& node)
{
    auto type = createTypeFromToken(node.getType());
    for (auto id : node.getIdentifiers())
    {
        IntrusiveVariantValue<Type> typeCopy{PrimitiveType{PrimitiveType::Integer}};

        switch (node.getMode())
        {
        case syntax::ParameterDeclaration::ParameterMode::Value:
            if (type->is<PrimitiveType>())
                typeCopy = IntrusiveVariantValue(PrimitiveType{type->as<PrimitiveType>().getKind(), Type::Value});
            else if (type->is<FixedType>())
                typeCopy = IntrusiveVariantValue(
                    FixedType{type->as<FixedType>().getTotalBits(), type->as<FixedType>().getFractionalBits(), Type::Value});
            break;
        case syntax::ParameterDeclaration::ParameterMode::Location:
            if (type->is<PrimitiveType>())
                typeCopy = IntrusiveVariantValue(PrimitiveType{type->as<PrimitiveType>().getKind(), Type::Location});
            else if (type->is<FixedType>())
                typeCopy = IntrusiveVariantValue(
                    FixedType{type->as<FixedType>().getTotalBits(), type->as<FixedType>().getFractionalBits(), Type::Location});
            break;
        case syntax::ParameterDeclaration::ParameterMode::Array:
            typeCopy = IntrusiveVariantValue(ArrayType{&untypedType, {}});
            break;
        case syntax::ParameterDeclaration::ParameterMode::Label:
            typeCopy = IntrusiveVariantValue(LabelType{});
            break;
        case syntax::ParameterDeclaration::ParameterMode::Switch:
            typeCopy = IntrusiveVariantValue(SwitchType{});
            break;
        case syntax::ParameterDeclaration::ParameterMode::Procedure:
            typeCopy = IntrusiveVariantValue(ProcedureType{nullptr, {}});
            break;
        case syntax::ParameterDeclaration::ParameterMode::UntypedValue:
            typeCopy = IntrusiveVariantValue(UntypedType{Type::Value});
            break;
        case syntax::ParameterDeclaration::ParameterMode::UntypedLocation:
            typeCopy = IntrusiveVariantValue(UntypedType{Type::Location});
            break;
        }

        auto declaration = makeIntrusiveVariant<VariableDeclaration>(id, std::move(typeCopy));
        const auto& name = std::get<std::string>(id->getValue());

        if (m_context.symbolTable.lookupInCurrentScope(name))
        {
            m_context.log(errors::semantics::REDEFINITION_OF_SYMBOL_N.args(*id, m_context.source, *id));
            DiagnosticCollector::addDiagnostic(errors::semantics::REDEFINITION_OF_SYMBOL_N);
        }
        else
        {
            m_context.symbolTable.insert(name, declaration.get());
        }
        m_context.ownedUsables.push_back(std::move(declaration));
    }
}

void SymbolResolutionPass::visit(const syntax::Expression& node)
{
    if (auto* idExpr = dynamic_cast<const syntax::IdentifierExpression*>(&node))
        visit(*idExpr);
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
void SymbolResolutionPass::visit(const syntax::IdentifierExpression& node)
{
    const auto& name = std::get<std::string>(node.getIdentifier()->getValue());
    const Usable* usable = m_context.symbolTable.lookup(name);
    if (!usable)
    {
        m_context.log(errors::semantics::UNDECLARED_IDENTIFIER_N.args(*node.getIdentifier(), m_context.source,
                                                                      *node.getIdentifier()));
        DiagnosticCollector::addDiagnostic(errors::semantics::UNDECLARED_IDENTIFIER_N);
    }
    else
    {
        m_context.resolvedIdentifiers[&node] = usable;
    }
}

void SymbolResolutionPass::visit(const syntax::BinaryExpression& node)
{
    visit(node.getLeft());
    visit(node.getRight());
}

void SymbolResolutionPass::visit(const syntax::UnaryExpression& node)
{
    visit(node.getOperand());
}

void SymbolResolutionPass::visit(const syntax::PartWordExpression& node)
{
    visit(node.getBase());
    visit(node.getStartBit());
    visit(node.getWidth());
}

void SymbolResolutionPass::visit(const syntax::ConditionalExpression& node)
{
    visit(node.getCondition());
    visit(node.getThenBranch());
    visit(node.getElseBranch());
}

void SymbolResolutionPass::visit(const syntax::FunctionCallExpression& node)
{
    const auto& name = std::get<std::string>(node.getFunction()->getValue());
    const Usable* usable = m_context.symbolTable.lookup(name);
    if (!usable)
    {
        m_context.log(errors::semantics::UNDECLARED_IDENTIFIER_N.args(*node.getFunction(), m_context.source,
                                                                      *node.getFunction()));
        DiagnosticCollector::addDiagnostic(errors::semantics::UNDECLARED_IDENTIFIER_N);
    }
    else
    {
        m_context.resolvedIdentifiers[&node] = usable;
    }

    for (const auto& arg : node.getArguments())
    {
        visit(*arg);
    }
}

void SymbolResolutionPass::visit(const syntax::ArrayAccessExpression& node)
{
    if (node.getArray())
    {
        const auto& name = std::get<std::string>(node.getArray()->getValue());
        const Usable* usable = m_context.symbolTable.lookup(name);
        if (!usable)
        {
            m_context.log(
                errors::semantics::UNDECLARED_IDENTIFIER_N.args(*node.getArray(), m_context.source, *node.getArray()));
            DiagnosticCollector::addDiagnostic(errors::semantics::UNDECLARED_IDENTIFIER_N);
        }
        else
        {
            m_context.resolvedIdentifiers[&node] = usable;
        }
    }
    for (const auto& idx : node.getIndices())
    {
        visit(*idx);
    }
}

void SymbolResolutionPass::visit(const syntax::LocationExpression& node)
{
    visit(node.getWordReference());
}

void SymbolResolutionPass::visit(const syntax::TypeChangeExpression& node)
{
    if (node.getTotalBits())
        visit(*node.getTotalBits());
    if (node.getFractionBits())
        visit(*node.getFractionBits());
    visit(node.getExpression());
}

} // namespace coral66::semantics
