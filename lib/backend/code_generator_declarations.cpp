/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "code_generator.hpp"

#include "lib/frontend/semantics/program.hpp"

namespace coral66::cgllvm
{

llvm::Constant* CodeGenerator::parsePresetList(const std::vector<lexer::CTokenIterator>& tokens, llvm::Type* targetType,
                                               const std::size_t numElements)
{
    std::vector<llvm::Constant*> elements;
    llvm::Type* elemType = nullptr;
    if (const auto* arrayType = llvm::dyn_cast<llvm::ArrayType>(targetType))
        elemType = arrayType->getElementType();
    else
        elemType = targetType; // Not really expected for arrays/tables, but safe fallback

    std::size_t currentElement = 0;

    for (std::size_t i = 0; i < tokens.size() && currentElement < numElements; ++i)
    {
        auto tType = tokens[i]->getTokenType();
        if (tType == lexer::TokenType::Comma)
        {
            // Void item if it's consecutive commas or leading comma
            if (i == 0 || tokens[i - 1]->getTokenType() == lexer::TokenType::Comma)
            {
                elements.push_back(llvm::Constant::getNullValue(elemType));
                currentElement++;
            }
            continue;
        }

        bool isNegative = false;
        if (tType == lexer::TokenType::Minus || tType == lexer::TokenType::Plus)
        {
            isNegative = (tType == lexer::TokenType::Minus);
            i++;
            if (i >= tokens.size())
                break;
            tType = tokens[i]->getTokenType();
        }

        if (tType == lexer::TokenType::Literal
            && (tokens[i]->getType() == lexer::CToken::Type::Integer
                || tokens[i]->getType() == lexer::CToken::Type::Floating))
        {
            llvm::Constant* val = nullptr;
            if (auto* intVal = std::get_if<llvm::APSInt>(&tokens[i]->getValue()))
            {
                std::int64_t v = intVal->getExtValue();
                if (isNegative)
                    v = -v;
                if (elemType->isFloatingPointTy())
                    val = llvm::ConstantFP::get(elemType, static_cast<double>(v));
                else
                    val = llvm::ConstantInt::get(elemType, v, true);
            }
            else if (auto* floatVal = std::get_if<llvm::APFloat>(&tokens[i]->getValue()))
            {
                double v = floatVal->convertToDouble();
                if (isNegative)
                    v = -v;
                if (elemType->isFloatingPointTy())
                    val = llvm::ConstantFP::get(elemType, v);
                else
                    val = llvm::ConstantInt::get(elemType, static_cast<std::int64_t>(v), true);
            }

            if (val)
            {
                elements.push_back(val);
                currentElement++;
            }
        }
    }

    // Fill remaining elements with zero
    while (currentElement < numElements)
    {
        elements.push_back(llvm::Constant::getNullValue(elemType));
        currentElement++;
    }

    if (auto* arrType = llvm::dyn_cast<llvm::ArrayType>(targetType))
    {
        return llvm::ConstantArray::get(arrType, elements);
    }

    return elements.empty() ? llvm::Constant::getNullValue(targetType) : elements[0];
}

void CodeGenerator::visit(const semantics::Declaration& declaration)
{
    // Only process global declarations here.

    if (auto* var = dynamic_cast<const semantics::VariableDeclaration*>(&declaration))
    {
        auto* llvmType = convertType(var->getType());
        llvm::Constant* initializer = llvm::Constant::getNullValue(llvmType);

        if (auto* astNode = dynamic_cast<const syntax::VariableDeclaration*>(var->getAstNode()))
        {
            if (astNode->hasPreset() && !astNode->getPresetTokens().empty())
            {
                initializer = parsePresetList(astNode->getPresetTokens(), llvmType, 1);
            }
        }

        auto* globalVar = new llvm::GlobalVariable(m_module, llvmType, false, llvm::GlobalValue::ExternalLinkage,
                                                   initializer, var->getName());
        m_globals[var] = globalVar;
    }
    else if (auto* arr = dynamic_cast<const semantics::ArrayDeclaration*>(&declaration))
    {
        auto* llvmType = convertType(arr->getType());
        llvm::Constant* initializer = llvm::Constant::getNullValue(llvmType);

        if (auto* astNode = dynamic_cast<const syntax::ArrayDeclaration*>(arr->getAstNode()))
        {
            if (astNode->hasPreset() && !astNode->getPresetTokens().empty())
            {
                std::size_t numElements = 1;
                if (const auto* arrayType = llvm::dyn_cast<llvm::ArrayType>(llvmType))
                    numElements = arrayType->getNumElements();
                initializer = parsePresetList(astNode->getPresetTokens(), llvmType, numElements);
            }
        }

        auto* globalVar = new llvm::GlobalVariable(m_module, llvmType, false, llvm::GlobalValue::ExternalLinkage,
                                                   initializer, arr->getName());
        m_globals[arr] = globalVar;
    }
    else if (auto* tab = dynamic_cast<const semantics::TableDeclaration*>(&declaration))
    {
        auto* llvmType = convertType(tab->getType());
        llvm::Constant* initializer = llvm::Constant::getNullValue(llvmType);

        if (auto* astNode = dynamic_cast<const syntax::TableDeclaration*>(tab->getAstNode()))
        {
            if (astNode->hasPreset() && !astNode->getPresetTokens().empty())
            {
                std::size_t numElements = 1;
                if (const auto* arrayType = llvm::dyn_cast<llvm::ArrayType>(llvmType))
                    numElements = arrayType->getNumElements();
                initializer = parsePresetList(astNode->getPresetTokens(), llvmType, numElements);
            }
        }

        auto* globalVar = new llvm::GlobalVariable(m_module, llvmType, false, llvm::GlobalValue::ExternalLinkage,
                                                   initializer, tab->getName());
        m_globals[tab] = globalVar;
    }
    else if (auto* over = dynamic_cast<const semantics::OverlayDeclaration*>(&declaration))
    {
        if (auto* astNode = dynamic_cast<const syntax::OverlayDeclaration*>(over->getAstNode()))
        {
            const auto baseName = std::get<std::string>(astNode->getBase()->getValue());
            if (const semantics::Usable* baseUsable = m_program.getContext().symbolTable.lookup(baseName);
                baseUsable && m_globals.contains(baseUsable))
            {
                llvm::Constant* baseAddr = llvm::cast<llvm::Constant>(m_globals[baseUsable]);
                std::int64_t offsetVal = 0;
                if (astNode->getOffset()->getType() == lexer::CToken::Integer)
                {
                    if (auto* intVal = std::get_if<llvm::APSInt>(&astNode->getOffset()->getValue()))
                        offsetVal = intVal->getExtValue();
                }

                llvm::Constant* finalAddr = baseAddr;
                if (offsetVal != 0)
                {
                    llvm::Type* i32Ty = llvm::Type::getInt32Ty(m_module.getContext());
                    llvm::Constant* idx = llvm::ConstantInt::get(i32Ty, offsetVal, true);
                    finalAddr = llvm::ConstantExpr::getGetElementPtr(i32Ty, baseAddr, idx);
                }
                m_globals[over] = finalAddr;
            }
        }
    }
    else if (auto* proc = dynamic_cast<const semantics::ProcedureDeclaration*>(&declaration))
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

void CodeGenerator::generateProcedure(const semantics::ProcedureDeclaration& procedureDeclaration,
                                      const syntax::ProcedureDeclaration& astNode)
{
    m_currentProcedure = &astNode;
    m_labels.clear();
    m_namedLabels.clear();
    m_locals.clear();

    auto* f = llvm::cast<llvm::Function>(m_globals[&procedureDeclaration]);
    m_currentFunction = f;

    llvm::BasicBlock* bb = llvm::BasicBlock::Create(m_module.getContext(), "entry", f);
    m_builder.SetInsertPoint(bb);

    if (const auto* procedureType = procedureDeclaration.getType().tryAs<semantics::ProcedureType>())
    {
        size_t argIdx = 0;
        for (const auto& param : procedureType->getParameters())
        {
            llvm::Argument* arg = f->getArg(argIdx);
            arg->setName(std::string(param.name) + "_arg");

            const semantics::Usable* parameterDeclaration = nullptr;
            for (const auto& usable : m_program.getGlobals())
            {
                if (const auto* variableDeclaration = usable->tryAs<semantics::VariableDeclaration>())
                {
                    if (variableDeclaration->getName() == param.name)
                    {
                        bool found = false;
                        for (const auto& p : astNode.getParameters())
                        {
                            if (variableDeclaration->getAstNode() == p.get())
                            {
                                found = true;
                                break;
                            }
                        }
                        if (found)
                        {
                            parameterDeclaration = usable.get();
                            break;
                        }
                    }
                }
            }

            if (parameterDeclaration)
            {
                if (llvm::Type* llvmType = convertType(*param.type);
                    param.type->isLocation() || param.type->is<semantics::ArrayType>())
                {
                    m_locals[parameterDeclaration] = arg;
                }
                else
                {
                    llvm::AllocaInst* alloca = m_builder.CreateAlloca(llvmType, nullptr, param.name);
                    m_locals[parameterDeclaration] = alloca;
                    m_builder.CreateStore(arg, alloca);
                }
            }
            argIdx++;
        }
    }

    // Generate body
    if constexpr (true) // astNode.getBody() always returns a reference
    {
        preScanLabels(astNode.getBody());
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

void CodeGenerator::visit(const syntax::Block& block)
{
    for (const auto& declaration : block.getDeclarations())
    {
        visit(*declaration);
    }
    for (const auto& stmt : block.getStatements())
    {
        visit(*stmt);
    }
}

void CodeGenerator::visit(const syntax::Declaration& declaration)
{
    if (auto* var = dynamic_cast<const syntax::VariableDeclaration*>(&declaration))
        visit(*var);
    else if (auto* arr = dynamic_cast<const syntax::ArrayDeclaration*>(&declaration))
        visit(*arr);
    else if (auto* proc = dynamic_cast<const syntax::ProcedureDeclaration*>(&declaration))
        visit(*proc);
    else if (auto* tab = dynamic_cast<const syntax::TableDeclaration*>(&declaration))
        visit(*tab);
    else if (auto* over = dynamic_cast<const syntax::OverlayDeclaration*>(&declaration))
        visit(*over);
}

void CodeGenerator::visit(const syntax::VariableDeclaration& declaration)
{
    if (const auto it = m_program.getContext().resolvedIdentifiers.find(&declaration);
        it != m_program.getContext().resolvedIdentifiers.end())
    {
        if (auto* semVar = it->second->tryAs<semantics::VariableDeclaration>())
        {
            auto* llvmType = convertType(semVar->getType());
            auto* alloca = m_builder.CreateAlloca(llvmType, nullptr, semVar->getName());
            m_locals[it->second] = alloca;
        }
    }
}

void CodeGenerator::visit(const syntax::ArrayDeclaration& declaration)
{
    if (const auto it = m_program.getContext().resolvedIdentifiers.find(&declaration);
        it != m_program.getContext().resolvedIdentifiers.end())
    {
        if (auto* semArr = it->second->tryAs<semantics::ArrayDeclaration>())
        {
            auto* llvmType = convertType(semArr->getType());
            auto* alloca = m_builder.CreateAlloca(llvmType, nullptr, semArr->getName());
            m_locals[it->second] = alloca;
        }
    }
}

void CodeGenerator::visit(const syntax::TableDeclaration& declaration)
{
    if (const auto it = m_program.getContext().resolvedIdentifiers.find(&declaration);
        it != m_program.getContext().resolvedIdentifiers.end())
    {
        if (auto* semTab = it->second->tryAs<semantics::TableDeclaration>())
        {
            auto* llvmType = convertType(semTab->getType());
            auto* alloca = m_builder.CreateAlloca(llvmType, nullptr, semTab->getName());
            m_locals[it->second] = alloca;
        }
    }
}

void CodeGenerator::visit(const syntax::OverlayDeclaration& declaration)
{
    const auto baseName = std::get<std::string>(declaration.getBase()->getValue());
    const semantics::Usable* baseUsable = m_program.getContext().symbolTable.lookup(baseName);
    if (!baseUsable)
        return;

    llvm::Value* baseAddr = nullptr;
    if (m_locals.contains(baseUsable))
        baseAddr = m_locals[baseUsable];
    else if (m_globals.contains(baseUsable))
        baseAddr = m_globals[baseUsable];

    if (!baseAddr)
        return;

    std::int64_t offsetVal = 0;
    if (declaration.getOffset()->getType() == lexer::CToken::Integer)
    {
        if (auto* intVal = std::get_if<llvm::APSInt>(&declaration.getOffset()->getValue()))
            offsetVal = intVal->getExtValue();
    }

    // Coral 66 OVERLAY array base offsets are usually 1 word (4 bytes) each, but depend on type.
    // Actually we just get the base address, GEP if offset != 0, and bitcast.
    llvm::Value* finalAddr = baseAddr;
    if (offsetVal != 0)
    {
        llvm::Type* i32Ty = llvm::Type::getInt32Ty(m_module.getContext());
        llvm::Value* idx = llvm::ConstantInt::get(i32Ty, offsetVal, true);
        finalAddr = m_builder.CreateGEP(i32Ty, baseAddr, idx, "overlay_offset");
    }

    const syntax::Declaration& innerDecl = declaration.getOverlayDeclaration();
    std::string targetName;
    if (auto* varDecl = dynamic_cast<const syntax::VariableDeclaration*>(&innerDecl))
    {
        if (!varDecl->getIdentifiers().empty())
            targetName = std::get<std::string>(varDecl->getIdentifiers()[0]->getValue());
    }
    else if (auto* arrDecl = dynamic_cast<const syntax::ArrayDeclaration*>(&innerDecl))
    {
        if (!arrDecl->getIdentifiers().empty())
            targetName = std::get<std::string>(arrDecl->getIdentifiers()[0]->getValue());
    }

    if (!targetName.empty())
    {
        if (const semantics::Usable* targetUsable = m_program.getContext().symbolTable.lookup(targetName))
        {
            m_locals[targetUsable] = finalAddr;
        }
    }
}

void CodeGenerator::visit(const syntax::ProcedureDeclaration& declaration)
{
    // Nested procedures can be handled here if needed.
    // For Phase 1, we only need top-level procedures.
}

} // namespace coral66::cgllvm
