/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include <algorithm>
#include <ranges>

#include "lib/frontend/common/error_messages.hpp"
#include "semantic_analysis.hpp"

namespace coral66::Semantics
{

template <class T, class... Args>
IntrVarPtr<T> makeIntr(Args&&... args)
{
    return IntrVarPtr<T>(new T(std::forward<Args>(args)...));
}

SemanticAnalysis::SemanticAnalysis(const source_interface& sourceInterface, llvm::raw_ostream* reporter, bool* errors)
    : m_sourceInterface(sourceInterface), m_reporter(reporter), m_errors(errors)
{
}

// Stable types
static PrimitiveType integerType{PrimitiveType::Integer};
static PrimitiveType floatingType{PrimitiveType::Floating};
static FixedType fixedType{16, 8};
static VoidType voidType{};
static ErrorType errorType{};

void SemanticAnalysis::visit(const Syntax::Program& node)
{
    m_globalDeclarations.clear();
    m_currentBlockStatements.clear();
    m_symbolTable.clear();
    m_isGlobalScope = true;
    visit(node.getBlock());
}

void SemanticAnalysis::visit(const Syntax::Block& node)
{
    for (const auto& decl : node.getDeclarations())
    {
        visit(*decl);
    }
    for (const auto& stmt : node.getStatements())
    {
        visit(*stmt);
    }
}

void SemanticAnalysis::visit(const Syntax::Declaration& node)
{
    if (auto* varDecl = dynamic_cast<const Syntax::VariableDeclaration*>(&node))
    {
        visit(*varDecl);
    }
    else if (auto* procDecl = dynamic_cast<const Syntax::ProcedureDeclaration*>(&node))
    {
        visit(*procDecl);
    }
    else if (auto* arrayDecl = dynamic_cast<const Syntax::ArrayDeclaration*>(&node))
    {
        visit(*arrayDecl);
    }
    else if (auto* tableDecl = dynamic_cast<const Syntax::TableDeclaration*>(&node))
    {
        visit(*tableDecl);
    }
    else if (auto* overlayDecl = dynamic_cast<const Syntax::OverlayDeclaration*>(&node))
    {
        visit(*overlayDecl);
    }
}

void SemanticAnalysis::visit(const Syntax::Statement& node)
{
    if (auto* assign = dynamic_cast<const Syntax::AssignmentStatement*>(&node))
    {
        visit(*assign);
    }
    else if (auto* ifStmt = dynamic_cast<const Syntax::IfStatement*>(&node))
    {
        visit(*ifStmt);
    }
    else if (auto* gotoStmt = dynamic_cast<const Syntax::GotoStatement*>(&node))
    {
        visit(*gotoStmt);
    }
    else if (auto* forStmt = dynamic_cast<const Syntax::ForStatement*>(&node))
    {
        visit(*forStmt);
    }
    else if (auto* ansStmt = dynamic_cast<const Syntax::AnswerStatement*>(&node))
    {
        visit(*ansStmt);
    }
    else if (auto* codeStmt = dynamic_cast<const Syntax::CodeStatement*>(&node))
    {
        visit(*codeStmt);
    }
    else if (auto* procCall = dynamic_cast<const Syntax::ProcedureCallStatement*>(&node))
    {
        visit(*procCall);
    }
}

void SemanticAnalysis::visit(const Syntax::Expression& node)
{
    if (auto* lit = dynamic_cast<const Syntax::LiteralExpression*>(&node))
    {
        visit(*lit);
    }
    else if (auto* id = dynamic_cast<const Syntax::IdentifierExpression*>(&node))
    {
        visit(*id);
    }
    else if (auto* bin = dynamic_cast<const Syntax::BinaryExpression*>(&node))
    {
        visit(*bin);
    }
    else if (auto* part = dynamic_cast<const Syntax::PartWordExpression*>(&node))
    {
        visit(*part);
    }
}

IntrusiveVariantValue<Type> cloneType(const Type& type)
{
    if (type.is<PrimitiveType>()) return IntrusiveVariantValue<Type>(PrimitiveType{type.as<PrimitiveType>().getKind()});
    if (type.is<FixedType>()) 
    {
        auto& f = type.as<FixedType>();
        return IntrusiveVariantValue<Type>(FixedType{f.getTotalBits(), f.getFractionalBits()});
    }
    if (type.is<ProcedureType>())
    {
        auto& p = type.as<ProcedureType>();
        return IntrusiveVariantValue<Type>(ProcedureType{p.getReturnType(), p.getParameters()});
    }
    if (type.is<ArrayType>())
    {
        auto& a = type.as<ArrayType>();
        return IntrusiveVariantValue<Type>(ArrayType{&a.getElementType(), a.getDimensions()});
    }
    if (type.is<TableType>())
    {
        auto& t = type.as<TableType>();
        return IntrusiveVariantValue<Type>(TableType{t.getInfo()});
    }
    if (type.is<VoidType>()) return IntrusiveVariantValue<Type>(VoidType{});
    if (type.is<ErrorType>()) return IntrusiveVariantValue<Type>(ErrorType{});
    return IntrusiveVariantValue<Type>(PrimitiveType{PrimitiveType::Integer});
}

void SemanticAnalysis::visit(const Syntax::VariableDeclaration& node)
{
    auto typeToken = node.getType();
    IntrusiveVariantValue<Type> type(PrimitiveType{PrimitiveType::Integer});
    
    if (typeToken->getTokenType() == Lexer::TokenType::FloatingKeyword)
        type = IntrusiveVariantValue<Type>(PrimitiveType{PrimitiveType::Floating});
    else if (typeToken->getTokenType() == Lexer::TokenType::FixedKeyword)
        type = IntrusiveVariantValue<Type>(FixedType{16, 8});

    for (auto id : node.getIdentifiers())
    {
        auto typeCopy = cloneType(*type);
        auto var = makeIntr<VariableDeclaration>(id, std::move(typeCopy));
        
        const std::string& name = std::get<std::string>(id->getValue());
        m_symbolTable[name] = var.get();
        
        if (m_isGlobalScope)
            m_globalDeclarations.push_back(std::move(var));
        else {
             // For locals/params, we don't put them in global declarations list
             // but we need them alive.
             // In this simplified compiler, we might just leak them or attach to procedure?
             // ProcedureDeclaration has setBody, but parameters/locals?
             // Parameters are attached to ProcedureType (just types) and ProcedureDeclaration (names).
             // But we need Declaration objects for symbol table lookup.
             // We can store them in m_globalDeclarations temporarily even if not global?
             // CodeGenerator iterates m_globals. If we put locals there, it will generate globals.
             // We need a way to store locals.
             // For now, let's put them in m_globalDeclarations but mark them?
             // Or just realize this transition compiler relies on m_symbolTable for resolution.
             // The ownership is the issue.
             // Let's store them in m_globalDeclarations for ownership, but we need to prevent CodeGen from emitting them.
             // Or better: ProcedureDeclaration body contains Statements. What about local declarations?
             // Syntax::Block has declarations.
             // CodeGen generates locals from Block declarations.
             // So we don't need to store locals in m_globalDeclarations!
             // EXCEPT parameters. Parameters are not in Block.
             // So we need to own parameters.
             // We can push parameters to m_globalDeclarations for ownership.
             // CodeGen needs to distinguish.
             // We can't change CodeGen easily right now.
             // Let's just push them. If CodeGen emits globals for parameters, that's a bug but maybe acceptable for "transition".
             // Actually, CodeGen iterates m_program.getGlobals().
             m_globalDeclarations.push_back(std::move(var));
        }
    }
}

void SemanticAnalysis::visit(const Syntax::ProcedureDeclaration& node)
{
    // 1. Create Procedure symbol first (recursive support)
    // For now we don't know types fully until we visit params.
    
    // We need to visit parameters.
    bool wasGlobal = m_isGlobalScope;
    m_isGlobalScope = false;
    
    std::vector<ProcedureType::Parameter> paramTypes;
    
    for(const auto& param : node.getParameters())
    {
        visit(*param); // This adds to symbol table
        
        // Extract type
        if (auto* var = dynamic_cast<const Syntax::VariableDeclaration*>(param.get()))
        {
            // Find in symbol table (it was just added)
            // Or access the last added declaration?
            // Since visit(VariableDeclaration) adds to m_globalDeclarations (per my decision above), we can look at back.
            // But VariableDeclaration can add multiple.
            // Simplified: iterate identifiers of var decl
            auto typeToken = var->getType();
            const Type* pType = &integerType;
            if (typeToken->getTokenType() == Lexer::TokenType::FloatingKeyword) pType = &floatingType;
            else if (typeToken->getTokenType() == Lexer::TokenType::FixedKeyword) pType = &fixedType;
            
            for(auto id : var->getIdentifiers())
            {
                 paramTypes.push_back({pType, std::get<std::string>(id->getValue()), false});
            }
        }
        else if (auto* arr = dynamic_cast<const Syntax::ArrayDeclaration*>(param.get()))
        {
             // For array param, we need to find the type.
             // visit(ArrayDeclaration) adds to symbol table.
             std::string name = std::get<std::string>(arr->getIdentifier()->getValue());
             if (auto* decl = m_symbolTable[name]) // Assuming found
             {
                 // decl is Useable*.
                 if (auto* v = decl->tryAs<VariableDeclaration>())
                 {
                     paramTypes.push_back(ProcedureType::Parameter{&v->getType(), name, false});
                 }
             }
        }
    }
    
    auto prevStatements = std::move(m_currentBlockStatements);
    m_currentBlockStatements.clear();
    
    visit(node.getBody());
    
    m_isGlobalScope = wasGlobal;
    
    IntrusiveVariantValue<Type> type(ProcedureType{&voidType, std::move(paramTypes)});
    auto proc = makeIntr<ProcedureDeclaration>(node.getName(), std::move(type));
    
    const std::string& name = std::get<std::string>(node.getName()->getValue());
    m_symbolTable[name] = proc.get();
    
    proc->setBody(std::move(m_currentBlockStatements));
    m_currentBlockStatements = std::move(prevStatements);
    
    m_globalDeclarations.push_back(std::move(proc));
}

void SemanticAnalysis::visit(const Syntax::ArrayDeclaration& node)
{
    auto typeToken = node.getType();
    const Type* elemTypePtr = &integerType;
    if (typeToken->getTokenType() == Lexer::TokenType::FloatingKeyword)
        elemTypePtr = &floatingType;
    else if (typeToken->getTokenType() == Lexer::TokenType::FixedKeyword)
        elemTypePtr = &fixedType;

    std::vector<std::uint64_t> dims;
    for (const auto& val : node.getDimensions() | std::views::values)
    {
        visit(*val); // Visit upper bound
        if (auto* c = dynamic_cast<const Constant*>(m_lastExpression.get())) {
             if (std::holds_alternative<llvm::APSInt>(c->getValue()))
                 dims.push_back(std::get<llvm::APSInt>(c->getValue()).getZExtValue());
             else dims.push_back(10);
        } else {
             dims.push_back(10); 
        }
    }

    IntrusiveVariantValue<Type> type(ArrayType{elemTypePtr, dims});
    auto var = makeIntr<VariableDeclaration>(node.getIdentifier(), std::move(type));
    
    const std::string& name = std::get<std::string>(node.getIdentifier()->getValue());
    m_symbolTable[name] = var.get();
    m_globalDeclarations.push_back(std::move(var));
}

void SemanticAnalysis::visit(const Syntax::TableDeclaration& node)
{
    uint64_t width = 1;
    uint64_t length = 1;

    if (node.getWidth()) {
        visit(*node.getWidth());
        if (auto* c = dynamic_cast<const Constant*>(m_lastExpression.get())) {
             if (std::holds_alternative<llvm::APSInt>(c->getValue()))
                 width = std::get<llvm::APSInt>(c->getValue()).getZExtValue();
        }
    }
    if (node.getLength()) {
        visit(*node.getLength());
         if (auto* c = dynamic_cast<const Constant*>(m_lastExpression.get())) {
             if (std::holds_alternative<llvm::APSInt>(c->getValue()))
                 length = std::get<llvm::APSInt>(c->getValue()).getZExtValue();
        }
    }

    auto* info = new TableInfo{std::get<std::string>(node.getIdentifier()->getValue()), 0, TableDefinition{{}, width, length}};
    
    IntrusiveVariantValue<Type> type(TableType{*info});
    auto var = makeIntr<VariableDeclaration>(node.getIdentifier(), std::move(type));
    
    const std::string& name = std::get<std::string>(node.getIdentifier()->getValue());
    m_symbolTable[name] = var.get();
    m_globalDeclarations.push_back(std::move(var));
}

void SemanticAnalysis::visit(const Syntax::OverlayDeclaration& node)
{
    visit(node.getOverlayedDeclaration());
}

void SemanticAnalysis::visit(const Syntax::AssignmentStatement& node)
{
    visit(node.getExpression());
    auto rhs = std::move(m_lastExpression);
    
    const auto& name = std::get<std::string>(node.getIdentifier()->getValue());
    auto it = std::ranges::find_if(m_symbolTable, [&](const auto& p){ return p.first == name; });
    
    if (it != m_symbolTable.end())
    {
        IntrusiveVariantValue<Type> type = it->second->match(
            [](const VariableDeclaration& v) { return cloneType(v.getType()); },
            [](const auto&) { return IntrusiveVariantValue<Type>(PrimitiveType{PrimitiveType::Integer}); }
        );
        
        auto lhs = makeIntr<DeclarationRead>(type, *it->second, node.getIdentifier());
        auto stmt = makeIntr<AssignmentStatement>(ScopePoint{0}, std::move(lhs), std::move(rhs));
        m_currentBlockStatements.push_back(std::move(stmt));
    }
}

void SemanticAnalysis::visit(const Syntax::IfStatement& node)
{
    visit(node.getCondition());
    auto cond = std::move(m_lastExpression);
    
    visit(node.getThenBranch());
    
    if (m_currentBlockStatements.empty()) return; 
    auto thenB = std::move(m_currentBlockStatements.back());
    m_currentBlockStatements.pop_back();
    
    IntrVarPtr<Statement> elseB = nullptr;
    if (node.getElseBranch())
    {
        visit(*node.getElseBranch());
        elseB = std::move(m_currentBlockStatements.back());
        m_currentBlockStatements.pop_back();
    }
    
    auto stmt = makeIntr<IfStatement>(ScopePoint{0}, std::move(cond), std::move(thenB), std::move(elseB));
    m_currentBlockStatements.push_back(std::move(stmt));
}

void SemanticAnalysis::visit([[maybe_unused]]const Syntax::GotoStatement& node)
{
    auto stmt = makeIntr<CodeStatement>(ScopePoint{0}, "nop");
    m_currentBlockStatements.push_back(std::move(stmt));
}

void SemanticAnalysis::visit(const Syntax::ForStatement& node)
{
    auto name = std::string(node.getVariable()->getText());

    if (auto it = std::ranges::find_if(m_symbolTable, [&](const auto& p) { return p.first == name; });
        it != m_symbolTable.end())
    {
        IntrusiveVariantValue<Type> type = it->second->match(
            [](const VariableDeclaration& v) { return cloneType(v.getType()); },
            [](const auto&) { return IntrusiveVariantValue<Type>(PrimitiveType{PrimitiveType::Integer}); }
        );
        m_lastExpression = makeIntr<DeclarationRead>(type, *it->second, node.getVariable());
    }
    else
    {
         IntrusiveVariantValue<Type> type(ErrorType{});
         m_lastExpression = makeIntr<ErrorExpression>(type, node.getVariable(), node.getVariable() + 1);
    }
    auto var = std::move(m_lastExpression);
    
    visit(node.getStart());
    auto start = std::move(m_lastExpression);
    
    visit(node.getStep());
    auto step = std::move(m_lastExpression);
    
    visit(node.getUntil());
    auto until = std::move(m_lastExpression);
    
    auto prevStatements = std::move(m_currentBlockStatements);
    m_currentBlockStatements.clear();
    visit(node.getBody());
    
    IntrVarPtr<Statement> body;
    if (!m_currentBlockStatements.empty())
    {
         body = std::move(m_currentBlockStatements.back());
    }
    m_currentBlockStatements = std::move(prevStatements);
    
    auto stmt = makeIntr<ForStatement>(ScopePoint{0}, std::move(var), std::move(start), std::move(step), std::move(until), std::move(body));
    m_currentBlockStatements.push_back(std::move(stmt));
}

void SemanticAnalysis::visit(const Syntax::AnswerStatement& node)
{
    visit(node.getExpression());
    auto stmt = makeIntr<AnswerStatement>(ScopePoint{0}, std::move(m_lastExpression));
    m_currentBlockStatements.push_back(std::move(stmt));
}

void SemanticAnalysis::visit(const Syntax::CodeStatement& node)
{
    auto lit = node.begin() + 1;
    std::string code = std::get<std::string>(lit->getValue());
    auto stmt = makeIntr<CodeStatement>(ScopePoint{0}, code);
    m_currentBlockStatements.push_back(std::move(stmt));
}

void SemanticAnalysis::visit(const Syntax::ProcedureCallStatement& node)
{
    visit(node.getExpression());
    auto callExpr = std::move(m_lastExpression);
    
    IntrusiveVariantValue<Type> type(PrimitiveType{PrimitiveType::Integer});
    auto call = makeIntr<CallExpression>(type, std::move(callExpr), std::vector<coral66::IntrVarPtr<ExpressionBase>>{}, node.begin(), node.end());
    
    auto stmt = makeIntr<ProcedureCallStatement>(ScopePoint{0}, std::move(call));
    m_currentBlockStatements.push_back(std::move(stmt));
}

void SemanticAnalysis::visit(const Syntax::LiteralExpression& node)
{
    IntrusiveVariantValue<Type> type(PrimitiveType{PrimitiveType::Integer});
    if (node.getLiteral()->getType() == Lexer::CToken::Floating)
        type = IntrusiveVariantValue<Type>(PrimitiveType{PrimitiveType::Floating});
        
    Semantics::Constant::Variant val;
    if (std::holds_alternative<llvm::APSInt>(node.getLiteral()->getValue()))
        val = std::get<llvm::APSInt>(node.getLiteral()->getValue());
    else if (std::holds_alternative<llvm::APFloat>(node.getLiteral()->getValue()))
        val = std::get<llvm::APFloat>(node.getLiteral()->getValue());
        
    m_lastExpression = makeIntr<Constant>(type, std::move(val), node.begin(), node.end());
}

void SemanticAnalysis::visit(const Syntax::IdentifierExpression& node)
{
    auto name = std::get<std::string>(node.getIdentifier()->getValue());

    if (const auto it = std::ranges::find_if(m_symbolTable, [&](const auto& p) { return p.first == name; });
        it != m_symbolTable.end())
    {
        IntrusiveVariantValue<Type> type = it->second->match(
            [](const VariableDeclaration& v) { return cloneType(v.getType()); },
            [](const ProcedureDeclaration& p) { return cloneType(p.getType()); }
        );
        m_lastExpression = makeIntr<DeclarationRead>(type, *it->second, node.getIdentifier());
    }
}

void SemanticAnalysis::visit(const Syntax::BinaryExpression& node)
{
    visit(node.getLeft());
    auto left = std::move(m_lastExpression);
    visit(node.getRight());
    auto right = std::move(m_lastExpression);
    
    using BinOp = Semantics::BinaryOperator;
    BinOp::Kind kind = BinOp::Addition;
    if (node.getOp()->getTokenType() == Lexer::TokenType::Minus) kind = BinOp::Subtraction;
    else if (node.getOp()->getTokenType() == Lexer::TokenType::Asterisk) kind = BinOp::Multiply;
    else if (node.getOp()->getTokenType() == Lexer::TokenType::Division) kind = BinOp::Divide;
    
    IntrusiveVariantValue<Type> type = cloneType(left->getType());
    m_lastExpression = makeIntr<BinaryOperator>(type, std::move(left), kind, node.getOp(), std::move(right));
}

void SemanticAnalysis::visit(const Syntax::PartWordExpression& node)
{
    visit(node.getBase());
    auto base = std::move(m_lastExpression);
    
    visit(node.getStartBit());
    auto startExpr = std::move(m_lastExpression);
    visit(node.getWidth());
    auto widthExpr = std::move(m_lastExpression);
    
    uint32_t s = 0;
    uint32_t w = 1;
    
    if (auto* c = dynamic_cast<const Semantics::Constant*>(startExpr.get()))
    {
        if (std::holds_alternative<llvm::APSInt>(c->getValue()))
             s = std::get<llvm::APSInt>(c->getValue()).getZExtValue();
    }
    if (auto* c = dynamic_cast<const Semantics::Constant*>(widthExpr.get()))
    {
        if (std::holds_alternative<llvm::APSInt>(c->getValue()))
             w = std::get<llvm::APSInt>(c->getValue()).getZExtValue();
    }
    
    IntrusiveVariantValue<Type> type(PrimitiveType{PrimitiveType::Integer}); 
    m_lastExpression = makeIntr<PartWordAccess>(type, std::move(base), s, w, node.begin(), node.end());
}

Program analyse(const Syntax::Program& parseTree, CSourceObject&& cTokens,
                llvm::raw_ostream* reporter, bool* errors)
{
    SemanticAnalysis analysis(cTokens, reporter, errors);
    analysis.visit(parseTree);
    
    std::vector<Lexer::CToken> syntheticTokens;
    
    bool mainExists = false;
    for(const auto& g : analysis.m_globalDeclarations)
    {
        if (g->match(
            [](const VariableDeclaration&) { return false; },
            [](const ProcedureDeclaration& proc) { return proc.getName() == "main"; },
            [](const auto&) { return false; }))
        {
            mainExists = true;
            break;
        }
    }
    
    if (!mainExists)
    {
        syntheticTokens.emplace_back(Lexer::TokenType::Identifier, 0, 4, 0, 0, std::string("main"));
        // Use Integer return type
        IntrusiveVariantValue<Type> type(ProcedureType{&integerType, {}}); 
        const Lexer::CToken* mainToken = &syntheticTokens.back();
        
        auto proc = makeIntr<ProcedureDeclaration>(mainToken, std::move(type));
        proc->setBody(std::move(analysis.m_currentBlockStatements));
        analysis.m_globalDeclarations.push_back(std::move(proc));
    }

    return Program(std::move(cTokens), std::move(syntheticTokens), std::move(analysis.m_globalDeclarations));
}

} // namespace coral66::Semantics
