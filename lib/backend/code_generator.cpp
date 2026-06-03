/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "code_generator.hpp"
#include "lib/frontend/semantics/program.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Module.h>

namespace coral66::CGLLVM
{

code_generator::code_generator(llvm::Module& module, const syntax::Program& tree, const semantics::Program& program,
                               const source_interface& sourceInterface, const CGLLVM::Options& options)
    : m_module(module), m_tree(tree), m_program(program), m_sourceInterface(sourceInterface), m_options(options)
{
}

void code_generator::generate()
{
    // First pass: Global Declarations
    // We iterate over semantic usables because the semantic pass already flattened and lifted
    // them. We only declare the ones that are at scope depth 1 (global).
    for (const auto& global : m_program.getGlobals())
    {
        global->match(
            [&](const semantics::VariableDeclaration& var) { visit(var); },
            [&](const semantics::ArrayDeclaration& arr) { visit(arr); },
            [&](const semantics::ProcedureDeclaration& proc) { visit(proc); },
            [](const auto&) {}
        );
    }

    // Second pass: Generate body for the main program and all procedures
    visit(m_tree);
}

llvm::Type* code_generator::convertType(const semantics::Type& type)
{
    auto& ctx = m_module.getContext();
    if (auto* prim = type.tryAs<semantics::PrimitiveType>())
    {
        switch (prim->getKind())
        {
            case semantics::PrimitiveType::Integer: return llvm::Type::getInt32Ty(ctx);
            case semantics::PrimitiveType::Floating: return llvm::Type::getFloatTy(ctx);
            case semantics::PrimitiveType::Bits: return llvm::Type::getInt32Ty(ctx);
        }
    }
    else if (type.is<semantics::UntypedType>())
    {
        return llvm::Type::getVoidTy(ctx);
    }
    else if (auto* arr = type.tryAs<semantics::ArrayType>())
    {
        llvm::Type* elemType = convertType(arr->getElementType());
        llvm::Type* arrType = elemType;
        for (auto it = arr->getDimensions().rbegin(); it != arr->getDimensions().rend(); ++it) {
            arrType = llvm::ArrayType::get(arrType, *it);
        }
        return arrType;
    }
    else if (auto* fixed = type.tryAs<semantics::FixedType>())
    {
        if (fixed->getTotalBits() <= 16)
            return llvm::Type::getInt16Ty(ctx);
        if (fixed->getTotalBits() <= 32)
            return llvm::Type::getInt32Ty(ctx);
        return llvm::Type::getInt64Ty(ctx);
    }
    return llvm::Type::getVoidTy(ctx);
}

llvm::BasicBlock* code_generator::getOrCreateLabel(const syntax::Statement* stmt)
{
    if (!m_labels.contains(stmt))
    {
        m_labels[stmt] = llvm::BasicBlock::Create(m_module.getContext(), "label", m_currentFunction);
    }
    return m_labels[stmt];
}

llvm::Value* code_generator::emitAddress(const syntax::Expression& expr)
{
    if (auto* idExpr = dynamic_cast<const syntax::IdentifierExpression*>(&expr))
    {
        auto it = m_program.getContext().resolvedIdentifiers.find(idExpr);
        if (it != m_program.getContext().resolvedIdentifiers.end()) {
            const auto* decl = it->second;
            if (m_locals.contains(decl))
                return m_locals[decl];
            if (m_globals.contains(decl))
                return m_globals[decl];
        }
    }
    else if (auto* part = dynamic_cast<const syntax::PartWordExpression*>(&expr))
    {
        return emitAddress(part->getBase());
    }
    else if (auto* arrAccess = dynamic_cast<const syntax::ArrayAccessExpression*>(&expr))
    {
        auto it = m_program.getContext().resolvedIdentifiers.find(arrAccess);
        if (it == m_program.getContext().resolvedIdentifiers.end()) return nullptr;
        
        const auto* decl = it->second;
        llvm::Value* baseAddr = nullptr;
        if (m_locals.contains(decl))
            baseAddr = m_locals[decl];
        else if (m_globals.contains(decl))
            baseAddr = m_globals[decl];

        if (!baseAddr) return nullptr;

        std::vector<llvm::Value*> indices;
        indices.push_back(llvm::ConstantInt::get(m_module.getContext(), llvm::APInt(32, 0)));
        for (const auto& idxExpr : arrAccess->getIndices())
        {
            llvm::Value* idxVal = visit(*idxExpr);
            indices.push_back(idxVal);
        }
        
        auto typeIt = m_program.getContext().expressionTypes.find(arrAccess->getIndices().front().get());
        // Wait, need the type of the array itself.
        // Actually we can get it from the declaration.
        llvm::Type* baseType = nullptr;
        decl->match(
            [&](const semantics::ArrayDeclaration& arrDecl) { baseType = convertType(arrDecl.getType()); },
            [&](const auto&) {}
        );
        
        if (baseType)
            return m_builder.CreateGEP(baseType, baseAddr, indices, "arrayidx");
    }
    return nullptr;
}

void code_generator::visit(const semantics::Declaration& decl)
{
    // Only process global declarations here.
    // if (decl.getScopeDepth() != 1) return;

    if (auto* var = dynamic_cast<const semantics::VariableDeclaration*>(&decl))
    {
        auto* llvmType = convertType(var->getType());
        auto* globalVar = new llvm::GlobalVariable(m_module, llvmType, false, llvm::GlobalValue::ExternalLinkage,
                                                   llvm::Constant::getNullValue(llvmType), var->getName());
        m_globals[var] = globalVar;
    }
    else if (auto* arr = dynamic_cast<const semantics::ArrayDeclaration*>(&decl))
    {
        auto* llvmType = convertType(arr->getType());
        auto* globalVar = new llvm::GlobalVariable(m_module, llvmType, false, llvm::GlobalValue::ExternalLinkage,
                                                   llvm::Constant::getNullValue(llvmType), arr->getName());
        m_globals[arr] = globalVar;
    }
    else if (auto* proc = dynamic_cast<const semantics::ProcedureDeclaration*>(&decl))
    {
        const auto* procType = proc->getType().tryAs<semantics::ProcedureType>();
        llvm::Type* retType = llvm::Type::getVoidTy(m_module.getContext());
        std::vector<llvm::Type*> paramTypes;
        if (procType && procType->getReturnType())
        {
            retType = convertType(*procType->getReturnType());
        }

        if (procType)
        {
            for (const auto& param : procType->getParameters())
            {
                paramTypes.push_back(convertType(*param.type));
            }
        }

        llvm::FunctionType* ft = llvm::FunctionType::get(retType, paramTypes, false);
        llvm::Function* f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, proc->getName(), m_module);
        m_globals[proc] = f;
    }
}

void code_generator::generateProcedure(const semantics::ProcedureDeclaration& procDecl, const syntax::ProcedureDeclaration& astNode)
{
    m_currentProcedure = &astNode;
    m_labels.clear();
    m_locals.clear();

    llvm::Function* f = llvm::cast<llvm::Function>(m_globals[&procDecl]);
    m_currentFunction = f;

    llvm::BasicBlock* bb = llvm::BasicBlock::Create(m_module.getContext(), "entry", f);
    m_builder.SetInsertPoint(bb);

    // Generate body
    if (true) // astNode.getBody() always returns a reference
    {
        visit(astNode.getBody());
    }

    if (!m_builder.GetInsertBlock()->getTerminator())
    {
        if (m_currentFunction->getReturnType()->isVoidTy())
            m_builder.CreateRetVoid();
        else
            m_builder.CreateRet(llvm::Constant::getNullValue(m_currentFunction->getReturnType()));
    }
}

void code_generator::visit(const syntax::Program& program)
{
    // Generate code for all top-level procedures
    for (const auto& global : m_program.getGlobals())
    {
        if (auto* procDecl = global->tryAs<semantics::ProcedureDeclaration>())
        {
            if (procDecl->getAstNode() /* && procDecl->getScopeDepth() == 1 */)
            {
                if (auto* syntaxProc = dynamic_cast<const syntax::ProcedureDeclaration*>(procDecl->getAstNode()))
                {
                    generateProcedure(*procDecl, *syntaxProc);
                }
            }
        }
    }

    // Now generate the main program block (if any)
    if (program.hasBlock())
    {
        m_labels.clear();
        m_locals.clear();
        
        llvm::FunctionType* ft = llvm::FunctionType::get(llvm::Type::getInt32Ty(m_module.getContext()), false);
        llvm::Function* f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "main", m_module);
        m_currentFunction = f;
        
        llvm::BasicBlock* bb = llvm::BasicBlock::Create(m_module.getContext(), "entry", f);
        m_builder.SetInsertPoint(bb);
        
        visit(program.getBlock());

        if (!m_builder.GetInsertBlock()->getTerminator())
        {
            m_builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_module.getContext()), 0));
        }
    }
}

void code_generator::visit(const syntax::Block& block)
{
    for (const auto& decl : block.getDeclarations())
    {
        visit(*decl);
    }
    for (const auto& stmt : block.getStatements())
    {
        visit(*stmt);
    }
}

void code_generator::visit(const syntax::Declaration& decl)
{
    if (auto* var = dynamic_cast<const syntax::VariableDeclaration*>(&decl))
        visit(*var);
    else if (auto* arr = dynamic_cast<const syntax::ArrayDeclaration*>(&decl))
        visit(*arr);
    else if (auto* proc = dynamic_cast<const syntax::ProcedureDeclaration*>(&decl))
        visit(*proc);
}

void code_generator::visit(const syntax::VariableDeclaration& decl)
{
    auto it = m_program.getContext().resolvedIdentifiers.find(&decl);
    if (it != m_program.getContext().resolvedIdentifiers.end())
    {
        if (auto* semVar = it->second->tryAs<semantics::VariableDeclaration>())
        {
            auto* llvmType = convertType(semVar->getType());
            auto* alloca = m_builder.CreateAlloca(llvmType, nullptr, semVar->getName());
            m_locals[it->second] = alloca;
        }
    }
}

void code_generator::visit(const syntax::ArrayDeclaration& decl)
{
    auto it = m_program.getContext().resolvedIdentifiers.find(&decl);
    if (it != m_program.getContext().resolvedIdentifiers.end())
    {
        if (auto* semArr = it->second->tryAs<semantics::ArrayDeclaration>())
        {
            auto* llvmType = convertType(semArr->getType());
            auto* alloca = m_builder.CreateAlloca(llvmType, nullptr, semArr->getName());
            m_locals[it->second] = alloca;
        }
    }
}

void code_generator::visit(const syntax::ProcedureDeclaration& decl)
{
    // Nested procedures can be handled here if needed.
    // For Phase 1, we only need top-level procedures.
}

void code_generator::visit(const syntax::Statement& stmt)
{
    if (auto* s = dynamic_cast<const syntax::AssignmentStatement*>(&stmt)) visit(*s);
    else if (auto* s = dynamic_cast<const syntax::IfStatement*>(&stmt)) visit(*s);
    else if (auto* s = dynamic_cast<const syntax::GotoStatement*>(&stmt)) visit(*s);
    else if (auto* s = dynamic_cast<const syntax::ForStatement*>(&stmt)) visit(*s);
    else if (auto* s = dynamic_cast<const syntax::AnswerStatement*>(&stmt)) visit(*s);
    else if (auto* s = dynamic_cast<const syntax::CodeStatement*>(&stmt)) visit(*s);
    else if (auto* s = dynamic_cast<const syntax::ProcedureCallStatement*>(&stmt)) visit(*s);
    else if (auto* s = dynamic_cast<const syntax::DummyStatement*>(&stmt)) visit(*s);
    else if (auto* s = dynamic_cast<const syntax::BlockStatement*>(&stmt)) visit(*s);
    else if (auto* s = dynamic_cast<const syntax::CompoundStatement*>(&stmt)) visit(*s);
}

void code_generator::visit(const syntax::DummyStatement& stmt) {}

void code_generator::visit(const syntax::BlockStatement& stmt)
{
    if (stmt.hasBlock())
        visit(stmt.getBlock());
}

void code_generator::visit(const syntax::CompoundStatement& stmt)
{
    for (const auto& s : stmt.getStatements())
        visit(*s);
}

void code_generator::visit(const syntax::AssignmentStatement& stmt)
{
    llvm::Value* targetAddr = emitAddress(stmt.getVariable());
    llvm::Value* rhs = visit(stmt.getExpression());
    if (!targetAddr || !rhs) return;

    auto it = m_program.getContext().expressionTypes.find(&stmt.getVariable());
    llvm::Type* targetType = nullptr;
    if (it != m_program.getContext().expressionTypes.end()) {
        targetType = convertType(*it->second);
    } else {
        // Fallback for arrays
        if (auto* arrAccess = dynamic_cast<const syntax::ArrayAccessExpression*>(&stmt.getVariable())) {
            auto rit = m_program.getContext().resolvedIdentifiers.find(arrAccess);
            if (rit != m_program.getContext().resolvedIdentifiers.end()) {
                if (auto* arrDecl = rit->second->tryAs<semantics::ArrayDeclaration>()) {
                    targetType = convertType(arrDecl->getType().tryAs<semantics::ArrayType>()->getElementType());
                }
            }
        }
    }

    if (rhs->getType() != targetType && targetType)
    {
        if (rhs->getType()->isDoubleTy() && targetType->isIntegerTy())
            rhs = m_builder.CreateFPToSI(rhs, targetType);
        else if (rhs->getType()->isIntegerTy() && targetType->isIntegerTy())
            rhs = m_builder.CreateIntCast(rhs, targetType, true);
    }

    m_builder.CreateStore(rhs, targetAddr);
}

void code_generator::visit(const syntax::IfStatement& stmt)
{
    llvm::Value* cond = visit(stmt.getCondition());
    if (!cond) return;

    if (cond->getType()->isIntegerTy() && cond->getType()->getIntegerBitWidth() > 1)
        cond = m_builder.CreateICmpNE(cond, llvm::ConstantInt::get(cond->getType(), 0));

    llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(m_module.getContext(), "then", m_currentFunction);
    llvm::BasicBlock* elseBB = llvm::BasicBlock::Create(m_module.getContext(), "else");
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(m_module.getContext(), "ifcont");

    bool hasElse = stmt.getElseBranch() != nullptr;
    m_builder.CreateCondBr(cond, thenBB, hasElse ? elseBB : mergeBB);

    m_builder.SetInsertPoint(thenBB);
    visit(stmt.getThenBranch());
    if (!m_builder.GetInsertBlock()->getTerminator())
        m_builder.CreateBr(mergeBB);

    if (hasElse)
    {
        m_currentFunction->insert(m_currentFunction->end(), elseBB);
        m_builder.SetInsertPoint(elseBB);
        visit(*stmt.getElseBranch());
        if (!m_builder.GetInsertBlock()->getTerminator())
            m_builder.CreateBr(mergeBB);
    }
    else
    {
        delete elseBB;
    }

    m_currentFunction->insert(m_currentFunction->end(), mergeBB);
    m_builder.SetInsertPoint(mergeBB);
}

void code_generator::visit(const syntax::GotoStatement& stmt) {} // Skip for Phase 1

void code_generator::visit(const syntax::ForStatement& stmt)
{
    llvm::Value* loopVarAddr = emitAddress(stmt.getVariable());
    llvm::Type* loopVarType = nullptr;
    
    auto it = m_program.getContext().expressionTypes.find(&stmt.getVariable());
    if (it != m_program.getContext().expressionTypes.end()) {
        loopVarType = convertType(*it->second);
    } else {
        loopVarType = llvm::Type::getInt32Ty(m_module.getContext()); // default
    }

    llvm::BasicBlock* endBB = llvm::BasicBlock::Create(m_module.getContext(), "for.end", m_currentFunction);

    for (const auto& el : stmt.getElements())
    {
        llvm::BasicBlock* nextElBB = llvm::BasicBlock::Create(m_module.getContext(), "for.next_el", m_currentFunction);

        if (el.step && el.until)
        {
            llvm::Value* v1 = visit(*el.expression);
            llvm::Value* v2 = visit(*el.step);
            llvm::Value* v3 = visit(*el.until);

            if (v1->getType() != loopVarType) v1 = m_builder.CreateIntCast(v1, loopVarType, true);
            if (v2->getType() != loopVarType) v2 = m_builder.CreateIntCast(v2, loopVarType, true);
            if (v3->getType() != loopVarType) v3 = m_builder.CreateIntCast(v3, loopVarType, true);

            llvm::AllocaInst* v1Addr = m_builder.CreateAlloca(loopVarType, nullptr, "v1");
            llvm::AllocaInst* v2Addr = m_builder.CreateAlloca(loopVarType, nullptr, "v2");
            llvm::AllocaInst* v3Addr = m_builder.CreateAlloca(loopVarType, nullptr, "v3");

            m_builder.CreateStore(v1, v1Addr);
            m_builder.CreateStore(v2, v2Addr);
            m_builder.CreateStore(v3, v3Addr);

            llvm::BasicBlock* condBB = llvm::BasicBlock::Create(m_module.getContext(), "for.step.cond", m_currentFunction);
            llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(m_module.getContext(), "for.step.body", m_currentFunction);
            llvm::BasicBlock* incBB = llvm::BasicBlock::Create(m_module.getContext(), "for.step.inc", m_currentFunction);

            m_builder.CreateBr(condBB);
            m_builder.SetInsertPoint(condBB);

            llvm::Value* currV1 = m_builder.CreateLoad(loopVarType, v1Addr);
            if (loopVarAddr) m_builder.CreateStore(currV1, loopVarAddr);

            llvm::Value* currV2 = m_builder.CreateLoad(loopVarType, v2Addr);
            llvm::Value* currV3 = m_builder.CreateLoad(loopVarType, v3Addr);

            llvm::Value* diff = m_builder.CreateSub(currV1, currV3);
            llvm::Value* prod = m_builder.CreateMul(diff, currV2);
            llvm::Value* isExhausted = m_builder.CreateICmpSGT(prod, llvm::ConstantInt::get(loopVarType, 0));
            m_builder.CreateCondBr(isExhausted, nextElBB, bodyBB);

            m_builder.SetInsertPoint(bodyBB);
            visit(stmt.getBody());
            m_builder.CreateBr(incBB);

            m_builder.SetInsertPoint(incBB);
            llvm::Value* cv = m_builder.CreateLoad(loopVarType, loopVarAddr);
            llvm::Value* nextV1 = m_builder.CreateAdd(cv, currV2);
            m_builder.CreateStore(nextV1, v1Addr);
            m_builder.CreateBr(condBB);
        }
        else if (el.whileCondition)
        {
            llvm::BasicBlock* condBB = llvm::BasicBlock::Create(m_module.getContext(), "for.while.cond", m_currentFunction);
            llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(m_module.getContext(), "for.while.body", m_currentFunction);

            m_builder.CreateBr(condBB);
            m_builder.SetInsertPoint(condBB);

            llvm::Value* exprVal = visit(*el.expression);
            if (exprVal->getType() != loopVarType) exprVal = m_builder.CreateIntCast(exprVal, loopVarType, true);
            if (loopVarAddr) m_builder.CreateStore(exprVal, loopVarAddr);

            llvm::Value* condVal = visit(*el.whileCondition);
            if (condVal->getType() != m_builder.getInt1Ty())
                condVal = m_builder.CreateICmpNE(condVal, llvm::ConstantInt::get(condVal->getType(), 0));

            m_builder.CreateCondBr(condVal, bodyBB, nextElBB);

            m_builder.SetInsertPoint(bodyBB);
            visit(stmt.getBody());
            m_builder.CreateBr(condBB);
        }
        else
        {
            llvm::Value* exprVal = visit(*el.expression);
            if (exprVal->getType() != loopVarType) exprVal = m_builder.CreateIntCast(exprVal, loopVarType, true);
            if (loopVarAddr) m_builder.CreateStore(exprVal, loopVarAddr);

            visit(stmt.getBody());
            m_builder.CreateBr(nextElBB);
        }

        m_builder.SetInsertPoint(nextElBB);
    }

    m_builder.CreateBr(endBB);
    m_builder.SetInsertPoint(endBB);
}

void code_generator::visit(const syntax::AnswerStatement& stmt)
{
    llvm::Value* val = visit(stmt.getExpression());
    if (val && m_currentFunction->getReturnType()->isIntegerTy())
    {
        val = m_builder.CreateIntCast(val, m_currentFunction->getReturnType(), true);
    }
    m_builder.CreateRet(val);
}

void code_generator::visit(const syntax::CodeStatement& stmt) {}

void code_generator::visit(const syntax::ProcedureCallStatement& stmt)
{
    visit(stmt.getExpression());
}

llvm::Value* code_generator::visit(const syntax::Expression& expr)
{
    if (auto* e = dynamic_cast<const syntax::LiteralExpression*>(&expr)) return visit(*e);
    if (auto* e = dynamic_cast<const syntax::IdentifierExpression*>(&expr)) return visit(*e);
    if (auto* e = dynamic_cast<const syntax::BinaryExpression*>(&expr)) return visit(*e);
    if (auto* e = dynamic_cast<const syntax::UnaryExpression*>(&expr)) return visit(*e);
    if (auto* e = dynamic_cast<const syntax::FunctionCallExpression*>(&expr)) return visit(*e);
    if (auto* e = dynamic_cast<const syntax::PartWordExpression*>(&expr)) return visit(*e);
    if (auto* e = dynamic_cast<const syntax::ArrayAccessExpression*>(&expr)) return visit(*e);
    return nullptr;
}

llvm::Value* code_generator::visit(const syntax::LiteralExpression& expr)
{
    auto& val = expr.getLiteral()->getValue();
    if (std::holds_alternative<llvm::APSInt>(val))
    {
        return llvm::ConstantInt::get(m_module.getContext(), std::get<llvm::APSInt>(val));
    }
    if (std::holds_alternative<llvm::APFloat>(val))
    {
        return llvm::ConstantFP::get(m_module.getContext(), std::get<llvm::APFloat>(val));
    }
    if (std::holds_alternative<std::string>(val))
    {
        // Try parsing string if fallback
        try {
            return llvm::ConstantInt::get(m_module.getContext(), llvm::APInt(32, std::stoi(std::get<std::string>(val)), 10));
        } catch (...) {}
    }
    return nullptr;
}

llvm::Value* code_generator::visit(const syntax::IdentifierExpression& expr)
{
    auto it = m_program.getContext().resolvedIdentifiers.find(&expr);
    if (it != m_program.getContext().resolvedIdentifiers.end()) {
        const auto* decl = it->second;
        if (auto* procDecl = decl->tryAs<semantics::ProcedureDeclaration>()) {
            // It's a procedure call without arguments!
            llvm::Value* funcVal = m_globals.contains(decl) ? m_globals[decl] : nullptr;
            if (!funcVal) return nullptr;

            llvm::FunctionType* ft = nullptr;
            const auto* procType = procDecl->getType().tryAs<semantics::ProcedureType>();
            if (procType) {
                llvm::Type* retType = llvm::Type::getVoidTy(m_module.getContext());
                if (procType->getReturnType()) retType = convertType(*procType->getReturnType());
                std::vector<llvm::Type*> params;
                ft = llvm::FunctionType::get(retType, params, false);
            }
            if (ft) return m_builder.CreateCall(ft, funcVal, {});
            return nullptr;
        }
    }

    llvm::Value* addr = emitAddress(expr);
    if (!addr) return nullptr;
    if (llvm::isa<llvm::Function>(addr)) return addr;
    
    llvm::Type* type = nullptr;
    auto typeIt = m_program.getContext().expressionTypes.find(&expr);
    if (typeIt != m_program.getContext().expressionTypes.end()) {
        type = convertType(*typeIt->second);
    } else {
        type = llvm::Type::getInt32Ty(m_module.getContext());
    }
    return m_builder.CreateLoad(type, addr);
}

llvm::Value* code_generator::visit(const syntax::BinaryExpression& expr)
{
    llvm::Value* L = visit(expr.getLeft());
    llvm::Value* R = visit(expr.getRight());
    if (!L || !R) return nullptr;

    if (L->getType() != R->getType())
    {
        if (L->getType()->isIntegerTy() && R->getType()->isDoubleTy())
            L = m_builder.CreateSIToFP(L, R->getType());
        else if (L->getType()->isDoubleTy() && R->getType()->isIntegerTy())
            R = m_builder.CreateSIToFP(R, L->getType());
        else if (L->getType()->isIntegerTy() && R->getType()->isIntegerTy())
        {
            if (L->getType()->getIntegerBitWidth() > R->getType()->getIntegerBitWidth())
                R = m_builder.CreateIntCast(R, L->getType(), true);
            else
                L = m_builder.CreateIntCast(L, R->getType(), true);
        }
    }

    auto kind = expr.getOp()->getTokenType();
    if (kind == lexer::TokenType::Plus) return L->getType()->isFloatingPointTy() ? m_builder.CreateFAdd(L, R) : m_builder.CreateAdd(L, R);
    if (kind == lexer::TokenType::Minus) return L->getType()->isFloatingPointTy() ? m_builder.CreateFSub(L, R) : m_builder.CreateSub(L, R);
    if (kind == lexer::TokenType::Asterisk) return L->getType()->isFloatingPointTy() ? m_builder.CreateFMul(L, R) : m_builder.CreateMul(L, R);
    if (kind == lexer::TokenType::Division) return L->getType()->isFloatingPointTy() ? m_builder.CreateFDiv(L, R) : m_builder.CreateSDiv(L, R);
    if (kind == lexer::TokenType::Equal) return m_builder.CreateICmpEQ(L, R);
    if (kind == lexer::TokenType::NotEqual) return m_builder.CreateICmpNE(L, R);
    if (kind == lexer::TokenType::LessThan) return m_builder.CreateICmpSLT(L, R);
    if (kind == lexer::TokenType::LessThanOrEqual) return m_builder.CreateICmpSLE(L, R);
    if (kind == lexer::TokenType::GreaterThan) return m_builder.CreateICmpSGT(L, R);
    if (kind == lexer::TokenType::GreaterThanOrEqual) return m_builder.CreateICmpSGE(L, R);
    return nullptr;
}

llvm::Value* code_generator::visit(const syntax::UnaryExpression& expr)
{
    llvm::Value* operand = visit(expr.getOperand());
    if (!operand) return nullptr;

    auto kind = expr.getOp()->getTokenType();
    if (kind == lexer::TokenType::Minus) return m_builder.CreateNeg(operand);
    if (kind == lexer::TokenType::Plus) return operand;
    return nullptr;
}

llvm::Value* code_generator::visit(const syntax::FunctionCallExpression& expr)
{
    auto it = m_program.getContext().resolvedIdentifiers.find(&expr);
    if (it == m_program.getContext().resolvedIdentifiers.end()) return nullptr;

    const auto* decl = it->second;
    llvm::Value* funcVal = m_globals.contains(decl) ? m_globals[decl] : nullptr;
    if (!funcVal) return nullptr;

    std::vector<llvm::Value*> args;
    for (const auto& arg : expr.getArguments())
    {
        args.push_back(visit(*arg));
    }

    llvm::FunctionType* ft = nullptr;
    if (auto* procDecl = decl->tryAs<semantics::ProcedureDeclaration>()) {
        const auto* procType = procDecl->getType().tryAs<semantics::ProcedureType>();
        if (procType) {
            llvm::Type* retType = llvm::Type::getVoidTy(m_module.getContext());
            if (procType->getReturnType()) retType = convertType(*procType->getReturnType());
            std::vector<llvm::Type*> params;
            for (auto& p : procType->getParameters()) params.push_back(convertType(*p.type));
            ft = llvm::FunctionType::get(retType, params, false);
        }
    }

    if (ft) return m_builder.CreateCall(ft, funcVal, args);
    return nullptr;
}

llvm::Value* code_generator::visit(const syntax::PartWordExpression& expr) { return nullptr; }

llvm::Value* code_generator::visit(const syntax::ArrayAccessExpression& expr)
{
    llvm::Value* addr = emitAddress(expr);
    if (!addr) return nullptr;

    auto rit = m_program.getContext().resolvedIdentifiers.find(&expr);
    llvm::Type* type = llvm::Type::getInt32Ty(m_module.getContext());
    if (rit != m_program.getContext().resolvedIdentifiers.end()) {
        if (auto* arrDecl = rit->second->tryAs<semantics::ArrayDeclaration>()) {
            type = convertType(arrDecl->getType().tryAs<semantics::ArrayType>()->getElementType());
        }
    }
    return m_builder.CreateLoad(type, addr, "arrayload");
}

} // namespace coral66::CGLLVM
