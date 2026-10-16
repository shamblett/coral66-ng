/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "code_generator.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/InlineAsm.h>

#include <ranges>

#include "lib/frontend/semantics/control_flow_pass.hpp"
#include "lib/frontend/semantics/program.hpp"

namespace coral66::cgllvm
{

llvm::BasicBlock* CodeGenerator::getOrCreateLabel(const syntax::Statement* statement)
{
    if (!m_labels.contains(statement))
    {
        llvm::BasicBlock* bb = llvm::BasicBlock::Create(m_module.getContext(), "label", m_currentFunction);
        m_labels[statement] = bb;
        for (const auto& labelToken : statement->getLabels())
        {
            const auto& labelName = std::get<std::string>(labelToken->getValue());
            m_namedLabels[labelName] = bb;
        }
    }
    return m_labels[statement];
}

void CodeGenerator::preScanLabels(const syntax::Statement& statement)
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

void CodeGenerator::visit(const syntax::Statement& statement)
{
    if (!statement.getLabels().empty())
    {
        llvm::BasicBlock* stmtBB = getOrCreateLabel(&statement);
        if (!m_builder.GetInsertBlock()->getTerminator())
        {
            m_builder.CreateBr(stmtBB);
        }
        // DO NOT manually insert into m_currentFunction here, as getOrCreateLabel
        // already attached it to the function. Doing so corrupts the LLVM ilist!
        m_builder.SetInsertPoint(stmtBB);
    }
    else if (m_builder.GetInsertBlock() && m_builder.GetInsertBlock()->getTerminator())
    {
        // Dead code: current block is terminated and this statement has no label to branch to.
        return;
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
    else if (auto* s8 = dynamic_cast<const syntax::BlockStatement*>(&statement))
        visit(*s8);
    else if (auto* s9 = dynamic_cast<const syntax::CompoundStatement*>(&statement))
        visit(*s9);
}

void CodeGenerator::visit(const syntax::BlockStatement& statement)
{
    if (statement.hasBlock())
        visit(statement.getBlock());
}

void CodeGenerator::visit(const syntax::CompoundStatement& statement)
{
    for (const auto& s : statement.getStatements())
        visit(*s);
}

void CodeGenerator::visit(const syntax::AssignmentStatement& statement)
{
    llvm::Value* targetAddr = emitAddress(statement.getVariable());
    llvm::Value* rhs = visit(statement.getExpression());
    if (!targetAddr || !rhs)
        return;

    bool isVolatile = false;
    if (m_options.baremetal)
    {
        if (const auto declIt = m_program.getContext().resolvedIdentifiers.find(&statement.getVariable());
            declIt != m_program.getContext().resolvedIdentifiers.end())
        {
            if (auto* var = declIt->second->tryAs<semantics::VariableDeclaration>())
                isVolatile = var->getAbsoluteAddress().has_value();
            else if (auto* arr = declIt->second->tryAs<semantics::ArrayDeclaration>())
                isVolatile = arr->getAbsoluteAddress().has_value();
            else if (auto* tab = declIt->second->tryAs<semantics::TableDeclaration>())
                isVolatile = tab->getAbsoluteAddress().has_value();
        }
    }

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

    // If we are assigning to an integer it could be fixed. check this first
    if (targetType && targetType->isIntegerTy())
    {
        const auto exprIt = m_program.getContext().expressionTypes.find(&statement.getExpression());
        std::uint32_t targetFraction = 0;
        std::uint32_t rhsFraction = 0;
        bool isTargetFixed = false;
        bool isRhsFixed = false;

        if (it != m_program.getContext().expressionTypes.end())
        {
            if (auto* fixedT = it->second->tryAs<semantics::FixedType>())
            {
                targetFraction = fixedT->getFractionalBits();
                isTargetFixed = true;
            }
        }
        if (exprIt != m_program.getContext().expressionTypes.end())
        {
            if (auto* fixedR = exprIt->second->tryAs<semantics::FixedType>())
            {
                rhsFraction = fixedR->getFractionalBits();
                isRhsFixed = true;
            }
        }

        if (isTargetFixed || isRhsFixed)
        {
            if (targetFraction > rhsFraction)
            {
                if (rhs->getType()->isFloatingPointTy())
                {
                    // Float represents the unscaled value. Scale by targetFraction.
                    const double factor = pow(2, targetFraction);
                    llvm::Value* tempFloat = llvm::ConstantFP::get(rhs->getType(), factor);
                    llvm::Value* multiplied = m_builder.CreateFMul(rhs, tempFloat, "scaled_val");
                    rhs = m_builder.CreateFPToUI(multiplied, m_builder.getInt32Ty(), "scaled_int");
                }
                else
                {
                    // Integer type: already scaled by rhsFraction. Scale up by the difference.
                    llvm::Value* shiftVal = llvm::ConstantInt::get(rhs->getType(), targetFraction - rhsFraction);
                    rhs = m_builder.CreateShl(rhs, shiftVal, "scaled_val_up");
                }
            }
            if (rhsFraction > targetFraction)
            {
                if (rhs->getType()->isFloatingPointTy())
                {
                    // Float represents the unscaled value. Scale by targetFraction.
                    const double factor = pow(2, targetFraction);
                    llvm::Value* tempFloat = llvm::ConstantFP::get(rhs->getType(), factor);
                    llvm::Value* multiplied = m_builder.CreateFMul(rhs, tempFloat, "scaled_val");
                    rhs = m_builder.CreateFPToUI(multiplied, m_builder.getInt32Ty(), "scaled_int");
                }
                else
                {
                    // Integer type: already scaled by rhsFraction. Scale down by the difference.
                    llvm::Value* shiftVal = llvm::ConstantInt::get(rhs->getType(), rhsFraction - targetFraction);
                    rhs = m_builder.CreateAShr(rhs, shiftVal, "scaled_val_down");
                }
            }
        }
    }

    if (rhs->getType() != targetType && targetType)
    {
        if (rhs->getType()->isFloatingPointTy() && targetType->isIntegerTy())
        {
            rhs = m_builder.CreateFPToSI(rhs, targetType);
        }
        else if (rhs->getType()->isIntegerTy() && targetType->isFloatingPointTy())
        {
            rhs = m_builder.CreateSIToFP(rhs, targetType);
            if (const auto expressionIt = m_program.getContext().expressionTypes.find(&statement.getExpression());
                expressionIt != m_program.getContext().expressionTypes.end())
            {
                if (auto* fixedR = expressionIt->second->tryAs<semantics::FixedType>())
                {
                    if (fixedR->getFractionalBits() > 0)
                    {
                        llvm::Value* divisor =
                            llvm::ConstantFP::get(targetType, static_cast<double>(1ULL << fixedR->getFractionalBits()));
                        rhs = m_builder.CreateFDiv(rhs, divisor, "fixed_to_float_scale");
                    }
                }
            }
        }
        else if (rhs->getType()->isFloatingPointTy() && targetType->isFloatingPointTy())
        {
            rhs = m_builder.CreateFPCast(rhs, targetType);
        }
        else if (rhs->getType()->isIntegerTy() && targetType->isIntegerTy())
        {
            rhs = m_builder.CreateIntCast(rhs, targetType, true);
        }
        else if (rhs->getType()->isIntegerTy() && targetType->isPointerTy())
        {
            rhs = m_builder.CreateIntToPtr(rhs, targetType);
        }
    }

    if (auto* partWord = dynamic_cast<const syntax::PartWordExpression*>(&statement.getVariable()))
    {
        llvm::Value* startBitVal = visit(partWord->getStartBit());

        if (llvm::Value* widthVal = visit(partWord->getWidth()); startBitVal && widthVal && targetType)
        {
            if (startBitVal->getType() != targetType)
                startBitVal = m_builder.CreateIntCast(startBitVal, targetType, false);
            if (widthVal->getType() != targetType)
                widthVal = m_builder.CreateIntCast(widthVal, targetType, false);

            llvm::Value* currentValue = m_builder.CreateLoad(targetType, targetAddr, "partword_load");

            llvm::Value* one = llvm::ConstantInt::get(targetType, 1);
            llvm::Value* shiftedOne = m_builder.CreateShl(one, widthVal);
            llvm::Value* widthMask = m_builder.CreateSub(shiftedOne, one);
            llvm::Value* mask = m_builder.CreateShl(widthMask, startBitVal);
            llvm::Value* invMask = m_builder.CreateNot(mask);

            llvm::Value* clearedBase = m_builder.CreateAnd(currentValue, invMask, "partword_cleared");

            llvm::Value* maskedRhs = m_builder.CreateAnd(rhs, widthMask);
            llvm::Value* shiftedRhs = m_builder.CreateShl(maskedRhs, startBitVal, "partword_shifted_rhs");

            rhs = m_builder.CreateOr(clearedBase, shiftedRhs, "partword_newval");
        }
    }
    else if (auto* arrAccess = dynamic_cast<const syntax::ArrayAccessExpression*>(&statement.getVariable()))
    {
        if (const auto rit = m_program.getContext().resolvedIdentifiers.find(arrAccess);
            rit != m_program.getContext().resolvedIdentifiers.end())
        {
            if (auto* tableElem = rit->second->tryAs<semantics::TableElementDeclaration>())
            {
                if (tableElem->getBitPosition().has_value())
                {
                    const std::uint32_t bitPos = tableElem->getBitPosition().value();
                    std::uint32_t bitLength = 32;
                    if (tableElem->getType().is<semantics::FixedType>())
                    {
                        bitLength = tableElem->getType().as<semantics::FixedType>().getTotalBits();
                    }
                    else if (auto* p = tableElem->getType().tryAs<semantics::PrimitiveType>())
                    {
                        if (p->getKind() == semantics::PrimitiveType::Integer
                            || p->getKind() == semantics::PrimitiveType::Bits)
                        {
                            bitLength = 32;
                            if (auto* astNode =
                                    dynamic_cast<const syntax::TableElementDeclaration*>(tableElem->getAstNode()))
                            {
                                if (astNode->getFixedTotalBits())
                                {
                                    if (const auto tVal =
                                            semantics::evaluateConstantInteger(*astNode->getFixedTotalBits());
                                        tVal && *tVal > 0)
                                    {
                                        bitLength = static_cast<std::uint32_t>(*tVal);
                                    }
                                }
                            }
                        }
                    }

                    if (bitLength > 0 && bitLength < 32)
                    {
                        llvm::Type* wordType = llvm::Type::getInt32Ty(m_module.getContext());
                        llvm::Value* currentValue = m_builder.CreateLoad(wordType, targetAddr, "table_elem_load");

                        const std::uint32_t maskValInt = (1U << bitLength) - 1;
                        llvm::Value* widthMask = llvm::ConstantInt::get(wordType, maskValInt, true);

                        llvm::Value* startBitVal = llvm::ConstantInt::get(wordType, bitPos, true);
                        llvm::Value* mask = m_builder.CreateShl(widthMask, startBitVal);
                        llvm::Value* invMask = m_builder.CreateNot(mask);

                        llvm::Value* clearedBase = m_builder.CreateAnd(currentValue, invMask, "table_elem_cleared");

                        if (rhs->getType() != wordType)
                        {
                            rhs = m_builder.CreateIntCast(rhs, wordType, true);
                        }

                        llvm::Value* maskedRhs = m_builder.CreateAnd(rhs, widthMask);
                        llvm::Value* shiftedRhs = m_builder.CreateShl(maskedRhs, startBitVal, "table_elem_shifted_rhs");

                        rhs = m_builder.CreateOr(clearedBase, shiftedRhs, "table_elem_newval");
                    }
                    else if (bitPos > 0)
                    {
                        // If it's a 32-bit field, but offset > 0 (which is weird for 32-bit words),
                        // we would just shift it, but if it takes the whole word, bitPos must be 0.
                        // Assuming 32-bit width and bitPos=0 is the only valid case for bitLength=32.
                    }
                }
            }
        }
    }

    auto* storeInst = m_builder.CreateStore(rhs, targetAddr);
    if (isVolatile)
        storeInst->setVolatile(true);
}

void CodeGenerator::visit(const syntax::IfStatement& statement)
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

void CodeGenerator::visit(const syntax::GotoStatement& statement)
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
        else if (auto* varDeclaration = declaration->tryAs<semantics::VariableDeclaration>())
        {
            if (varDeclaration->getType().is<semantics::LabelType>())
            {
                llvm::Value* ptrAddr = m_locals.contains(varDeclaration) ? m_locals[varDeclaration] : nullptr;
                if (!ptrAddr && m_globals.contains(varDeclaration))
                    ptrAddr = m_globals[varDeclaration];

                if (ptrAddr)
                {
                    llvm::Type* ptrType = llvm::PointerType::getUnqual(m_module.getContext());
                    llvm::Value* blockAddr = m_builder.CreateLoad(ptrType, ptrAddr, "label_param_load");

                    llvm::IndirectBrInst* indirectBr =
                        m_builder.CreateIndirectBr(blockAddr, std::max(static_cast<size_t>(1), m_namedLabels.size()));
                    if (m_namedLabels.empty())
                    {
                        llvm::BasicBlock* dummyBB =
                            llvm::BasicBlock::Create(m_module.getContext(), "dummy_dest", m_currentFunction);
                        indirectBr->addDestination(dummyBB);

                        // We also need to add a terminator to the dummy block to keep LLVM IR valid
                        llvm::BasicBlock* currentInsertBlock = m_builder.GetInsertBlock();
                        m_builder.SetInsertPoint(dummyBB);
                        m_builder.CreateUnreachable();
                        m_builder.SetInsertPoint(currentInsertBlock);
                    }
                    else
                    {
                        for (const auto& bb : m_namedLabels | std::views::values)
                        {
                            indirectBr->addDestination(bb);
                        }
                    }
                }
            }
            else if (varDeclaration->getType().is<semantics::SwitchType>())
            {
                llvm::Value* ptrAddr = m_locals.contains(varDeclaration) ? m_locals[varDeclaration] : nullptr;
                if (!ptrAddr && m_globals.contains(varDeclaration))
                    ptrAddr = m_globals[varDeclaration];

                if (ptrAddr)
                {
                    if (auto* indexExpr = statement.getIndex())
                    {
                        if (llvm::Value* indexVal = visit(*indexExpr))
                        {
                            if (!indexVal->getType()->isIntegerTy(32))
                            {
                                indexVal = m_builder.CreateIntCast(indexVal,
                                                                   llvm::Type::getInt32Ty(m_module.getContext()), true);
                            }

                            indexVal = m_builder.CreateSub(
                                indexVal, llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_module.getContext()), 1));

                            llvm::Type* ptrType = llvm::PointerType::getUnqual(m_module.getContext());
                            llvm::Value* switchArrayPtr = m_builder.CreateLoad(ptrType, ptrAddr, "switch_array_load");

                            llvm::Value* targetBlockPtrAddr =
                                m_builder.CreateGEP(ptrType, switchArrayPtr, indexVal, "switch_target_gep");
                            llvm::Value* blockAddr =
                                m_builder.CreateLoad(ptrType, targetBlockPtrAddr, "switch_target_load");

                            llvm::IndirectBrInst* indirectBr = m_builder.CreateIndirectBr(
                                blockAddr, std::max(static_cast<size_t>(1), m_namedLabels.size()));
                            if (m_namedLabels.empty())
                            {
                                llvm::BasicBlock* dummyBB =
                                    llvm::BasicBlock::Create(m_module.getContext(), "dummy_dest", m_currentFunction);
                                indirectBr->addDestination(dummyBB);

                                llvm::BasicBlock* currentInsertBlock = m_builder.GetInsertBlock();
                                m_builder.SetInsertPoint(dummyBB);
                                m_builder.CreateUnreachable();
                                m_builder.SetInsertPoint(currentInsertBlock);
                            }
                            else
                            {
                                for (const auto& bb : m_namedLabels | std::views::values)
                                {
                                    indirectBr->addDestination(bb);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void CodeGenerator::visit(const syntax::ForStatement& statement)
{
    llvm::Value* loopVarAddr = emitAddress(statement.getVariable());

    bool isVolatile = false;
    if (m_options.baremetal)
    {
        if (const auto declIt = m_program.getContext().resolvedIdentifiers.find(&statement.getVariable());
            declIt != m_program.getContext().resolvedIdentifiers.end())
        {
            if (auto* var = declIt->second->tryAs<semantics::VariableDeclaration>())
                isVolatile = var->getAbsoluteAddress().has_value();
            else if (auto* arr = declIt->second->tryAs<semantics::ArrayDeclaration>())
                isVolatile = arr->getAbsoluteAddress().has_value();
            else if (auto* tab = declIt->second->tryAs<semantics::TableDeclaration>())
                isVolatile = tab->getAbsoluteAddress().has_value();
        }
    }
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

            auto castToLoopVar = [&](llvm::Value* val) -> llvm::Value*
            {
                if (val->getType() == loopVarType)
                    return val;
                if (val->getType()->isIntegerTy() && loopVarType->isFloatingPointTy())
                    return m_builder.CreateSIToFP(val, loopVarType);
                if (val->getType()->isFloatingPointTy() && loopVarType->isIntegerTy())
                    return m_builder.CreateFPToSI(val, loopVarType);
                if (val->getType()->isFloatingPointTy() && loopVarType->isFloatingPointTy())
                    return m_builder.CreateFPCast(val, loopVarType);
                return m_builder.CreateIntCast(val, loopVarType, true);
            };

            v1 = castToLoopVar(v1);
            v2 = castToLoopVar(v2);
            v3 = castToLoopVar(v3);

            llvm::IRBuilder tmpBuilder(&m_currentFunction->getEntryBlock(), m_currentFunction->getEntryBlock().begin());

            llvm::AllocaInst* v1Addr = tmpBuilder.CreateAlloca(loopVarType, nullptr, "v1");
            llvm::AllocaInst* v2Addr = tmpBuilder.CreateAlloca(loopVarType, nullptr, "v2");
            llvm::AllocaInst* v3Addr = tmpBuilder.CreateAlloca(loopVarType, nullptr, "v3");

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
            {
                auto* st = m_builder.CreateStore(currV1, loopVarAddr);
                if (isVolatile)
                    st->setVolatile(true);
            }

            llvm::Value* currV2 = m_builder.CreateLoad(loopVarType, v2Addr);
            llvm::Value* currV3 = m_builder.CreateLoad(loopVarType, v3Addr);

            llvm::Value* isExhausted = nullptr;
            if (loopVarType->isFloatingPointTy())
            {
                llvm::Value* diff = m_builder.CreateFSub(currV1, currV3);
                llvm::Value* prod = m_builder.CreateFMul(diff, currV2);
                isExhausted = m_builder.CreateFCmpOGT(prod, llvm::ConstantFP::get(loopVarType, 0.0));
            }
            else
            {
                llvm::Value* diff = m_builder.CreateSub(currV1, currV3);
                llvm::Value* prod = m_builder.CreateMul(diff, currV2);
                isExhausted = m_builder.CreateICmpSGT(prod, llvm::ConstantInt::get(loopVarType, 0));
            }
            m_builder.CreateCondBr(isExhausted, nextElBB, bodyBB);

            m_builder.SetInsertPoint(bodyBB);
            visit(statement.getBody());
            m_builder.CreateBr(incBB);

            m_builder.SetInsertPoint(incBB);
            llvm::LoadInst* cv = m_builder.CreateLoad(loopVarType, loopVarAddr);
            if (isVolatile)
                cv->setVolatile(true);
            llvm::Value* nextV1 =
                loopVarType->isFloatingPointTy() ? m_builder.CreateFAdd(cv, currV2) : m_builder.CreateAdd(cv, currV2);
            m_builder.CreateStore(nextV1, v1Addr);
            m_builder.CreateBr(condBB);
        }
        else if (whileCondition)
        {
            llvm::BasicBlock* condBB =
                llvm::BasicBlock::Create(m_module.getContext(), "for.while.cond", m_currentFunction);
            llvm::BasicBlock* bodyBB =
                llvm::BasicBlock::Create(m_module.getContext(), "for.while.body", m_currentFunction);
            llvm::BasicBlock* evalBB =
                llvm::BasicBlock::Create(m_module.getContext(), "for.while.eval", m_currentFunction);

            m_builder.CreateBr(evalBB);
            m_builder.SetInsertPoint(evalBB);

            llvm::Value* exprVal = visit(*expression);
            if (exprVal->getType() != loopVarType)
            {
                if (exprVal->getType()->isIntegerTy() && loopVarType->isFloatingPointTy())
                    exprVal = m_builder.CreateSIToFP(exprVal, loopVarType);
                else if (exprVal->getType()->isFloatingPointTy() && loopVarType->isIntegerTy())
                    exprVal = m_builder.CreateFPToSI(exprVal, loopVarType);
                else if (exprVal->getType()->isFloatingPointTy() && loopVarType->isFloatingPointTy())
                    exprVal = m_builder.CreateFPCast(exprVal, loopVarType);
                else
                    exprVal = m_builder.CreateIntCast(exprVal, loopVarType, true);
            }
            if (loopVarAddr)
            {
                auto* st = m_builder.CreateStore(exprVal, loopVarAddr);
                if (isVolatile)
                    st->setVolatile(true);
            }

            m_builder.CreateBr(condBB);
            m_builder.SetInsertPoint(condBB);

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
            {
                if (exprVal->getType()->isIntegerTy() && loopVarType->isFloatingPointTy())
                    exprVal = m_builder.CreateSIToFP(exprVal, loopVarType);
                else if (exprVal->getType()->isFloatingPointTy() && loopVarType->isIntegerTy())
                    exprVal = m_builder.CreateFPToSI(exprVal, loopVarType);
                else if (exprVal->getType()->isFloatingPointTy() && loopVarType->isFloatingPointTy())
                    exprVal = m_builder.CreateFPCast(exprVal, loopVarType);
                else
                    exprVal = m_builder.CreateIntCast(exprVal, loopVarType, true);
            }
            if (loopVarAddr)
            {
                auto* st = m_builder.CreateStore(exprVal, loopVarAddr);
                if (isVolatile)
                    st->setVolatile(true);
            }

            visit(statement.getBody());
            m_builder.CreateBr(nextElBB);
        }

        m_builder.SetInsertPoint(nextElBB);
    }

    m_builder.CreateBr(endBB);
    m_builder.SetInsertPoint(endBB);
}

void CodeGenerator::visit(const syntax::AnswerStatement& statement)
{
    llvm::Value* val = visit(statement.getExpression());
    if (!val)
        return;
    if (llvm::Type* returnType = m_currentFunction->getReturnType(); val->getType() != returnType)
    {
        if (val->getType()->isIntegerTy() && returnType->isFloatingPointTy())
            val = m_builder.CreateSIToFP(val, returnType);
        else if (val->getType()->isFloatingPointTy() && returnType->isIntegerTy())
            val = m_builder.CreateFPToSI(val, returnType);
        else if (val->getType()->isFloatingPointTy() && returnType->isFloatingPointTy())
            val = m_builder.CreateFPCast(val, returnType);
        else if (val->getType()->isIntegerTy() && returnType->isIntegerTy())
            val = m_builder.CreateIntCast(val, returnType, true);
    }
    m_builder.CreateRet(val);
}

void CodeGenerator::visit(const syntax::CodeStatement& statement)
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
                constraints += ',';
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
                constraints += ',';
            constraints += 'r'; // Input constraint
        }
    }

    // Include clobbers for safety as per specification recommendation
    if (!constraints.empty())
        constraints += ',';
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

void CodeGenerator::visit(const syntax::ProcedureCallStatement& statement)
{
    visit(statement.getExpression());
}

} // namespace coral66::cgllvm
