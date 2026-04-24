#include "lib/frontend/common/diagnostic_collection.hpp"
/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "symbol_resolution_pass.hpp"
#include "lib/frontend/common/diagnostic_collection.hpp"
#include "lib/frontend/common/messages/errors.hpp"
#include <string>

namespace coral66::semantics
{
using DiagnosticCollector = DiagnosticCollection<detail::Diagnostic::DiagnosticBase>;

static PrimitiveType integerType{PrimitiveType::Integer};
static PrimitiveType floatingType{PrimitiveType::Floating};
static FixedType fixedType{16, 8};
static UntypedType untypedType{};

template <class T, class... Args>
IntrVarPtr<T> makeIntr(Args&&... args)
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

    // In a full implementation, we might want to do a pre-pass to gather labels here.
    // For now, we process declarations and statements in order.
    
    for (const auto& decl : node.getDeclarations())
    {
        visit(*decl);
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
        if (element.step) visit(*element.step);
        if (element.until) visit(*element.until);
        if (element.whileCondition) visit(*element.whileCondition);
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

IntrusiveVariantValue<Type> SymbolResolutionPass::createTypeFromToken(std::optional<lexer::CTokenIterator> typeToken)
{
    if (!typeToken)
        return IntrusiveVariantValue<Type>(UntypedType{});
        
    auto type = typeToken.value()->getTokenType();
    if (type == lexer::TokenType::FloatingKeyword)
        return IntrusiveVariantValue<Type>(PrimitiveType{PrimitiveType::Floating});
    if (type == lexer::TokenType::FixedKeyword)
        return IntrusiveVariantValue<Type>(FixedType{16, 8});
        
    return IntrusiveVariantValue<Type>(PrimitiveType{PrimitiveType::Integer});
}

void SymbolResolutionPass::visit(const syntax::Declaration& node)
{
    if (auto* varDecl = dynamic_cast<const syntax::VariableDeclaration*>(&node))
        visit(*varDecl);
    else if (auto* procDecl = dynamic_cast<const syntax::ProcedureDeclaration*>(&node))
        visit(*procDecl);
    else if (auto* arrDecl = dynamic_cast<const syntax::ArrayDeclaration*>(&node))
        visit(*arrDecl);
    else if (auto* tabDecl = dynamic_cast<const syntax::TableDeclaration*>(&node))
        visit(*tabDecl);
    else if (auto* ovrDecl = dynamic_cast<const syntax::OverlayDeclaration*>(&node))
        visit(*ovrDecl);
    else if (auto* paramDecl = dynamic_cast<const syntax::ParameterDeclaration*>(&node))
        visit(*paramDecl);
}

void SymbolResolutionPass::visit(const syntax::VariableDeclaration& node)
{
    auto type = createTypeFromToken(node.getType());
    for (auto id : node.getIdentifiers())
    {
        auto typeCopy = IntrusiveVariantValue<Type>(PrimitiveType{PrimitiveType::Integer});
        if (type->is<PrimitiveType>())
            typeCopy = IntrusiveVariantValue<Type>(PrimitiveType{type->as<PrimitiveType>().getKind()});
        else if (type->is<FixedType>())
            typeCopy = IntrusiveVariantValue<Type>(FixedType{type->as<FixedType>().getTotalBits(), type->as<FixedType>().getFractionalBits()});
            
        auto decl = makeIntr<VariableDeclaration>(id, std::move(typeCopy));
        const auto& name = std::get<std::string>(id->getValue());
        
        if (m_context.symbolTable.lookupInCurrentScope(name))
        {
            m_context.log(errors::semantics::REDEFINITION_OF_SYMBOL_N.args(*id, m_context.source, *id));
            DiagnosticCollector::addDiagnostic(errors::semantics::REDEFINITION_OF_SYMBOL_N);
        }
        else
        {
            m_context.symbolTable.insert(name, decl.get());
        }
        m_context.ownedUsables.push_back(std::move(decl));
    }
}

void SymbolResolutionPass::visit(const syntax::ArrayDeclaration& node)
{
    for (auto id : node.getIdentifiers())
    {
        std::vector<std::uint64_t> dims(node.getDimensions().size(), 10); // Dummy dimensions
        auto typeCopy = IntrusiveVariantValue<Type>(ArrayType{&untypedType, dims});
        
        auto decl = makeIntr<ArrayDeclaration>(id, std::move(typeCopy));
        const auto& name = std::get<std::string>(id->getValue());
        
        if (m_context.symbolTable.lookupInCurrentScope(name))
        {
            m_context.log(errors::semantics::REDEFINITION_OF_SYMBOL_N.args(*id, m_context.source, *id));
            DiagnosticCollector::addDiagnostic(errors::semantics::REDEFINITION_OF_SYMBOL_N);
        }
        else
        {
            m_context.symbolTable.insert(name, decl.get());
        }
        m_context.ownedUsables.push_back(std::move(decl));
    }
    
    // Visit dimension expressions
    for (const auto& dim : node.getDimensions())
    {
        if (dim.first) visit(*dim.first);
        if (dim.second) visit(*dim.second);
    }
}

void SymbolResolutionPass::visit(const syntax::TableDeclaration& node)
{
    if (node.getWidth()) visit(*node.getWidth());
    if (node.getLength()) visit(*node.getLength());

    auto id = node.getIdentifier();
    auto typeCopy = IntrusiveVariantValue<Type>(TableType{*new TableInfo{std::get<std::string>(id->getValue()), 0, TableDefinition{{}, 1, 1}}});
    auto decl = makeIntr<TableDeclaration>(id, std::move(typeCopy));
    const auto& name = std::get<std::string>(id->getValue());
    
    if (m_context.symbolTable.lookupInCurrentScope(name))
    {
        m_context.log(errors::semantics::REDEFINITION_OF_SYMBOL_N.args(*id, m_context.source, *id));
            DiagnosticCollector::addDiagnostic(errors::semantics::REDEFINITION_OF_SYMBOL_N);
    }
    else
    {
        m_context.symbolTable.insert(name, decl.get());
    }
    m_context.ownedUsables.push_back(std::move(decl));
}

void SymbolResolutionPass::visit(const syntax::OverlayDeclaration& node)
{
    // Evaluate overlay target expressions
    visit(node.getOverlayDeclaration());
}

void SymbolResolutionPass::visit(const syntax::ProcedureDeclaration& node)
{
    auto id = node.getName();
    auto type = createTypeFromToken(node.getTypeToken());
    auto typeCopy = IntrusiveVariantValue<Type>(ProcedureType{nullptr, {}}); // Dummy procedure type
    
    auto decl = makeIntr<ProcedureDeclaration>(id, std::move(typeCopy));
    const auto& name = std::get<std::string>(id->getValue());
    
    if (m_context.symbolTable.lookupInCurrentScope(name))
    {
        m_context.log(errors::semantics::REDEFINITION_OF_SYMBOL_N.args(*id, m_context.source, *id));
            DiagnosticCollector::addDiagnostic(errors::semantics::REDEFINITION_OF_SYMBOL_N);
    }
    else
    {
        m_context.symbolTable.insert(name, decl.get());
    }
    m_context.ownedUsables.push_back(std::move(decl));

    m_context.symbolTable.enterScope();
    for (const auto& param : node.getParameters())
    {
        visit(*param);
    }
    visit(node.getBody());
    m_context.symbolTable.exitScope();
}

void SymbolResolutionPass::visit(const syntax::ParameterDeclaration& node)
{
    auto type = createTypeFromToken(node.getType());
    for (auto id : node.getIdentifiers())
    {
        auto typeCopy = IntrusiveVariantValue<Type>(PrimitiveType{PrimitiveType::Integer});
        auto decl = makeIntr<VariableDeclaration>(id, std::move(typeCopy));
        const auto& name = std::get<std::string>(id->getValue());
        
        if (m_context.symbolTable.lookupInCurrentScope(name))
        {
            m_context.log(errors::semantics::REDEFINITION_OF_SYMBOL_N.args(*id, m_context.source, *id));
            DiagnosticCollector::addDiagnostic(errors::semantics::REDEFINITION_OF_SYMBOL_N);
        }
        else
        {
            m_context.symbolTable.insert(name, decl.get());
        }
        m_context.ownedUsables.push_back(std::move(decl));
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

void SymbolResolutionPass::visit(const syntax::IdentifierExpression& node)
{
    const auto& name = std::get<std::string>(node.getIdentifier()->getValue());
    const Usable* usable = m_context.symbolTable.lookup(name);
    if (!usable)
    {
        m_context.log(errors::semantics::UNDECLARED_IDENTIFIER_N.args(*node.getIdentifier(), m_context.source, *node.getIdentifier()));
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
        m_context.log(errors::semantics::UNDECLARED_IDENTIFIER_N.args(*node.getFunction(), m_context.source, *node.getFunction()));
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
            m_context.log(errors::semantics::UNDECLARED_IDENTIFIER_N.args(*node.getArray(), m_context.source, *node.getArray()));
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
    if (node.getTotalBits()) visit(*node.getTotalBits());
    if (node.getFractionBits()) visit(*node.getFractionBits());
    visit(node.getExpression());
}

} // namespace coral66::semantics
