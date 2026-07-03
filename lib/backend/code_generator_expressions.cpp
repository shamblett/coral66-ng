/*
 * Project: coral66
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

llvm::Value* code_generator::emitAddress(const syntax::Expression& expression)
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

        if (!baseAddr)
            return nullptr;

        llvm::Type* baseType = nullptr;
        const syntax::ArrayDeclaration* arrDeclNode = nullptr;
        declaration->match(
            [&](const semantics::ArrayDeclaration& arrayDeclaration)
            {
                baseType = convertType(arrayDeclaration.getType());
                arrDeclNode = dynamic_cast<const syntax::ArrayDeclaration*>(arrayDeclaration.getAstNode());
            },
            [&](const semantics::VariableDeclaration& varDecl)
            {
                baseType = convertType(varDecl.getType());
            },
            [&](const auto&)
            {
            });

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
                auto lower = semantics::evaluateConstantInteger(*arrDeclNode->getDimensions()[i].first);
                if (lower && *lower != 0)
                {
                    llvm::Value* lowerVal = llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_module.getContext()), *lower, true);
                    idxVal = m_builder.CreateSub(idxVal, lowerVal, "idx_offset");
                }
            }

            indices.push_back(idxVal);
        }

        return m_builder.CreateGEP(baseType, baseAddr, indices, "arrayidx");
    }
    return nullptr;
}

llvm::Value* code_generator::visit(const syntax::Expression& expression)
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
    if (auto* e = dynamic_cast<const syntax::TypeChangeExpression*>(&expression))
        return visit(*e);
    return nullptr;
}

// ReSharper disable once CppMemberFunctionMayBeConst
llvm::Value* code_generator::visit(const syntax::LiteralExpression& expression)
{
    auto& val = expression.getLiteral()->getValue();
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

llvm::Value* code_generator::visit(const syntax::IdentifierExpression& expression)
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
                return m_builder.CreateCall(ft, funcVal, {});
            return nullptr;
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
    return m_builder.CreateLoad(type, addr);
}

llvm::Value* code_generator::visit(const syntax::BinaryExpression& expression)
{
    llvm::Value* L = visit(expression.getLeft());
    llvm::Value* R = visit(expression.getRight());
    if (!L || !R)
        return nullptr;

    const auto itL = m_program.getContext().expressionTypes.find(&expression.getLeft());
    const auto itR = m_program.getContext().expressionTypes.find(&expression.getRight());

    std::uint32_t leftFraction = 0;
    std::uint32_t rightFraction = 0;
    bool isLeftFixed = false;
    bool isRightFixed = false;

    if (itL != m_program.getContext().expressionTypes.end()) {
        if (auto* fixedL = itL->second->tryAs<semantics::FixedType>()) {
            leftFraction = fixedL->getFractionalBits();
            isLeftFixed = true;
        }
    }

    if (itR != m_program.getContext().expressionTypes.end()) {
        if (auto* fixedR = itR->second->tryAs<semantics::FixedType>()) {
            rightFraction = fixedR->getFractionalBits();
            isRightFixed = true;
        }
    }

    if (L->getType() != R->getType())
    {
        if (L->getType()->isIntegerTy() && R->getType()->isFloatingPointTy())
            L = m_builder.CreateSIToFP(L, R->getType());
        else if (L->getType()->isFloatingPointTy() && R->getType()->isIntegerTy())
            R = m_builder.CreateSIToFP(R, L->getType());
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

    const auto kind = expression.getOp()->getTokenType();

    if (L->getType()->isIntegerTy() && (isLeftFixed || isRightFixed))
    {
        if (kind == lexer::TokenType::Plus || kind == lexer::TokenType::Minus ||
            kind == lexer::TokenType::LessThan || kind == lexer::TokenType::LessThanOrEqual ||
            kind == lexer::TokenType::Equal || kind == lexer::TokenType::GreaterThanOrEqual ||
            kind == lexer::TokenType::GreaterThan || kind == lexer::TokenType::NotEqual)
        {
            if (leftFraction > rightFraction)
            {
                llvm::Value* shiftVal = llvm::ConstantInt::get(R->getType(), leftFraction - rightFraction);
                R = m_builder.CreateShl(R, shiftVal, "align_fraction");
            }
            else if (rightFraction > leftFraction)
            {
                llvm::Value* shiftVal = llvm::ConstantInt::get(L->getType(), rightFraction - leftFraction);
                L = m_builder.CreateShl(L, shiftVal, "align_fraction");
            }
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
        else
        {
            llvm::Value* mul = m_builder.CreateMul(L, R);
            if (isLeftFixed || isRightFixed)
            {
                std::uint32_t targetFraction = std::max(leftFraction, rightFraction);
                std::uint32_t currentFraction = leftFraction + rightFraction;
                if (currentFraction > targetFraction)
                {
                    llvm::Value* shiftVal = llvm::ConstantInt::get(mul->getType(), currentFraction - targetFraction);
                    return m_builder.CreateAShr(mul, shiftVal, "scale_mul");
                }
                else if (currentFraction < targetFraction)
                {
                    llvm::Value* shiftVal = llvm::ConstantInt::get(mul->getType(), targetFraction - currentFraction);
                    return m_builder.CreateShl(mul, shiftVal, "scale_mul");
                }
            }
            return mul;
        }
    }
    if (kind == lexer::TokenType::Division)
    {
        if (L->getType()->isFloatingPointTy())
            return m_builder.CreateFDiv(L, R);
        else
        {
            if (isLeftFixed || isRightFixed)
            {
                std::uint32_t targetFraction = std::max(leftFraction, rightFraction);
                std::uint32_t shiftAmt = targetFraction + rightFraction - leftFraction;
                if (shiftAmt > 0)
                {
                    llvm::Value* shiftVal = llvm::ConstantInt::get(L->getType(), shiftAmt);
                    L = m_builder.CreateShl(L, shiftVal, "scale_div_num");
                }
            }
            return m_builder.CreateSDiv(L, R);
        }
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

llvm::Value* code_generator::visit(const syntax::UnaryExpression& expression)
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

llvm::Value* code_generator::visit(const syntax::FunctionCallExpression& expression)
{
    const auto it = m_program.getContext().resolvedIdentifiers.find(&expression);
    if (it == m_program.getContext().resolvedIdentifiers.end())
        return nullptr;

    const auto* declaration = it->second;
    llvm::Value* funcVal = m_globals.contains(declaration) ? m_globals[declaration] : nullptr;
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
        else
        {
            args.push_back(visit(*arg));
        }
        i++;
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
        return m_builder.CreateCall(ft, funcVal, args);
    }
    return nullptr;
}

llvm::Value* code_generator::visit(const syntax::PartWordExpression& expression)
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

llvm::Value* code_generator::visit(const syntax::ArrayAccessExpression& expression)
{
    llvm::Value* addr = emitAddress(expression);
    if (!addr)
        return nullptr;

    const auto rit = m_program.getContext().resolvedIdentifiers.find(&expression);
    llvm::Type* type = llvm::Type::getInt32Ty(m_module.getContext());
    if (rit != m_program.getContext().resolvedIdentifiers.end())
    {
        if (auto* arrayDeclaration = rit->second->tryAs<semantics::ArrayDeclaration>())
        {
            type = convertType(arrayDeclaration->getType().tryAs<semantics::ArrayType>()->getElementType());
        }
    }
    return m_builder.CreateLoad(type, addr, "arrayload");
}

llvm::Value* code_generator::visit(const syntax::TypeChangeExpression& expression)
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
    const auto exprIt = m_program.getContext().expressionTypes.find(&expression.getExpression());
    if (exprIt != m_program.getContext().expressionTypes.end())
    {
        if (auto* fixedV = exprIt->second->tryAs<semantics::FixedType>())
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
            llvm::Value* multiplier = llvm::ConstantFP::get(val->getType(), static_cast<double>(1ULL << targetFraction));
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
