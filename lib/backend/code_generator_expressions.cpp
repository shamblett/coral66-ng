/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "code_generator.hpp"

#include "lib/frontend/semantics/program.hpp"

namespace coral66::semantics
{
std::optional<std::int64_t> evaluateConstantInteger(const syntax::Expression& expr);
}

namespace coral66::cgllvm
{

llvm::Value* CodeGenerator::emitAddress(const syntax::Expression& expression)
{
    if (auto* idExpr = dynamic_cast<const syntax::IdentifierExpression*>(&expression))
    {
        if (const auto it = m_program.getContext().resolvedIdentifiers.find(idExpr);
            it != m_program.getContext().resolvedIdentifiers.end())
        {
            const auto* declaration = it->second;
            if (m_locals.contains(declaration))
                return m_locals[declaration];
            if (m_globals.contains(declaration))
                return m_globals[declaration];

            if (auto* tableElem = declaration->tryAs<semantics::TableElementDeclaration>())
            {
                llvm::Value* baseAddr = nullptr;
                llvm::Type* baseType = nullptr;
                if (auto* parentTable = tableElem->getParentTable())
                {
                    if (m_locals.contains(parentTable))
                        baseAddr = m_locals[parentTable];
                    else if (m_globals.contains(parentTable))
                        baseAddr = m_globals[parentTable];
                    baseType = convertType(parentTable->getType());
                }

                if (baseAddr && baseType)
                {
                    std::vector<llvm::Value*> indices;
                    if (baseType->isArrayTy())
                        indices.push_back(llvm::ConstantInt::get(m_module.getContext(), llvm::APInt(32, 0)));

                    llvm::Value* idxVal = llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_module.getContext()), 0, true);

                    if (tableElem->getWordPosition().has_value() && tableElem->getWordPosition().value() > 0)
                    {
                        llvm::Value* wordOffsetVal = llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_module.getContext()),
                                                                            tableElem->getWordPosition().value(), true);
                        idxVal = m_builder.CreateAdd(idxVal, wordOffsetVal, "table_word_offset");
                    }

                    indices.push_back(idxVal);
                    return m_builder.CreateGEP(baseType, baseAddr, indices, "table_elem_idx");
                }
            }
        }
    }
    else if (auto* part = dynamic_cast<const syntax::PartWordExpression*>(&expression))
    {
        return emitAddress(part->getBase());
    }
    else if (auto* arrAccess = dynamic_cast<const syntax::ArrayAccessExpression*>(&expression))
    {
        const auto it = m_program.getContext().resolvedIdentifiers.find(arrAccess);
        if (it == m_program.getContext().resolvedIdentifiers.end())
            return nullptr;

        const auto* declaration = it->second;
        llvm::Value* baseAddr = nullptr;
        if (m_locals.contains(declaration))
            baseAddr = m_locals[declaration];
        else if (m_globals.contains(declaration))
            baseAddr = m_globals[declaration];
        else if (auto* tableElem = declaration->tryAs<semantics::TableElementDeclaration>())
        {
            if (auto* parentTable = tableElem->getParentTable())
            {
                if (m_locals.contains(parentTable))
                    baseAddr = m_locals[parentTable];
                else if (m_globals.contains(parentTable))
                    baseAddr = m_globals[parentTable];
            }
        }

        if (!baseAddr)
            return nullptr;

        llvm::Type* baseType = nullptr;
        const syntax::ArrayDeclaration* arrDeclNode = nullptr;
        const semantics::TableElementDeclaration* tableElemNode = nullptr;
        std::uint32_t tableWidth = 0;

        declaration->match(
            [&](const semantics::ArrayDeclaration& arrayDeclaration)
            {
                baseType = convertType(arrayDeclaration.getType());
                arrDeclNode = dynamic_cast<const syntax::ArrayDeclaration*>(arrayDeclaration.getAstNode());
            },
            [&](const semantics::VariableDeclaration& varDecl) { baseType = convertType(varDecl.getType()); },
            [&](const semantics::TableElementDeclaration& tableElemDecl)
            {
                tableElemNode = &tableElemDecl;
                if (tableElemDecl.getWordPosition().has_value()) {
                    llvm::errs() << "TableElem: " << tableElemDecl.getMangledName() << " WP: " << tableElemDecl.getWordPosition().value() << "\n";
                } else {
                    llvm::errs() << "TableElem: " << tableElemDecl.getMangledName() << " NO WP\n";
                }
                if (auto* parentTable = tableElemDecl.getParentTable())
                {
                    baseType = convertType(parentTable->getType());
                    if (auto* tabType = parentTable->getType().tryAs<semantics::TableType>())
                    {
                        if (auto* def = std::get_if<semantics::TableDefinition>(&tabType->getInfo().type))
                        {
                            tableWidth = def->getWidth();
                        }
                    }
                }
            },
            [&](const semantics::TableDeclaration& tableDeclaration)
            {
                baseType = convertType(tableDeclaration.getType());
                if (auto* tabType = tableDeclaration.getType().tryAs<semantics::TableType>())
                {
                    if (auto* def = std::get_if<semantics::TableDefinition>(&tabType->getInfo().type))
                    {
                        tableWidth = def->getWidth();
                    }
                }
            },
            [&](const auto&) {});

        if (!baseType)
            return nullptr;

        std::vector<llvm::Value*> indices;
        if (baseType->isArrayTy())
            indices.push_back(llvm::ConstantInt::get(m_module.getContext(), llvm::APInt(32, 0)));

        for (std::size_t i = 0; i < arrAccess->getIndices().size(); ++i)
        {
            const auto& idxExpr = arrAccess->getIndices()[i];
            llvm::Value* idxVal = visit(*idxExpr);
            if (!idxVal)
                return nullptr;
            if (idxVal->getType()->isFloatingPointTy())
            {
                idxVal = m_builder.CreateFPToSI(idxVal, llvm::Type::getInt32Ty(m_module.getContext()));
            }
            else if (!idxVal->getType()->isIntegerTy(32) && idxVal->getType()->isIntegerTy())
            {
                idxVal = m_builder.CreateIntCast(idxVal, llvm::Type::getInt32Ty(m_module.getContext()), true);
            }

            if (arrDeclNode && i < arrDeclNode->getDimensions().size())
            {
                if (auto lower = semantics::evaluateConstantInteger(*arrDeclNode->getDimensions()[i].first);
                    lower && *lower != 0)
                {
                    llvm::Value* lowerVal =
                        llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_module.getContext()), *lower, true);
                    idxVal = m_builder.CreateSub(idxVal, lowerVal, "idx_offset");
                }
            }

            if (tableElemNode)
            {
                if (tableWidth > 1)
                {
                    llvm::Value* widthVal =
                        llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_module.getContext()), tableWidth, true);
                    idxVal = m_builder.CreateMul(idxVal, widthVal, "table_row_offset");
                }

                if (tableElemNode->getWordPosition().has_value() && tableElemNode->getWordPosition().value() > 0)
                {
                    llvm::Value* wordOffsetVal = llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_module.getContext()),
                                                                        tableElemNode->getWordPosition().value(), true);
                    idxVal = m_builder.CreateAdd(idxVal, wordOffsetVal, "table_word_offset");
                }
            }

            indices.push_back(idxVal);
        }

        return m_builder.CreateGEP(baseType, baseAddr, indices, "arrayidx");
    }
    return nullptr;
}

llvm::Value* CodeGenerator::visit(const syntax::Expression& expression)
{
    if (auto* e = dynamic_cast<const syntax::LiteralExpression*>(&expression))
        return visit(*e);
    if (auto* e = dynamic_cast<const syntax::IdentifierExpression*>(&expression))
        return visit(*e);
    if (auto* e = dynamic_cast<const syntax::BinaryExpression*>(&expression))
        return visit(*e);
    if (auto* e = dynamic_cast<const syntax::UnaryExpression*>(&expression))
        return visit(*e);
    if (auto* e = dynamic_cast<const syntax::FunctionCallExpression*>(&expression))
        return visit(*e);
    if (auto* e = dynamic_cast<const syntax::PartWordExpression*>(&expression))
        return visit(*e);
    if (auto* e = dynamic_cast<const syntax::ArrayAccessExpression*>(&expression))
        return visit(*e);
    if (auto* e = dynamic_cast<const syntax::LocationExpression*>(&expression))
        return visit(*e);
    if (auto* e = dynamic_cast<const syntax::TypeChangeExpression*>(&expression))
        return visit(*e);
    return nullptr;
}

// ReSharper disable once CppMemberFunctionMayBeConst
llvm::Value* CodeGenerator::visit(const syntax::LiteralExpression& expression)
{
    auto& val = expression.getLiteral()->getValue();
    if (expression.begin()->getTokenType() == lexer::TokenType::LiteralKeyword)
    {
        if (std::holds_alternative<std::string>(val))
        {
            const auto& strValue = std::get<std::string>(val);
            if (!strValue.empty())
            {
                return llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_module.getContext()),
                                              static_cast<std::uint8_t>(strValue[0]), false);
            }
        }
    }
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
        const auto& strValue = std::get<std::string>(val);
        // Standard Coral string literals should be emitted as global string constants
        // returning an i8* (pointer to characters) which allows passing to EXTERNAL C functions.
        return m_builder.CreateGlobalString(strValue);
    }
    return nullptr;
}

llvm::Value* CodeGenerator::visit(const syntax::IdentifierExpression& expression)
{
    if (const auto it = m_program.getContext().resolvedIdentifiers.find(&expression);
        it != m_program.getContext().resolvedIdentifiers.end())
    {
        const auto* declaration = it->second;
        if (auto* procedureDeclaration = declaration->tryAs<semantics::ProcedureDeclaration>())
        {
            // It's a procedure call without arguments!
            llvm::Value* funcVal = m_globals.contains(declaration) ? m_globals[declaration] : nullptr;
            if (!funcVal)
                return nullptr;

            llvm::FunctionType* ft = nullptr;
            if (const auto* procType = procedureDeclaration->getType().tryAs<semantics::ProcedureType>())
            {
                llvm::Type* retType = llvm::Type::getVoidTy(m_module.getContext());
                if (procType->getReturnType())
                    retType = convertType(*procType->getReturnType());
                constexpr std::vector<llvm::Type*> params;
                ft = llvm::FunctionType::get(retType, params, false);
            }
            if (ft)
            {
                llvm::Value* callRes = m_builder.CreateCall(ft, funcVal, {});
                emitNonLocalGotoCheck();
                return callRes;
            }
            return nullptr;
        }
        if (auto* varDeclaration = declaration->tryAs<semantics::VariableDeclaration>())
        {
            if (const auto* procType = varDeclaration->getType().tryAs<semantics::ProcedureType>())
            {
                llvm::Value* addr = emitAddress(expression);
                if (!addr)
                    return nullptr;
                llvm::Value* funcVal = m_builder.CreateLoad(llvm::PointerType::getUnqual(m_module.getContext()), addr,
                                                            "proc_param_load");
                llvm::Type* retType = llvm::Type::getVoidTy(m_module.getContext());
                if (procType->getReturnType())
                    retType = convertType(*procType->getReturnType());
                constexpr std::vector<llvm::Type*> params;
                llvm::FunctionType* ft = llvm::FunctionType::get(retType, params, false);
                llvm::Value* callRes = m_builder.CreateCall(ft, funcVal, {});
                emitNonLocalGotoCheck();
                return callRes;
            }
        }
        if (declaration->is<semantics::LabelDeclaration>())
        {
            if (const auto& labelName = std::get<std::string>(expression.getIdentifier()->getValue());
                m_namedLabels.contains(labelName))
            {
                return llvm::BlockAddress::get(m_currentFunction, m_namedLabels[labelName]);
            }
        }
        else if (auto* switchDeclaration = declaration->tryAs<semantics::SwitchDeclaration>())
        {
            auto& switchLabels = switchDeclaration->getLabelDeclarations();
            llvm::Type* ptrType = llvm::PointerType::getUnqual(m_module.getContext());
            llvm::ArrayType* arrType = llvm::ArrayType::get(ptrType, switchLabels.size());

            llvm::AllocaInst* switchArray = m_builder.CreateAlloca(arrType, nullptr, "switch_array");

            for (size_t i = 0; i < switchLabels.size(); ++i)
            {
                if (switchLabels[i])
                {
                    if (const std::string targetLabelName(switchLabels[i]->getName());
                        m_namedLabels.contains(targetLabelName))
                    {
                        llvm::BlockAddress* blockAddr =
                            llvm::BlockAddress::get(m_currentFunction, m_namedLabels[targetLabelName]);

                        llvm::Value* indices[] = {
                            llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_module.getContext()), 0),
                            llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_module.getContext()), i)};

                        llvm::Value* gep = m_builder.CreateGEP(arrType, switchArray, indices);
                        m_builder.CreateStore(blockAddr, gep);
                    }
                }
            }

            llvm::Value* indices[] = {llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_module.getContext()), 0),
                                      llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_module.getContext()), 0)};
            return m_builder.CreateGEP(arrType, switchArray, indices);
        }
    }

    llvm::Value* addr = emitAddress(expression);
    if (!addr)
        return nullptr;
    if (llvm::isa<llvm::Function>(addr))
        return addr;

    llvm::Type* type = nullptr;
    if (const auto typeIt = m_program.getContext().expressionTypes.find(&expression);
        typeIt != m_program.getContext().expressionTypes.end())
    {
        if (typeIt->second->is<semantics::TableType>() || typeIt->second->is<semantics::ArrayType>())
        {
            return addr;
        }

        type = convertType(*typeIt->second);
        // If the variable is a LOCATION parameter, the address holds a pointer to the actual value.
        // Wait, emitAddress returns the pointer. So if type is ptr, we want to load the underlying primitive type.
        // Actually, expressionTypes gives the type of the expression. A LOCATION variable used in an expression
        // has the underlying PrimitiveType, NOT Location!
        // Let's ensure the type is not a pointer if it's evaluated.
        if (type->isPointerTy() && typeIt->second->isLocation())
        {
            // Location variables evaluate to their underlying type!
            type = convertType(semantics::PrimitiveType(typeIt->second->tryAs<semantics::PrimitiveType>()->getKind()));
        }
    }
    else
    {
        type = llvm::Type::getInt32Ty(m_module.getContext());
    }
    bool isVolatile = false;
    if (m_options.baremetal)
    {
        if (const auto declIt = m_program.getContext().resolvedIdentifiers.find(&expression);
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
    llvm::LoadInst* loadInst = m_builder.CreateLoad(type, addr);
    if (isVolatile)
        loadInst->setVolatile(true);
    return loadInst;
}

llvm::Value* CodeGenerator::visit(const syntax::BinaryExpression& expression)
{
    llvm::Value* L = visit(expression.getLeft());
    llvm::Value* R = visit(expression.getRight());
    if (!L || !R)
        return nullptr;

    const auto kind = expression.getOp()->getTokenType();

    // Check for and process fixed values in the expression
    if (const auto value = processFixedBinaryExpression(expression))
    {
        if (value != nullptr)
        {
            return value;
        }
    }

    if (kind == lexer::TokenType::MaskKeyword || kind == lexer::TokenType::UnionKeyword
        || kind == lexer::TokenType::DifferKeyword)
    {
        if (L->getType()->isPointerTy())
            L = m_builder.CreatePtrToInt(L, llvm::Type::getInt32Ty(m_module.getContext()));
        if (R->getType()->isPointerTy())
            R = m_builder.CreatePtrToInt(R, llvm::Type::getInt32Ty(m_module.getContext()));
    }

    if (L->getType() != R->getType())
    {
        if (L->getType()->isIntegerTy() && R->getType()->isFloatingPointTy())
        {
            L = m_builder.CreateSIToFP(L, R->getType());
            if (const auto it = m_program.getContext().expressionTypes.find(&expression.getLeft());
                it != m_program.getContext().expressionTypes.end())
            {
                if (auto* fixed = it->second->tryAs<semantics::FixedType>())
                {
                    if (fixed->getFractionalBits() > 0)
                    {
                        llvm::Value* divisor = llvm::ConstantFP::get(
                            R->getType(), static_cast<double>(1ULL << fixed->getFractionalBits()));
                        L = m_builder.CreateFDiv(L, divisor, "fixed_to_float_scale");
                    }
                }
            }
        }
        else if (L->getType()->isFloatingPointTy() && R->getType()->isIntegerTy())
        {
            R = m_builder.CreateSIToFP(R, L->getType());
            if (const auto it = m_program.getContext().expressionTypes.find(&expression.getRight());
                it != m_program.getContext().expressionTypes.end())
            {
                if (auto* fixed = it->second->tryAs<semantics::FixedType>())
                {
                    if (fixed->getFractionalBits() > 0)
                    {
                        llvm::Value* divisor = llvm::ConstantFP::get(
                            L->getType(), static_cast<double>(1ULL << fixed->getFractionalBits()));
                        R = m_builder.CreateFDiv(R, divisor, "fixed_to_float_scale");
                    }
                }
            }
        }
        else if (L->getType()->isIntegerTy() && R->getType()->isIntegerTy())
        {
            if (L->getType()->getIntegerBitWidth() > R->getType()->getIntegerBitWidth())
                R = m_builder.CreateIntCast(R, L->getType(), true);
            else
                L = m_builder.CreateIntCast(L, R->getType(), true);
        }
        else if (L->getType()->isFloatingPointTy() && R->getType()->isFloatingPointTy())
        {
            if (L->getType()->getFPMantissaWidth() > R->getType()->getFPMantissaWidth())
                R = m_builder.CreateFPCast(R, L->getType());
            else
                L = m_builder.CreateFPCast(L, R->getType());
        }
    }

    if (kind == lexer::TokenType::Plus)
        return L->getType()->isFloatingPointTy() ? m_builder.CreateFAdd(L, R) : m_builder.CreateAdd(L, R);
    if (kind == lexer::TokenType::Minus)
        return L->getType()->isFloatingPointTy() ? m_builder.CreateFSub(L, R) : m_builder.CreateSub(L, R);
    if (kind == lexer::TokenType::Asterisk)
    {
        if (L->getType()->isFloatingPointTy())
            return m_builder.CreateFMul(L, R);
        llvm::Value* mul = m_builder.CreateMul(L, R);
        return mul;
    }
    if (kind == lexer::TokenType::Division)
    {
        if (L->getType()->isFloatingPointTy())
            return m_builder.CreateFDiv(L, R);
        return m_builder.CreateSDiv(L, R);
    }
    if (kind == lexer::TokenType::Equal)
        return L->getType()->isFloatingPointTy() ? m_builder.CreateFCmpOEQ(L, R) : m_builder.CreateICmpEQ(L, R);
    if (kind == lexer::TokenType::NotEqual)
        return L->getType()->isFloatingPointTy() ? m_builder.CreateFCmpONE(L, R) : m_builder.CreateICmpNE(L, R);
    if (kind == lexer::TokenType::LessThan)
        return L->getType()->isFloatingPointTy() ? m_builder.CreateFCmpOLT(L, R) : m_builder.CreateICmpSLT(L, R);
    if (kind == lexer::TokenType::LessThanOrEqual)
        return L->getType()->isFloatingPointTy() ? m_builder.CreateFCmpOLE(L, R) : m_builder.CreateICmpSLE(L, R);
    if (kind == lexer::TokenType::GreaterThan)
        return L->getType()->isFloatingPointTy() ? m_builder.CreateFCmpOGT(L, R) : m_builder.CreateICmpSGT(L, R);
    if (kind == lexer::TokenType::GreaterThanOrEqual)
        return L->getType()->isFloatingPointTy() ? m_builder.CreateFCmpOGE(L, R) : m_builder.CreateICmpSGE(L, R);
    if (kind == lexer::TokenType::AndKeyword || kind == lexer::TokenType::MaskKeyword)
        return m_builder.CreateAnd(L, R);
    if (kind == lexer::TokenType::OrKeyword || kind == lexer::TokenType::UnionKeyword)
        return m_builder.CreateOr(L, R);
    if (kind == lexer::TokenType::DifferKeyword)
        return m_builder.CreateXor(L, R);
    return nullptr;
}

llvm::Value* CodeGenerator::visit(const syntax::UnaryExpression& expression)
{
    llvm::Value* operand = visit(expression.getOperand());
    if (!operand)
        return nullptr;

    const auto kind = expression.getOp()->getTokenType();
    if (kind == lexer::TokenType::Minus)
        return operand->getType()->isFloatingPointTy() ? m_builder.CreateFNeg(operand) : m_builder.CreateNeg(operand);
    if (kind == lexer::TokenType::Plus)
        return operand;
    return nullptr;
}

llvm::Value* CodeGenerator::visit(const syntax::FunctionCallExpression& expression)
{
    const auto it = m_program.getContext().resolvedIdentifiers.find(&expression);
    if (it == m_program.getContext().resolvedIdentifiers.end())
        return nullptr;

    const auto* declaration = it->second;
    llvm::Value* funcVal = m_globals.contains(declaration) ? m_globals[declaration] : nullptr;
    if (!funcVal && m_locals.contains(declaration))
    {
        funcVal = m_builder.CreateLoad(llvm::PointerType::getUnqual(m_module.getContext()), m_locals[declaration],
                                       "proc_param_load");
    }
    if (!funcVal)
        return nullptr;

    std::vector<llvm::Value*> args;

    llvm::FunctionType* ft = nullptr;
    const semantics::ProcedureType* procType = nullptr;

    if (auto* procedureDeclaration = declaration->tryAs<semantics::ProcedureDeclaration>())
    {
        procType = procedureDeclaration->getType().tryAs<semantics::ProcedureType>();
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
    }
    else if (auto* varDeclaration = declaration->tryAs<semantics::VariableDeclaration>())
    {
        procType = varDeclaration->getType().tryAs<semantics::ProcedureType>();
    }

    size_t i = 0;
    for (const auto& arg : expression.getArguments())
    {
        if (procType && i < procType->getParameters().size()
            && (procType->getParameters()[i].type->isLocation()
                || procType->getParameters()[i].type->tryAs<semantics::ArrayType>()))
        {
            llvm::Value* addr = emitAddress(*arg);
            if (!addr)
                addr = visit(*arg); // Fallback
            args.push_back(addr);
        }
        else if (procType && i < procType->getParameters().size()
                 && procType->getParameters()[i].type->is<semantics::ProcedureType>())
        {
            llvm::Value* procArg = nullptr;
            if (auto* idExpr = dynamic_cast<const syntax::IdentifierExpression*>(arg.get()))
            {
                if (const auto argIt = m_program.getContext().resolvedIdentifiers.find(idExpr);
                    argIt != m_program.getContext().resolvedIdentifiers.end())
                {
                    if (argIt->second->is<semantics::ProcedureDeclaration>())
                    {
                        procArg = m_globals.contains(argIt->second) ? m_globals[argIt->second] : nullptr;
                    }
                    else if (argIt->second->is<semantics::VariableDeclaration>())
                    {
                        if (llvm::Value* addr = emitAddress(*arg))
                        {
                            procArg = m_builder.CreateLoad(llvm::PointerType::getUnqual(m_module.getContext()), addr,
                                                           "proc_param_forward");
                        }
                    }
                }
            }
            if (!procArg)
                procArg = visit(*arg);
            args.push_back(procArg);
        }
        else
        {
            args.push_back(visit(*arg));
        }
        i++;
    }

    if (!ft && procType)
    {
        llvm::Type* retType = llvm::Type::getVoidTy(m_module.getContext());
        if (procType->getReturnType())
            retType = convertType(*procType->getReturnType());
        std::vector<llvm::Type*> params;
        for (auto* a : args)
        {
            if (!a)
                return nullptr;
            params.push_back(a->getType());
        }
        ft = llvm::FunctionType::get(retType, params, false);
    }

    if (ft)
    {
        for (size_t i1 = 0; i1 < args.size() && i1 < ft->getNumParams(); ++i1)
        {
            if (args[i1]->getType() != ft->getParamType(i1))
            {
                const llvm::Type* argumentType = args[i1]->getType();
                if (llvm::Type* parameterType = ft->getParamType(i1);
                    argumentType->isPointerTy() && parameterType->isIntegerTy())
                    args[i1] = m_builder.CreatePtrToInt(args[i1], parameterType);
                else if (argumentType->isIntegerTy() && parameterType->isPointerTy())
                    args[i1] = m_builder.CreateIntToPtr(args[i1], parameterType);
                else if (argumentType->isIntegerTy() && parameterType->isFloatingPointTy())
                    args[i1] = m_builder.CreateSIToFP(args[i1], parameterType);
                else if (argumentType->isFloatingPointTy() && parameterType->isIntegerTy())
                    args[i1] = m_builder.CreateFPToSI(args[i1], parameterType);
                else if (argumentType->isFloatingPointTy() && parameterType->isFloatingPointTy())
                    args[i1] = m_builder.CreateFPCast(args[i1], parameterType);
                else if (argumentType->isIntegerTy() && parameterType->isIntegerTy())
                    args[i1] = m_builder.CreateIntCast(args[i1], parameterType, true);
                // If it's something else (like an array value), we skip casting to avoid crashes.
            }
        }
        llvm::Value* callRes = m_builder.CreateCall(ft, funcVal, args);
        emitNonLocalGotoCheck();
        return callRes;
    }
    return nullptr;
}

llvm::Value* CodeGenerator::visit(const syntax::PartWordExpression& expression)
{
    llvm::Value* baseVal = visit(expression.getBase());
    llvm::Value* startBitVal = visit(expression.getStartBit());
    llvm::Value* widthVal = visit(expression.getWidth());

    if (!baseVal || !startBitVal || !widthVal)
        return nullptr;

    llvm::Type* baseType = baseVal->getType();
    if (!baseType->isIntegerTy())
    {
        baseVal = m_builder.CreateBitCast(baseVal, llvm::Type::getInt32Ty(m_module.getContext()));
        baseType = baseVal->getType();
    }

    if (startBitVal->getType() != baseType)
        startBitVal = m_builder.CreateIntCast(startBitVal, baseType, false);

    if (widthVal->getType() != baseType)
        widthVal = m_builder.CreateIntCast(widthVal, baseType, false);

    llvm::Value* shifted = m_builder.CreateLShr(baseVal, startBitVal, "partword_shift");

    llvm::Value* one = llvm::ConstantInt::get(baseType, 1);
    llvm::Value* shiftedOne = m_builder.CreateShl(one, widthVal, "partword_mask_shl");
    llvm::Value* mask = m_builder.CreateSub(shiftedOne, one, "partword_mask");

    return m_builder.CreateAnd(shifted, mask, "partword_val");
}

llvm::Value* CodeGenerator::visit(const syntax::ArrayAccessExpression& expression)
{
    const auto rit = m_program.getContext().resolvedIdentifiers.find(&expression);
    if (rit != m_program.getContext().resolvedIdentifiers.end() && expression.getIndices().size() == 1)
    {
        if (auto* switchDeclaration = rit->second->tryAs<semantics::SwitchDeclaration>())
        {
            llvm::Value* indexVal = visit(*expression.getIndices()[0]);
            if (!indexVal)
                return nullptr;
            if (!indexVal->getType()->isIntegerTy(32))
                indexVal = m_builder.CreateIntCast(indexVal, llvm::Type::getInt32Ty(m_module.getContext()), true);

            const auto& switchLabels = switchDeclaration->getLabelDeclarations();
            if (auto* constIdx = llvm::dyn_cast<llvm::ConstantInt>(indexVal))
            {
                const std::int64_t idx = constIdx->getSExtValue() - 1;
                if (idx >= 0 && static_cast<std::size_t>(idx) < switchLabels.size() && switchLabels[idx])
                {
                    if (const std::string targetLabelName(switchLabels[idx]->getName());
                        m_namedLabels.contains(targetLabelName))
                    {
                        return llvm::BlockAddress::get(m_currentFunction, m_namedLabels[targetLabelName]);
                    }
                }
                return llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(m_module.getContext()));
            }

            llvm::Type* ptrType = llvm::PointerType::getUnqual(m_module.getContext());
            llvm::ArrayType* arrType = llvm::ArrayType::get(ptrType, switchLabels.size());
            llvm::AllocaInst* switchArray = m_builder.CreateAlloca(arrType, nullptr, "switch_array");

            for (std::size_t i = 0; i < switchLabels.size(); ++i)
            {
                if (switchLabels[i])
                {
                    if (const std::string targetLabelName(switchLabels[i]->getName());
                        m_namedLabels.contains(targetLabelName))
                    {
                        llvm::BlockAddress* blockAddr =
                            llvm::BlockAddress::get(m_currentFunction, m_namedLabels[targetLabelName]);
                        llvm::Value* indices[] = {
                            llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_module.getContext()), 0),
                            llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_module.getContext()), i)};
                        llvm::Value* gep = m_builder.CreateGEP(arrType, switchArray, indices);
                        m_builder.CreateStore(blockAddr, gep);
                    }
                }
            }

            llvm::Value* zeroIdx = m_builder.CreateSub(
                indexVal, llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_module.getContext()), 1));
            llvm::Value* indices[] = {llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_module.getContext()), 0),
                                      zeroIdx};
            llvm::Value* elemPtr = m_builder.CreateGEP(arrType, switchArray, indices, "switch_elem_gep");
            return m_builder.CreateLoad(ptrType, elemPtr, "switch_elem_load");
        }
        if (auto* varDeclaration = rit->second->tryAs<semantics::VariableDeclaration>();
            varDeclaration && varDeclaration->getType().is<semantics::SwitchType>())
        {
            llvm::Value* ptrAddr = m_locals.contains(varDeclaration) ? m_locals[varDeclaration] : nullptr;
            if (!ptrAddr && m_globals.contains(varDeclaration))
                ptrAddr = m_globals[varDeclaration];
            if (!ptrAddr)
                return nullptr;

            llvm::Value* indexVal = visit(*expression.getIndices()[0]);
            if (!indexVal)
                return nullptr;
            if (!indexVal->getType()->isIntegerTy(32))
                indexVal = m_builder.CreateIntCast(indexVal, llvm::Type::getInt32Ty(m_module.getContext()), true);
            indexVal = m_builder.CreateSub(
                indexVal, llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_module.getContext()), 1));

            llvm::Type* ptrType = llvm::PointerType::getUnqual(m_module.getContext());
            llvm::Value* switchArrayPtr = m_builder.CreateLoad(ptrType, ptrAddr, "switch_array_load");
            llvm::Value* targetBlockPtrAddr =
                m_builder.CreateGEP(ptrType, switchArrayPtr, indexVal, "switch_target_gep");
            return m_builder.CreateLoad(ptrType, targetBlockPtrAddr, "switch_target_load");
        }
    }

    llvm::Value* addr = emitAddress(expression);
    if (!addr)
        return nullptr;

    llvm::Type* type = llvm::Type::getInt32Ty(m_module.getContext());
    const semantics::TableElementDeclaration* tableElem = nullptr;

    if (rit != m_program.getContext().resolvedIdentifiers.end())
    {
        if (auto* arrayDeclaration = rit->second->tryAs<semantics::ArrayDeclaration>())
        {
            type = convertType(arrayDeclaration->getType().tryAs<semantics::ArrayType>()->getElementType());
        }
        else if (auto* tabElem = rit->second->tryAs<semantics::TableElementDeclaration>())
        {
            type = llvm::Type::getInt32Ty(m_module.getContext()); // The array element type is always i32 for tables
            tableElem = tabElem;
        }
        else if (rit->second->is<semantics::TableDeclaration>())
        {
            type = llvm::Type::getInt32Ty(m_module.getContext());
        }
    }

    bool isVolatile = false;
    if (m_options.baremetal)
    {
        if (const auto declIt = m_program.getContext().resolvedIdentifiers.find(&expression);
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

    llvm::LoadInst* loadInst = m_builder.CreateLoad(type, addr, "arrayload");
    if (isVolatile)
        loadInst->setVolatile(true);
    llvm::Value* loaded = loadInst;

    if (tableElem && tableElem->getBitPosition().has_value())
    {
        const std::uint32_t bitPos = tableElem->getBitPosition().value();
        std::uint32_t bitLength = 32;

        if (tableElem->getType().is<semantics::FixedType>())
        {
            bitLength = tableElem->getType().as<semantics::FixedType>().getTotalBits();
        }
        else if (auto* p = tableElem->getType().tryAs<semantics::PrimitiveType>())
        {
            if (p->getKind() == semantics::PrimitiveType::Integer || p->getKind() == semantics::PrimitiveType::Bits)
            {
                bitLength = 32;
                if (auto* astNode = dynamic_cast<const syntax::TableElementDeclaration*>(tableElem->getAstNode()))
                {
                    if (astNode->getFixedTotalBits())
                    {
                        if (const auto tVal = semantics::evaluateConstantInteger(*astNode->getFixedTotalBits());
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
            llvm::Value* shiftVal = llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_module.getContext()), bitPos, true);
            llvm::Value* shifted = m_builder.CreateLShr(loaded, shiftVal, "partword_shift");

            const std::uint32_t mask = (1U << bitLength) - 1;
            llvm::Value* maskVal = llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_module.getContext()), mask, true);
            llvm::Value* masked = m_builder.CreateAnd(shifted, maskVal, "partword_mask");

            // If it's signed (i.e. not unsigned), we should sign-extend it if the MSB is set.
            if (!tableElem->isUnsigned())
            {
                llvm::Value* shiftAmt =
                    llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_module.getContext()), 32 - bitLength, true);
                llvm::Value* shl = m_builder.CreateShl(masked, shiftAmt);
                masked = m_builder.CreateAShr(shl, shiftAmt, "partword_sext");
            }
            loaded = masked;
        }
        else if (bitPos > 0)
        {
            // Just shift if length is 32 (though bitPos + 32 would overflow the word)
            llvm::Value* shiftVal = llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_module.getContext()), bitPos, true);
            loaded = m_builder.CreateLShr(loaded, shiftVal, "partword_shift");
        }
    }

    return loaded;
}

llvm::Value* CodeGenerator::visit(const syntax::LocationExpression& expression)
{
    return emitAddress(expression.getWordReference());
}

llvm::Value* CodeGenerator::visit(const syntax::TypeChangeExpression& expression)
{
    llvm::Value* val = visit(expression.getExpression());
    if (!val)
        return nullptr;

    const auto it = m_program.getContext().expressionTypes.find(&expression);
    if (it == m_program.getContext().expressionTypes.end())
        return val;

    llvm::Type* targetType = convertType(*it->second);

    std::uint32_t targetFraction = 0;
    bool isTargetFixed = false;
    if (auto* fixedT = it->second->tryAs<semantics::FixedType>())
    {
        targetFraction = fixedT->getFractionalBits();
        isTargetFixed = true;
    }

    std::uint32_t valFraction = 0;
    bool isValFixed = false;
    if (const auto expressionIterator = m_program.getContext().expressionTypes.find(&expression.getExpression());
        expressionIterator != m_program.getContext().expressionTypes.end())
    {
        if (auto* fixedV = expressionIterator->second->tryAs<semantics::FixedType>())
        {
            valFraction = fixedV->getFractionalBits();
            isValFixed = true;
        }
    }

    // Convert from FIXED to FLOATING
    if (targetType->isFloatingPointTy() && val->getType()->isIntegerTy())
    {
        val = m_builder.CreateSIToFP(val, targetType);
        if (isValFixed && valFraction > 0)
        {
            llvm::Value* divisor = llvm::ConstantFP::get(targetType, static_cast<double>(1ULL << valFraction));
            val = m_builder.CreateFDiv(val, divisor, "fixed_to_float_scale");
        }
        return val;
    }

    // Convert from FLOATING to FIXED or INTEGER
    if (targetType->isIntegerTy() && val->getType()->isFloatingPointTy())
    {
        if (isTargetFixed && targetFraction > 0)
        {
            llvm::Value* multiplier =
                llvm::ConstantFP::get(val->getType(), static_cast<double>(1ULL << targetFraction));
            val = m_builder.CreateFMul(val, multiplier, "float_to_fixed_scale");
        }
        return m_builder.CreateFPToSI(val, targetType);
    }

    // Convert between floating points
    if (targetType->isFloatingPointTy() && val->getType()->isFloatingPointTy())
        return m_builder.CreateFPCast(val, targetType);

    // Convert between INTEGER and FIXED (or FIXED and FIXED)
    if (targetType->isIntegerTy() && val->getType()->isIntegerTy())
    {
        if (val->getType()->getIntegerBitWidth() < targetType->getIntegerBitWidth())
            val = m_builder.CreateIntCast(val, targetType, true);

        if (isTargetFixed || isValFixed)
        {
            if (targetFraction > valFraction)
            {
                llvm::Value* shiftVal = llvm::ConstantInt::get(val->getType(), targetFraction - valFraction);
                val = m_builder.CreateShl(val, shiftVal, "scale_up");
            }
            else if (valFraction > targetFraction)
            {
                llvm::Value* shiftVal = llvm::ConstantInt::get(val->getType(), valFraction - targetFraction);
                val = m_builder.CreateAShr(val, shiftVal, "scale_down");
            }
        }

        if (val->getType() != targetType)
            val = m_builder.CreateIntCast(val, targetType, true);

        return val;
    }

    return val;
}

} // namespace coral66::cgllvm
