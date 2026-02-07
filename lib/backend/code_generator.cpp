/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "code_generator.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Module.h>

namespace coral66::CGLLVM
{

code_generator::code_generator(llvm::Module& module, const Semantics::Program& program,
                               const source_interface& sourceInterface, const CGLLVM::Options& options)
    : m_module(module), m_program(program), m_sourceInterface(sourceInterface), m_options(options)
{
}

void code_generator::generate()
{
    // Process globals
    for (const auto& global : m_program.getGlobals())
    {
        global->match([&](const Semantics::VariableDeclaration& var) { visit(var); },
                      [&](const Semantics::ProcedureDeclaration& proc) { visit(proc); }, [](const auto&) {});
    }
}

llvm::Type* code_generator::convertType(const Semantics::Type& type)
{
    auto& ctx = m_module.getContext();
    if (auto* prim = type.tryAs<Semantics::PrimitiveType>())
    {
        switch (prim->getKind())
        {
            case Semantics::PrimitiveType::Integer: return llvm::Type::getInt32Ty(ctx);
            case Semantics::PrimitiveType::Floating: return llvm::Type::getFloatTy(ctx);
            case Semantics::PrimitiveType::Bits: return llvm::Type::getInt32Ty(ctx);
        }
    }
    else if (type.is<Semantics::VoidType>())
    {
        return llvm::Type::getVoidTy(ctx);
    }
    else if (auto* fixed = type.tryAs<Semantics::FixedType>())
    {
        if (fixed->getTotalBits() <= 16)
            return llvm::Type::getInt16Ty(ctx);
        if (fixed->getTotalBits() <= 32)
            return llvm::Type::getInt32Ty(ctx);
        return llvm::Type::getInt64Ty(ctx);
    }
    else if (auto* arr = type.tryAs<Semantics::ArrayType>())
    {
        llvm::Type* ret = convertType(arr->getElementType());
        for (auto it = arr->getDimensions().rbegin(); it != arr->getDimensions().rend(); ++it)
        {
            ret = llvm::ArrayType::get(ret, *it);
        }
        return ret;
    }
    else if (auto* table = type.tryAs<Semantics::TableType>())
    {
        if (auto* def = std::get_if<Semantics::TableDefinition>(&table->getInfo().type))
        {
            uint64_t totalWords = def->getWidth() * def->getLength();
            return llvm::ArrayType::get(llvm::Type::getInt32Ty(ctx), totalWords);
        }
        return llvm::Type::getInt32Ty(ctx);
    }
    return llvm::Type::getVoidTy(ctx);
}

llvm::BasicBlock* code_generator::getOrCreateLabel(const Semantics::LabelStatement* label)
{
    if (!m_labels.contains(label))
    {
        m_labels[label] = llvm::BasicBlock::Create(m_module.getContext(), "label", m_currentFunction);
    }
    return m_labels[label];
}

llvm::Value* code_generator::emitAddress(const Semantics::ExpressionBase& expr)
{
    if (auto* declRead = dynamic_cast<const Semantics::DeclarationRead*>(&expr))
    {
        const auto* decl = &declRead->getDeclRead();
        if (m_locals.contains(decl))
            return m_locals[decl];
        if (m_globals.contains(decl))
            return m_globals[decl];
    }
    else if (auto* part = dynamic_cast<const Semantics::PartWordAccess*>(&expr))
    {
        return emitAddress(part->getBase());
    }
    return nullptr;
}

void code_generator::visit(const Semantics::Declaration& decl)
{
    if (auto* var = dynamic_cast<const Semantics::VariableDeclaration*>(&decl))
    {
        auto* llvmType = convertType(var->getType());
        auto* globalVar = new llvm::GlobalVariable(m_module, llvmType, false, llvm::GlobalValue::ExternalLinkage,
                                                   llvm::Constant::getNullValue(llvmType), var->getName());
        m_globals[var] = globalVar;
    }
    else if (auto* proc = dynamic_cast<const Semantics::ProcedureDeclaration*>(&decl))
    {
        generateProcedure(*proc);
    }
}

void code_generator::generateProcedure(const Semantics::ProcedureDeclaration& procDecl)
{
    m_currentProcedure = &procDecl;
    m_labels.clear();
    m_locals.clear();

    const auto* procType = procDecl.getType().tryAs<Semantics::ProcedureType>();
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
    llvm::Function* f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, procDecl.getName(), m_module);
    m_globals[&procDecl] = f;
    m_currentFunction = f;

    llvm::BasicBlock* bb = llvm::BasicBlock::Create(m_module.getContext(), "entry", f);
    m_builder.SetInsertPoint(bb);

    for (const auto& stmt : procDecl.getBody())
    {
        visit(*stmt);
    }

    if (!m_builder.GetInsertBlock()->getTerminator())
    {
        if (m_currentFunction->getReturnType()->isVoidTy())
            m_builder.CreateRetVoid();
        else
            m_builder.CreateRet(llvm::Constant::getNullValue(m_currentFunction->getReturnType()));
    }
}

void code_generator::visit(const Semantics::Statement& stmt)
{
    stmt.match([&](const Semantics::AssignmentStatement& s) { visit(s); },
               [&](const Semantics::IfStatement& s) { visit(s); }, [&](const Semantics::GotoStatement& s) { visit(s); },
               [&](const Semantics::ForStatement& s) { visit(s); }, [&](const Semantics::AnswerStatement& s)
               { visit(s); }, [&](const Semantics::CodeStatement& s) { visit(s); },
               [&](const Semantics::ProcedureCallStatement& s) { visit(s); },
               [&](const Semantics::LabelStatement& s) { visit(s); }, [](const auto&) {});
}

void code_generator::visit(const Semantics::AssignmentStatement& stmt)
{
    const auto& target = stmt.getTarget();
    llvm::Value* rhs = visit(stmt.getExpression());
    if (!rhs)
        return;

    // Cast RHS to target type
    // Assuming type compatibility is checked in Semantics
    // But for Fixed/Integer conversion we might need explicit casts?
    // Let's do a safe bitcast or intcast if sizes differ (e.g. i64 constant to i32 var)
    // Actually, store instruction requires matching types.
    // Fixed(16,8) is i16. Constant 1.5 is double.
    // If we have double constant and integer target, we must cast.
    // SemanticAnalysis currently just passes Constant.
    // We should handle type conversion here for robustness.

    // Check target type
    llvm::Type* targetType = nullptr;
    llvm::Value* targetAddr = nullptr;

    if (auto* part = dynamic_cast<const Semantics::PartWordAccess*>(&target))
    {
        targetAddr = emitAddress(*part);
        targetType = convertType(part->getBase().getType());
    }
    else
    {
        targetAddr = emitAddress(target);
        if (targetAddr)
            targetType = convertType(target.getType());
    }

    if (rhs->getType() != targetType && targetType)
    {
        if (rhs->getType()->isDoubleTy() && targetType->isIntegerTy())
        {
            // Float to Fixed/Int conversion
            // Fixed 1.5 -> 1.5 * 2^frac
            // Need fractional bits.
            // If target is FixedType...
            if (auto* fixed = target.getType().tryAs<Semantics::FixedType>())
            {
                double scale = std::pow(2.0, fixed->getFractionalBits());
                llvm::Value* scaleVal = llvm::ConstantFP::get(rhs->getType(), scale);
                rhs = m_builder.CreateFMul(rhs, scaleVal);
                rhs = m_builder.CreateFPToSI(rhs, targetType);
            }
            else
            {
                rhs = m_builder.CreateFPToSI(rhs, targetType);
            }
        }
        else if (rhs->getType()->isIntegerTy() && targetType->isIntegerTy())
        {
            rhs = m_builder.CreateIntCast(rhs, targetType, true);
        }
    }

    if (auto* part = dynamic_cast<const Semantics::PartWordAccess*>(&target))
    {
        // Read-Modify-Write
        if (!targetAddr)
            return;

        llvm::Value* baseVal = m_builder.CreateLoad(targetType, targetAddr);

        llvm::Value* startVal = llvm::ConstantInt::get(targetType, part->getStartBit());
        llvm::Value* widthVal = llvm::ConstantInt::get(targetType, part->getWidth());

        llvm::Value* one = llvm::ConstantInt::get(targetType, 1);
        llvm::Value* mask = m_builder.CreateSub(m_builder.CreateShl(one, widthVal), one, "mask");

        llvm::Value* shiftedMask = m_builder.CreateShl(mask, startVal, "shiftedMask");
        llvm::Value* clearMask = m_builder.CreateNot(shiftedMask, "clearMask");

        llvm::Value* clearedBase = m_builder.CreateAnd(baseVal, clearMask, "clearedBase");

        llvm::Value* castRhs = m_builder.CreateIntCast(rhs, targetType, false);
        llvm::Value* maskedRhs = m_builder.CreateAnd(castRhs, mask, "maskedRhs");
        llvm::Value* shiftedRhs = m_builder.CreateShl(maskedRhs, startVal, "shiftedRhs");

        llvm::Value* newBase = m_builder.CreateOr(clearedBase, shiftedRhs, "newBase");

        m_builder.CreateStore(newBase, targetAddr);
    }
    else
    {
        if (targetAddr)
        {
            m_builder.CreateStore(rhs, targetAddr);
        }
    }
}

void code_generator::visit(const Semantics::IfStatement& stmt)
{
    llvm::Value* cond = visit(stmt.getCondition());
    if (!cond)
        return;

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

void code_generator::visit(const Semantics::GotoStatement& stmt)
{
    if (stmt.getLabel())
    {
        llvm::BasicBlock* targetBB = getOrCreateLabel(stmt.getLabel());
        m_builder.CreateBr(targetBB);
    }
}

void code_generator::visit(const Semantics::LabelStatement& stmt)
{
    llvm::BasicBlock* bb = getOrCreateLabel(&stmt);
    if (!bb->getParent())
    {
        m_currentFunction->insert(m_currentFunction->end(), bb);
    }

    if (auto* curBB = m_builder.GetInsertBlock())
    {
        if (!curBB->getTerminator())
        {
            m_builder.CreateBr(bb);
        }
    }

    m_builder.SetInsertPoint(bb);
    if (stmt.getTarget())
    {
        visit(*stmt.getTarget());
    }
}

void code_generator::visit(const Semantics::ForStatement& stmt)
{
    llvm::Value* startVal = visit(stmt.getStart());
    llvm::Value* loopVarAddr = emitAddress(stmt.getVariable());

    if (loopVarAddr && startVal)
        m_builder.CreateStore(startVal, loopVarAddr);

    llvm::BasicBlock* condBB = llvm::BasicBlock::Create(m_module.getContext(), "for.cond", m_currentFunction);
    llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(m_module.getContext(), "for.body", m_currentFunction);
    llvm::BasicBlock* stepBB = llvm::BasicBlock::Create(m_module.getContext(), "for.step", m_currentFunction);
    llvm::BasicBlock* endBB = llvm::BasicBlock::Create(m_module.getContext(), "for.end", m_currentFunction);

    m_builder.CreateBr(condBB);

    m_builder.SetInsertPoint(condBB);
    llvm::Value* currVal = m_builder.CreateLoad(convertType(stmt.getVariable().getType()), loopVarAddr);
    llvm::Value* untilVal = visit(stmt.getUntil());
    llvm::Value* cond = m_builder.CreateICmpSLT(currVal, untilVal);
    m_builder.CreateCondBr(cond, bodyBB, endBB);

    m_builder.SetInsertPoint(bodyBB);
    visit(stmt.getBody());
    m_builder.CreateBr(stepBB);

    m_builder.SetInsertPoint(stepBB);
    llvm::Value* stepVal = visit(stmt.getStep());
    llvm::Value* valForStep = m_builder.CreateLoad(convertType(stmt.getVariable().getType()), loopVarAddr);
    llvm::Value* incVal = m_builder.CreateAdd(valForStep, stepVal);
    m_builder.CreateStore(incVal, loopVarAddr);
    m_builder.CreateBr(condBB);

    m_builder.SetInsertPoint(endBB);
}

void code_generator::visit(const Semantics::AnswerStatement& stmt)
{
    llvm::Value* val = visit(stmt.getExpression());
    if (val && m_currentFunction->getReturnType()->isIntegerTy())
    {
        val = m_builder.CreateIntCast(val, m_currentFunction->getReturnType(), true);
    }
    m_builder.CreateRet(val);
}

void code_generator::visit(const Semantics::CodeStatement& stmt)
{
    llvm::FunctionType* ft = llvm::FunctionType::get(llvm::Type::getVoidTy(m_module.getContext()), {}, false);
    llvm::InlineAsm* ia = llvm::InlineAsm::get(ft, stmt.getCode(), "", true);
    m_builder.CreateCall(ft, ia, {});
}

void code_generator::visit(const Semantics::ProcedureCallStatement& stmt)
{
    visit(stmt.getCallExpr());
}

llvm::Value* code_generator::visit(const Semantics::ExpressionBase& expr)
{
    return expr.match([&](const Semantics::Constant& e) { return visit(e); }, [&](const Semantics::DeclarationRead& e)
                      { return visit(e); }, [&](const Semantics::BinaryOperator& e) { return visit(e); },
                      [&](const Semantics::UnaryOperator& e) { return visit(e); },
                      [&](const Semantics::CallExpression& e) { return visit(e); },
                      [&](const Semantics::PartWordAccess& e) { return visit(e); },
                      [&](const Semantics::ErrorExpression& e) { return visit(e); });
}

// ReSharper disable once CppMemberFunctionMayBeConst
llvm::Value* code_generator::visit(const Semantics::Constant& expr)
{
    auto& val = expr.getValue();
    if (std::holds_alternative<llvm::APSInt>(val))
    {
        return llvm::ConstantInt::get(m_module.getContext(), std::get<llvm::APSInt>(val));
    }
    if (std::holds_alternative<llvm::APFloat>(val))
    {
        return llvm::ConstantFP::get(m_module.getContext(), std::get<llvm::APFloat>(val));
    }
    return nullptr;
}

llvm::Value* code_generator::visit(const Semantics::DeclarationRead& expr)
{
    llvm::Value* addr = emitAddress(expr);
    if (!addr)
        return nullptr;
    if (llvm::isa<llvm::Function>(addr))
        return addr;
    return m_builder.CreateLoad(convertType(expr.getType()), addr);
}

llvm::Value* code_generator::visit(const Semantics::BinaryOperator& expr)
{
    llvm::Value* L = visit(expr.getLeftExpression());
    llvm::Value* R = visit(expr.getRightExpression());
    if (!L || !R)
        return nullptr;

    // Ensure types match before op
    if (L->getType() != R->getType())
    {
        // Simple promotion int -> float, etc. Stub.
        if (L->getType()->isIntegerTy() && R->getType()->isDoubleTy())
            L = m_builder.CreateSIToFP(L, R->getType());
        else if (L->getType()->isDoubleTy() && R->getType()->isIntegerTy())
            R = m_builder.CreateSIToFP(R, L->getType());
    }

    switch (expr.getKind())
    {
        case Semantics::BinaryOperator::Addition:
            if (L->getType()->isFloatingPointTy())
                return m_builder.CreateFAdd(L, R);
            return m_builder.CreateAdd(L, R);
        case Semantics::BinaryOperator::Subtraction:
            if (L->getType()->isFloatingPointTy())
                return m_builder.CreateFSub(L, R);
            return m_builder.CreateSub(L, R);
        case Semantics::BinaryOperator::Multiply:
        {
            llvm::Value* mul = nullptr;
            if (L->getType()->isFloatingPointTy())
                mul = m_builder.CreateFMul(L, R);
            else
                mul = m_builder.CreateMul(L, R);

            if (auto* fixed = expr.getType().tryAs<Semantics::FixedType>())
            {
                // Fixed Point Scaling: val = (a * b) >> frac
                if (fixed->getFractionalBits() > 0)
                    return m_builder.CreateAShr(mul, fixed->getFractionalBits());
            }
            return mul;
        }
        case Semantics::BinaryOperator::Divide:
            if (L->getType()->isFloatingPointTy())
                return m_builder.CreateFDiv(L, R);
            return m_builder.CreateSDiv(L, R);
        case Semantics::BinaryOperator::Equal: return m_builder.CreateICmpEQ(L, R);
        case Semantics::BinaryOperator::NotEqual: return m_builder.CreateICmpNE(L, R);
        case Semantics::BinaryOperator::LessThan: return m_builder.CreateICmpSLT(L, R);
        default: return nullptr;
    }
}

llvm::Value* code_generator::visit(const Semantics::UnaryOperator& expr)
{
    llvm::Value* operand = visit(expr.getOperand());
    if (!operand)
        return nullptr;

    switch (expr.getKind())
    {
        case Semantics::UnaryOperator::Minus: return m_builder.CreateNeg(operand);
        case Semantics::UnaryOperator::Not: return m_builder.CreateNot(operand);
        case Semantics::UnaryOperator::Plus: return operand;
    }
    return nullptr;
}

llvm::Value* code_generator::visit(const Semantics::CallExpression& expr)
{
    llvm::Value* funcVal = visit(expr.getFunctionExpression());

    if (!funcVal)
        return nullptr;

    std::vector<llvm::Value*> args;
    for (const auto& arg : expr.getArguments())
    {
        args.push_back(visit(*arg));
    }

    const auto* procType = expr.getFunctionExpression().getType().tryAs<Semantics::ProcedureType>();
    llvm::FunctionType* ft = nullptr;
    if (procType)
    {
        llvm::Type* retType = llvm::Type::getVoidTy(m_module.getContext());
        if (procType->getReturnType())
            retType = convertType(*procType->getReturnType());
        std::vector<llvm::Type*> params;
        for (auto& p : procType->getParameters())
            params.push_back(convertType(*p.type));
        ft = llvm::FunctionType::get(retType, params, false);
    }
    else
    {
        // Try to deduce from pointer?
        if (funcVal->getType()->isPointerTy())
        {
            // Assume opaque pointer, can't deduce. Fail or use stub?
            // Since we have semantic info, we should have ProcedureType.
            // If not, we can't generate Call.
        }
    }

    if (ft)
    {
        return m_builder.CreateCall(ft, funcVal, args);
    }
    return nullptr;
}

llvm::Value* code_generator::visit(const Semantics::PartWordAccess& expr)
{
    // Read: (val >> start) & ((1 << width) - 1)
    llvm::Value* baseVal = visit(expr.getBase());
    if (!baseVal)
        return nullptr;

    llvm::Type* type = baseVal->getType();
    llvm::Value* startVal = llvm::ConstantInt::get(type, expr.getStartBit());
    llvm::Value* widthVal = llvm::ConstantInt::get(type, expr.getWidth());

    llvm::Value* shifted = m_builder.CreateLShr(baseVal, startVal, "shifted");
    llvm::Value* one = llvm::ConstantInt::get(type, 1);
    llvm::Value* mask = m_builder.CreateSub(m_builder.CreateShl(one, widthVal), one, "mask");

    return m_builder.CreateAnd(shifted, mask, "partWord");
}

llvm::Value* code_generator::visit(const Semantics::ErrorExpression&)
{
    return nullptr;
}

} // namespace coral66::CGLLVM
