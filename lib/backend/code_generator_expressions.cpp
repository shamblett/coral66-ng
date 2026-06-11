/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "code_generator.hpp"

#include "lib/frontend/semantics/program.hpp"

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

        std::vector<llvm::Value*> indices;
        indices.push_back(llvm::ConstantInt::get(m_module.getContext(), llvm::APInt(32, 0)));
        for (const auto& idxExpr : arrAccess->getIndices())
        {
            llvm::Value* idxVal = visit(*idxExpr);
            indices.push_back(idxVal);
        }

        llvm::Type* baseType = nullptr;
        declaration->match([&](const semantics::ArrayDeclaration& arrayDeclaration)
                           { baseType = convertType(arrayDeclaration.getType()); }, [&](const auto&) {});

        if (baseType)
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
            // If they are both floats but different precision, cast the smaller to the larger
            if (L->getType()->getFPMantissaWidth() > R->getType()->getFPMantissaWidth())
                R = m_builder.CreateFPCast(R, L->getType());
            else
                L = m_builder.CreateFPCast(L, R->getType());
        }
    }

    const auto kind = expression.getOp()->getTokenType();
    if (kind == lexer::TokenType::Plus)
        return L->getType()->isFloatingPointTy() ? m_builder.CreateFAdd(L, R) : m_builder.CreateAdd(L, R);
    if (kind == lexer::TokenType::Minus)
        return L->getType()->isFloatingPointTy() ? m_builder.CreateFSub(L, R) : m_builder.CreateSub(L, R);
    if (kind == lexer::TokenType::Asterisk)
        return L->getType()->isFloatingPointTy() ? m_builder.CreateFMul(L, R) : m_builder.CreateMul(L, R);
    if (kind == lexer::TokenType::Division)
        return L->getType()->isFloatingPointTy() ? m_builder.CreateFDiv(L, R) : m_builder.CreateSDiv(L, R);
    if (kind == lexer::TokenType::Equal)
        return m_builder.CreateICmpEQ(L, R);
    if (kind == lexer::TokenType::NotEqual)
        return m_builder.CreateICmpNE(L, R);
    if (kind == lexer::TokenType::LessThan)
        return m_builder.CreateICmpSLT(L, R);
    if (kind == lexer::TokenType::LessThanOrEqual)
        return m_builder.CreateICmpSLE(L, R);
    if (kind == lexer::TokenType::GreaterThan)
        return m_builder.CreateICmpSGT(L, R);
    if (kind == lexer::TokenType::GreaterThanOrEqual)
        return m_builder.CreateICmpSGE(L, R);
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
        return m_builder.CreateNeg(operand);
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
        if (procType && i < procType->getParameters().size() && procType->getParameters()[i].type->isLocation())
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
                if (args[i1]->getType()->isPointerTy() && ft->getParamType(i1)->isIntegerTy())
                    args[i1] = m_builder.CreatePtrToInt(args[i1], ft->getParamType(i1));
                else if (args[i1]->getType()->isIntegerTy() && ft->getParamType(i1)->isPointerTy())
                    args[i1] = m_builder.CreateIntToPtr(args[i1], ft->getParamType(i1));
                else
                    args[i1] = m_builder.CreateIntCast(args[i1], ft->getParamType(i1), true);
            }
        }
        return m_builder.CreateCall(ft, funcVal, args);
    }
    return nullptr;
}

llvm::Value* code_generator::visit(const syntax::PartWordExpression& expression)
{
    return nullptr;
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
    if (val->getType() == targetType)
        return val;

    if (targetType->isFloatingPointTy() && val->getType()->isIntegerTy())
        return m_builder.CreateSIToFP(val, targetType);
    if (targetType->isIntegerTy() && val->getType()->isFloatingPointTy())
        return m_builder.CreateFPToSI(val, targetType);
    if (targetType->isFloatingPointTy() && val->getType()->isFloatingPointTy())
        return m_builder.CreateFPCast(val, targetType);
    if (targetType->isIntegerTy() && val->getType()->isIntegerTy())
        return m_builder.CreateIntCast(val, targetType, true);

    return val;
}

} // namespace coral66::cgllvm
