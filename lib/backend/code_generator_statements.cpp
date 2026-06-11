/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "code_generator.hpp"

#include <llvm/IR/InlineAsm.h>

#include "lib/frontend/semantics/program.hpp"

namespace coral66::cgllvm
{

llvm::BasicBlock* code_generator::getOrCreateLabel(const syntax::Statement* statement)
{
    if (!m_labels.contains(statement))
    {
        llvm::BasicBlock* bb = llvm::BasicBlock::Create(m_module.getContext(), "label");
        m_labels[statement] = bb;
        for (const auto& labelToken : statement->getLabels())
        {
            const auto& labelName = std::get<std::string>(labelToken->getValue());
            m_namedLabels[labelName] = bb;
        }
    }
    return m_labels[statement];
}

void code_generator::preScanLabels(const syntax::Statement& statement)
{
    if (!statement.getLabels().empty())
        getOrCreateLabel(&statement);

    if (auto* compStmt = dynamic_cast<const syntax::CompoundStatement*>(&statement))
    {
        for (const auto& innerStmt : compStmt->getStatements())
            preScanLabels(*innerStmt);
    }
    else if (auto* ifStmt = dynamic_cast<const syntax::IfStatement*>(&statement))
    {
        preScanLabels(ifStmt->getThenBranch());
        if (ifStmt->getElseBranch())
            preScanLabels(*ifStmt->getElseBranch());
    }
    else if (auto* forStmt = dynamic_cast<const syntax::ForStatement*>(&statement))
    {
        preScanLabels(forStmt->getBody());
    }
    else if (auto* blockStmt = dynamic_cast<const syntax::BlockStatement*>(&statement))
    {
        if (blockStmt->hasBlock())
        {
            for (const auto& innerStmt : blockStmt->getBlock().getStatements())
                preScanLabels(*innerStmt);
        }
    }
}

void code_generator::visit(const syntax::Statement& statement)
{
    if (!statement.getLabels().empty())
    {
        llvm::BasicBlock* stmtBB = getOrCreateLabel(&statement);
        if (!m_builder.GetInsertBlock()->getTerminator())
        {
            m_builder.CreateBr(stmtBB);
        }
        m_currentFunction->insert(m_currentFunction->end(), stmtBB);
        m_builder.SetInsertPoint(stmtBB);
    }

    if (auto* s = dynamic_cast<const syntax::AssignmentStatement*>(&statement))
        visit(*s);
    else if (auto* s1 = dynamic_cast<const syntax::IfStatement*>(&statement))
        visit(*s1);
    else if (auto* s2 = dynamic_cast<const syntax::GotoStatement*>(&statement))
        visit(*s2);
    else if (auto* s3 = dynamic_cast<const syntax::ForStatement*>(&statement))
        visit(*s3);
    else if (auto* s4 = dynamic_cast<const syntax::AnswerStatement*>(&statement))
        visit(*s4);
    else if (auto* s5 = dynamic_cast<const syntax::CodeStatement*>(&statement))
        visit(*s5);
    else if (auto* s6 = dynamic_cast<const syntax::ProcedureCallStatement*>(&statement))
        visit(*s6);
    else if (auto* s7 = dynamic_cast<const syntax::DummyStatement*>(&statement))
        visit(*s7);
    else if (auto* s8 = dynamic_cast<const syntax::BlockStatement*>(&statement))
        visit(*s8);
    else if (auto* s9 = dynamic_cast<const syntax::CompoundStatement*>(&statement))
        visit(*s9);
}

void code_generator::visit(const syntax::DummyStatement& statement) {}

void code_generator::visit(const syntax::BlockStatement& statement)
{
    if (statement.hasBlock())
        visit(statement.getBlock());
}

void code_generator::visit(const syntax::CompoundStatement& statement)
{
    for (const auto& s : statement.getStatements())
        visit(*s);
}

void code_generator::visit(const syntax::AssignmentStatement& statement)
{
    llvm::Value* targetAddr = emitAddress(statement.getVariable());
    llvm::Value* rhs = visit(statement.getExpression());
    if (!targetAddr || !rhs)
        return;

    const auto it = m_program.getContext().expressionTypes.find(&statement.getVariable());
    llvm::Type* targetType = nullptr;
    if (it != m_program.getContext().expressionTypes.end())
    {
        targetType = convertType(*it->second);
        if (targetType->isPointerTy() && it->second->isLocation())
        {
            if (auto* prim = it->second->tryAs<semantics::PrimitiveType>())
                targetType = convertType(semantics::PrimitiveType(prim->getKind()));
        }
    }
    else
    {
        // Fallback for arrays
        if (auto* arrAccess = dynamic_cast<const syntax::ArrayAccessExpression*>(&statement.getVariable()))
        {
            if (const auto rit = m_program.getContext().resolvedIdentifiers.find(arrAccess);
                rit != m_program.getContext().resolvedIdentifiers.end())
            {
                if (auto* arrDeclaration = rit->second->tryAs<semantics::ArrayDeclaration>())
                {
                    targetType = convertType(arrDeclaration->getType().tryAs<semantics::ArrayType>()->getElementType());
                }
            }
        }
    }

    if (rhs->getType() != targetType && targetType)
    {
        if (rhs->getType()->isFloatingPointTy() && targetType->isIntegerTy())
            rhs = m_builder.CreateFPToSI(rhs, targetType);
        else if (rhs->getType()->isIntegerTy() && targetType->isFloatingPointTy())
            rhs = m_builder.CreateSIToFP(rhs, targetType);
        else if (rhs->getType()->isFloatingPointTy() && targetType->isFloatingPointTy())
            rhs = m_builder.CreateFPCast(rhs, targetType);
        else if (rhs->getType()->isIntegerTy() && targetType->isIntegerTy())
            rhs = m_builder.CreateIntCast(rhs, targetType, true);
    }

    m_builder.CreateStore(rhs, targetAddr);
}

void code_generator::visit(const syntax::IfStatement& statement)
{
    llvm::Value* cond = visit(statement.getCondition());
    if (!cond)
        return;

    if (cond->getType()->isIntegerTy() && cond->getType()->getIntegerBitWidth() > 1)
        cond = m_builder.CreateICmpNE(cond, llvm::ConstantInt::get(cond->getType(), 0));

    llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(m_module.getContext(), "then", m_currentFunction);
    llvm::BasicBlock* elseBB = nullptr;
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(m_module.getContext(), "ifcont");

    const bool hasElse = statement.getElseBranch() != nullptr;
    if (hasElse)
        elseBB = llvm::BasicBlock::Create(m_module.getContext(), "else");

    m_builder.CreateCondBr(cond, thenBB, hasElse ? elseBB : mergeBB);

    m_builder.SetInsertPoint(thenBB);
    visit(statement.getThenBranch());
    if (!m_builder.GetInsertBlock()->getTerminator())
        m_builder.CreateBr(mergeBB);

    if (hasElse)
    {
        m_currentFunction->insert(m_currentFunction->end(), elseBB);
        m_builder.SetInsertPoint(elseBB);
        visit(*statement.getElseBranch());
        if (!m_builder.GetInsertBlock()->getTerminator())
            m_builder.CreateBr(mergeBB);
    }

    m_currentFunction->insert(m_currentFunction->end(), mergeBB);
    m_builder.SetInsertPoint(mergeBB);
}

void code_generator::visit(const syntax::GotoStatement& statement)
{
    const auto& labelToken = statement.getLabel();
    const auto& labelName = std::get<std::string>(labelToken->getValue());

    if (const auto it = m_program.getContext().resolvedIdentifiers.find(&statement);
        it != m_program.getContext().resolvedIdentifiers.end())
    {
        // If it's a simple label:
        if (const auto* declaration = it->second; declaration->tryAs<semantics::LabelDeclaration>())
        {
            if (m_namedLabels.contains(labelName))
            {
                llvm::BasicBlock* targetBB = m_namedLabels[labelName];
                m_builder.CreateBr(targetBB);

                // Set insertion point to a new unreachable block to handle dead code after GOTO
                llvm::BasicBlock* deadBB = llvm::BasicBlock::Create(m_module.getContext(), "dead", m_currentFunction);
                m_builder.SetInsertPoint(deadBB);
            }
        }
        else if (auto* switchDeclaration = declaration->tryAs<semantics::SwitchDeclaration>())
        {
            if (auto* indexExpr = statement.getIndex())
            {
                llvm::Value* indexVal = visit(*indexExpr);
                if (!indexVal)
                    return;

                // Ensure index is an integer type (i32)
                if (!indexVal->getType()->isIntegerTy(32))
                {
                    indexVal = m_builder.CreateIntCast(indexVal, llvm::Type::getInt32Ty(m_module.getContext()), true);
                }

                // Coral 66 switch indices are 1-based by default
                indexVal = m_builder.CreateSub(
                    indexVal, llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_module.getContext()), 1));

                // Create default block (fallthrough or unreachable)
                llvm::BasicBlock* defaultBB =
                    llvm::BasicBlock::Create(m_module.getContext(), "switch.default", m_currentFunction);

                auto& switchLabels = switchDeclaration->getLabels();
                llvm::SwitchInst* switchInst = m_builder.CreateSwitch(indexVal, defaultBB, switchLabels.size());

                for (size_t i = 0; i < switchLabels.size(); ++i)
                {
                    if (switchLabels[i])
                    {
                        const auto& targetLabelNameToken = switchLabels[i]->getName();

                        if (const auto& targetLabelName = std::get<std::string>(targetLabelNameToken->getValue());
                            m_namedLabels.contains(targetLabelName))
                        {
                            llvm::BasicBlock* targetBB = m_namedLabels[targetLabelName];
                            switchInst->addCase(
                                llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_module.getContext()), i), targetBB);
                        }
                    }
                }

                m_builder.SetInsertPoint(defaultBB);
                // What to do if index is out of bounds? We'll just continue execution.
            }
        }
    }
}

void code_generator::visit(const syntax::ForStatement& statement)
{
    llvm::Value* loopVarAddr = emitAddress(statement.getVariable());
    llvm::Type* loopVarType = nullptr;

    if (const auto it = m_program.getContext().expressionTypes.find(&statement.getVariable());
        it != m_program.getContext().expressionTypes.end())
    {
        loopVarType = convertType(*it->second);
    }
    else
    {
        loopVarType = llvm::Type::getInt32Ty(m_module.getContext()); // default
    }

    llvm::BasicBlock* endBB = llvm::BasicBlock::Create(m_module.getContext(), "for.end", m_currentFunction);

    for (const auto& [expression, step, until, whileCondition] : statement.getElements())
    {
        llvm::BasicBlock* nextElBB = llvm::BasicBlock::Create(m_module.getContext(), "for.next_el", m_currentFunction);

        if (step && until)
        {
            llvm::Value* v1 = visit(*expression);
            llvm::Value* v2 = visit(*step);
            llvm::Value* v3 = visit(*until);

            if (v1->getType() != loopVarType)
                v1 = m_builder.CreateIntCast(v1, loopVarType, true);
            if (v2->getType() != loopVarType)
                v2 = m_builder.CreateIntCast(v2, loopVarType, true);
            if (v3->getType() != loopVarType)
                v3 = m_builder.CreateIntCast(v3, loopVarType, true);

            llvm::AllocaInst* v1Addr = m_builder.CreateAlloca(loopVarType, nullptr, "v1");
            llvm::AllocaInst* v2Addr = m_builder.CreateAlloca(loopVarType, nullptr, "v2");
            llvm::AllocaInst* v3Addr = m_builder.CreateAlloca(loopVarType, nullptr, "v3");

            m_builder.CreateStore(v1, v1Addr);
            m_builder.CreateStore(v2, v2Addr);
            m_builder.CreateStore(v3, v3Addr);

            llvm::BasicBlock* condBB =
                llvm::BasicBlock::Create(m_module.getContext(), "for.step.cond", m_currentFunction);
            llvm::BasicBlock* bodyBB =
                llvm::BasicBlock::Create(m_module.getContext(), "for.step.body", m_currentFunction);
            llvm::BasicBlock* incBB =
                llvm::BasicBlock::Create(m_module.getContext(), "for.step.inc", m_currentFunction);

            m_builder.CreateBr(condBB);
            m_builder.SetInsertPoint(condBB);

            llvm::Value* currV1 = m_builder.CreateLoad(loopVarType, v1Addr);
            if (loopVarAddr)
                m_builder.CreateStore(currV1, loopVarAddr);

            llvm::Value* currV2 = m_builder.CreateLoad(loopVarType, v2Addr);
            llvm::Value* currV3 = m_builder.CreateLoad(loopVarType, v3Addr);

            llvm::Value* diff = m_builder.CreateSub(currV1, currV3);
            llvm::Value* prod = m_builder.CreateMul(diff, currV2);
            llvm::Value* isExhausted = m_builder.CreateICmpSGT(prod, llvm::ConstantInt::get(loopVarType, 0));
            m_builder.CreateCondBr(isExhausted, nextElBB, bodyBB);

            m_builder.SetInsertPoint(bodyBB);
            visit(statement.getBody());
            m_builder.CreateBr(incBB);

            m_builder.SetInsertPoint(incBB);
            llvm::Value* cv = m_builder.CreateLoad(loopVarType, loopVarAddr);
            llvm::Value* nextV1 = m_builder.CreateAdd(cv, currV2);
            m_builder.CreateStore(nextV1, v1Addr);
            m_builder.CreateBr(condBB);
        }
        else if (whileCondition)
        {
            llvm::BasicBlock* condBB =
                llvm::BasicBlock::Create(m_module.getContext(), "for.while.cond", m_currentFunction);
            llvm::BasicBlock* bodyBB =
                llvm::BasicBlock::Create(m_module.getContext(), "for.while.body", m_currentFunction);

            m_builder.CreateBr(condBB);
            m_builder.SetInsertPoint(condBB);

            llvm::Value* exprVal = visit(*expression);
            if (exprVal->getType() != loopVarType)
                exprVal = m_builder.CreateIntCast(exprVal, loopVarType, true);
            if (loopVarAddr)
                m_builder.CreateStore(exprVal, loopVarAddr);

            llvm::Value* condVal = visit(*whileCondition);
            if (condVal->getType() != m_builder.getInt1Ty())
                condVal = m_builder.CreateICmpNE(condVal, llvm::ConstantInt::get(condVal->getType(), 0));

            m_builder.CreateCondBr(condVal, bodyBB, nextElBB);

            m_builder.SetInsertPoint(bodyBB);
            visit(statement.getBody());
            m_builder.CreateBr(condBB);
        }
        else
        {
            llvm::Value* exprVal = visit(*expression);
            if (exprVal->getType() != loopVarType)
                exprVal = m_builder.CreateIntCast(exprVal, loopVarType, true);
            if (loopVarAddr)
                m_builder.CreateStore(exprVal, loopVarAddr);

            visit(statement.getBody());
            m_builder.CreateBr(nextElBB);
        }

        m_builder.SetInsertPoint(nextElBB);
    }

    m_builder.CreateBr(endBB);
    m_builder.SetInsertPoint(endBB);
}

void code_generator::visit(const syntax::AnswerStatement& statement)
{
    llvm::Value* val = visit(statement.getExpression());
    if (val && m_currentFunction->getReturnType()->isIntegerTy())
    {
        val = m_builder.CreateIntCast(val, m_currentFunction->getReturnType(), true);
    }
    m_builder.CreateRet(val);
}

void code_generator::visit(const syntax::CodeStatement& statement)
{
    const auto it = m_program.getContext().parsedCodes.find(&statement);
    if (it == m_program.getContext().parsedCodes.end())
        return;

    const auto& [llvmCode, inputs, outputs] = it->second;

    std::vector<llvm::Value*> inputArgs;
    std::string constraints;
    std::vector<llvm::Type*> outputTypes;
    std::vector<llvm::Value*> outputAddress;

    // For outputs
    for (const auto* output : outputs)
    {
        llvm::Value* addr = nullptr;
        if (m_locals.contains(output))
            addr = m_locals[output];
        else if (m_globals.contains(output))
            addr = m_globals[output];

        if (addr)
        {
            outputAddress.push_back(addr);
            llvm::Type* outType = nullptr;
            if (auto* var = output->tryAs<semantics::VariableDeclaration>())
                outType = convertType(var->getType());
            else
                outType = llvm::Type::getInt32Ty(m_module.getContext());
            outputTypes.push_back(outType);

            if (!constraints.empty())
                constraints += ",";
            constraints += "=r"; // Output constraint
        }
    }

    // For inputs
    for (const auto* input : inputs)
    {
        llvm::Value* addr = nullptr;
        if (m_locals.contains(input))
            addr = m_locals[input];
        else if (m_globals.contains(input))
            addr = m_globals[input];

        if (addr)
        {
            llvm::Type* type = nullptr;
            if (auto* var = input->tryAs<semantics::VariableDeclaration>())
                type = convertType(var->getType());
            else
                type = llvm::Type::getInt32Ty(m_module.getContext()); // default

            llvm::Value* val = m_builder.CreateLoad(type, addr);
            inputArgs.push_back(val);
            if (!constraints.empty())
                constraints += ",";
            constraints += "r"; // Input constraint
        }
    }

    // Include clobbers for safety as per specification recommendation
    if (!constraints.empty())
        constraints += ",";
    constraints += "~{memory},~{dirflag},~{fpsr},~{flags},~{eax},~{ebx},~{ecx},~{edx},~{r10}";

    std::vector<llvm::Type*> argTypes;
    for (const auto* argument : inputArgs)
        argTypes.push_back(argument->getType());

    llvm::Type* retType = llvm::Type::getVoidTy(m_module.getContext());
    if (outputTypes.size() == 1)
    {
        retType = outputTypes[0];
    }
    else if (outputTypes.size() > 1)
    {
        retType = llvm::StructType::get(m_module.getContext(), outputTypes);
    }

    llvm::FunctionType* aStatement = llvm::FunctionType::get(retType, argTypes, false);
    llvm::InlineAsm* ia = llvm::InlineAsm::get(aStatement, llvmCode, constraints, true);

    llvm::CallInst* call = m_builder.CreateCall(ia, inputArgs);

    // Store outputs
    if (outputTypes.size() == 1)
    {
        m_builder.CreateStore(call, outputAddress[0]);
    }
    else if (outputTypes.size() > 1)
    {
        for (size_t i = 0; i < outputTypes.size(); ++i)
        {
            llvm::Value* ext = m_builder.CreateExtractValue(call, i);
            m_builder.CreateStore(ext, outputAddress[i]);
        }
    }
}

void code_generator::visit(const syntax::ProcedureCallStatement& statement)
{
    visit(statement.getExpression());
}

} // namespace coral66::cgllvm
