/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "code_generator.hpp"

#include <cmath>

#include "lib/frontend/semantics/program.hpp"

namespace coral66::cgllvm
{

llvm::Constant* CodeGenerator::reshapeArray(llvm::Type* targetType, const std::vector<llvm::Constant*>& flatElements, std::size_t& offset)
{
    if (auto* arrayType = llvm::dyn_cast<llvm::ArrayType>(targetType))
    {
        std::vector<llvm::Constant*> subElements;
        std::size_t numElements = arrayType->getNumElements();
        llvm::Type* elementType = arrayType->getElementType();
        for (std::size_t i = 0; i < numElements; ++i)
        {
            subElements.push_back(reshapeArray(elementType, flatElements, offset));
        }
        return llvm::ConstantArray::get(arrayType, subElements);
    }
    
    // Base case: primitive type
    if (offset < flatElements.size())
    {
        return flatElements[offset++];
    }
    
    return llvm::Constant::getNullValue(targetType);
}

llvm::Constant* CodeGenerator::parsePresetList(const std::vector<lexer::CTokenIterator>& tokens, llvm::Type* targetType,
                                               const std::size_t numElements, const std::size_t startElementIndex,
                                               const semantics::Type* semanticType,
                                               const syntax::TableDeclaration* astTable)
{
    std::vector<llvm::Constant*> elements;
    llvm::Type* elemType = targetType;
    while (const auto* arrayType = llvm::dyn_cast<llvm::ArrayType>(elemType))
    {
        elemType = arrayType->getElementType();
    }

    std::uint8_t fractionBits = 0;
    if (semanticType)
    {
        const semantics::Type* st = semanticType;
        while (const auto* arr = st->tryAs<semantics::ArrayType>())
            st = &arr->getElementType();
        
        if (const auto* fixed = st->tryAs<semantics::FixedType>())
            fractionBits = fixed->getFractionalBits();
    }


    std::size_t currentElement = 0;
    
    // Support for table element packing
    std::size_t tableElementIndex = 0;
    std::size_t tableFields = 1;
    if (astTable)
        tableFields = astTable->getElements().size();
    
    std::vector<std::int64_t> currentTableWord(startElementIndex + numElements, 0);
    std::vector<bool> currentTableWordSet(startElementIndex + numElements, false);

    for (std::size_t i = 0; i < tokens.size() && currentElement < (startElementIndex + numElements); ++i)
    {
        auto tType = tokens[i]->getTokenType();
        
        // Skip parentheses as they are just grouping
        if (tType == lexer::TokenType::OpenParentheses || tType == lexer::TokenType::CloseParentheses)
            continue;

        if (tType == lexer::TokenType::Comma)
        {
            // Void item if it's consecutive commas or leading comma or comma after parenthesis
            // Basically if the previous non-parenthesis token was a comma (or there was no previous)
            bool isVoid = false;
            if (i == 0) isVoid = true;
            else {
                for (std::int64_t j = i - 1; j >= 0; --j) {
                    auto prevType = tokens[j]->getTokenType();
                    if (prevType == lexer::TokenType::OpenParentheses || prevType == lexer::TokenType::CloseParentheses)
                        continue;
                    if (prevType == lexer::TokenType::Comma)
                        isVoid = true;
                    break; // Stop at first non-parenthesis token
                }
            }

            if (isVoid)
            {
                if (currentElement >= startElementIndex)
                {
                    if (astTable)
                    {
                        // Just advance the field index for tables, the default 0 will be left un-ORed
                    }
                    else
                    {
                        elements.push_back(llvm::Constant::getNullValue(elemType));
                    }
                }
                if (astTable)
                {
                    tableElementIndex++;
                    if (tableElementIndex >= tableFields)
                    {
                        tableElementIndex = 0;
                        currentElement++;
                    }
                }
                else
                {
                    currentElement++;
                }
            }
            continue;
        }

        bool isNegative = false;
        if (tType == lexer::TokenType::Minus || tType == lexer::TokenType::Plus)
        {
            isNegative = tType == lexer::TokenType::Minus;
            i++;
            if (i >= tokens.size())
                break;
            tType = tokens[i]->getTokenType();
        }

        bool isOctal = false;
        if (tType == lexer::TokenType::OctalKeyword)
        {
            isOctal = true;
            i++;
            if (i < tokens.size() && tokens[i]->getTokenType() == lexer::TokenType::OpenParentheses)
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
            std::int64_t rawIntVal = 0;
            
            // Adjust fractionBits if we are packing table fields
            std::uint8_t localFractionBits = fractionBits;
            const syntax::TableElementDeclaration* elemDecl = nullptr;
            if (astTable)
            {
                elemDecl = dynamic_cast<const syntax::TableElementDeclaration*>(astTable->getElements()[tableElementIndex].get());
                if (elemDecl && elemDecl->getFixedFractionBits())
                {
                    // No access to evaluateConstantInteger in backend, but these are typically literals
                    if (auto* lit = dynamic_cast<const syntax::LiteralExpression*>(elemDecl->getFixedFractionBits()))
                    {
                        if (auto* intVal = std::get_if<llvm::APSInt>(&lit->getLiteral()->getValue()))
                        {
                            if (intVal->getExtValue() > 0)
                                localFractionBits = static_cast<std::uint8_t>(intVal->getExtValue());
                        }
                    }
                }
            }

            if (auto* intVal = std::get_if<llvm::APSInt>(&tokens[i]->getValue()))
            {
                llvm::APSInt vInt = *intVal;
                if (isOctal)
                {
                    llvm::SmallString<32> str;
                    vInt.toString(str, 10);
                    llvm::APInt octalVal;
                    llvm::StringRef(str).getAsInteger(8, octalVal);
                    octalVal = octalVal.zextOrTrunc(vInt.getBitWidth());
                    vInt = llvm::APSInt(octalVal, vInt.isUnsigned());
                }
                std::int64_t v = vInt.getExtValue();
                if (isNegative)
                    v = -v;
                
                if (elemType->isFloatingPointTy() && !astTable)
                {
                    val = llvm::ConstantFP::get(elemType, static_cast<double>(v));
                }
                else
                {
                    if (localFractionBits > 0)
                        v = v << localFractionBits;
                    rawIntVal = v;
                    val = llvm::ConstantInt::get(elemType, v, true);
                }
            }
            else if (auto* floatVal = std::get_if<llvm::APFloat>(&tokens[i]->getValue()))
            {
                // We do not handle octal floating point correctly yet, but we will pass it through
                double v = floatVal->convertToDouble();
                if (isNegative)
                    v = -v;
                if (elemType->isFloatingPointTy() && !astTable)
                {
                    val = llvm::ConstantFP::get(elemType, v);
                }
                else
                {
                    if (localFractionBits > 0)
                        v *= std::pow(2.0, localFractionBits);
                    rawIntVal = static_cast<std::int64_t>(std::round(v));
                    val = llvm::ConstantInt::get(elemType, rawIntVal, true);
                }
            }

            if (val)
            {
                if (currentElement >= startElementIndex)
                {
                    if (astTable)
                    {
                        std::int64_t bitOffset = 0;
                        if (elemDecl && elemDecl->getBitPosition())
                        {
                            if (auto* bPos = std::get_if<llvm::APSInt>(&elemDecl->getBitPosition().value()->getValue()))
                                bitOffset = bPos->getExtValue();
                        }

                        currentTableWord[currentElement] |= (rawIntVal << bitOffset);
                        currentTableWordSet[currentElement] = true;
                    }
                    else
                    {
                        elements.push_back(val);
                    }
                }
                
                if (astTable)
                {
                    tableElementIndex++;
                    if (tableElementIndex >= tableFields)
                    {
                        tableElementIndex = 0;
                        currentElement++;
                    }
                }
                else
                {
                    currentElement++;
                }
            }
            
            // If it was an OCTAL expression, we should skip the closing parenthesis
            if (isOctal)
            {
                if (i + 1 < tokens.size() && tokens[i + 1]->getTokenType() == lexer::TokenType::CloseParentheses)
                    i++;
            }
        }
    }
    
    // Process any trailing void if the last non-parenthesis token was a comma
    if (!tokens.empty())
    {
        bool trailingVoid = false;
        for (std::int64_t i = tokens.size() - 1; i >= 0; --i)
        {
            auto type = tokens[i]->getTokenType();
            if (type == lexer::TokenType::OpenParentheses || type == lexer::TokenType::CloseParentheses)
                continue;
            if (type == lexer::TokenType::Comma)
                trailingVoid = true;
            break;
        }
        
        if (trailingVoid && currentElement < (startElementIndex + numElements))
        {
            if (currentElement >= startElementIndex)
            {
                if (astTable)
                {
                    // advance field
                }
                else
                {
                    elements.push_back(llvm::Constant::getNullValue(elemType));
                }
            }
            if (astTable)
            {
                tableElementIndex++;
                if (tableElementIndex >= tableFields)
                {
                    tableElementIndex = 0;
                    currentElement++;
                }
            }
            else
            {
                currentElement++;
            }
        }
    }

    if (astTable)
    {
        for (std::size_t i = 0; i < currentElement; ++i)
        {
            if (i >= startElementIndex && currentTableWordSet[i])
            {
                elements.push_back(llvm::ConstantInt::get(elemType, currentTableWord[i], true));
            }
            else if (i >= startElementIndex)
            {
                elements.push_back(llvm::Constant::getNullValue(elemType));
            }
        }
    }

    // Fill remaining elements with zero
    while (elements.size() < numElements)
    {
        elements.push_back(llvm::Constant::getNullValue(elemType));
    }

    if (llvm::isa<llvm::ArrayType>(targetType))
    {
        std::size_t offset = 0;
        return reshapeArray(targetType, elements, offset);
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
        bool hasExplicitPreset = false;

        if (auto* astNode = dynamic_cast<const syntax::VariableDeclaration*>(var->getAstNode()))
        {
            if (astNode->hasPreset() && !astNode->getPresetTokens().empty())
            {
                initializer = parsePresetList(astNode->getPresetTokens(), llvmType, 1, var->getPresetIndex(), &var->getType());
                hasExplicitPreset = true;
            }
        }

        if (m_options.baremetal && var->getAbsoluteAddress().has_value())
        {
            llvm::Constant* addrInt =
                llvm::ConstantInt::get(m_module.getContext(), llvm::APInt(64, var->getAbsoluteAddress().value()));
            llvm::Constant* ptr =
                llvm::ConstantExpr::getIntToPtr(addrInt, llvm::PointerType::getUnqual(m_module.getContext()));
            m_globals[var] = ptr;
        }
        else
        {
            m_module.getOrInsertGlobal(var->getMangledName(), llvmType);
            llvm::GlobalVariable* globalVar = m_module.getNamedGlobal(var->getMangledName());
            if (declaration.getCommunicatorType() == semantics::CommunicatorType::External ||
                declaration.getCommunicatorType() == semantics::CommunicatorType::Library)
            {
                globalVar->setLinkage(llvm::GlobalValue::ExternalLinkage);
            }
            else if (declaration.getCommunicatorType() == semantics::CommunicatorType::Common)
            {
                if (hasExplicitPreset)
                {
                    globalVar->setLinkage(llvm::GlobalValue::ExternalLinkage);
                    if (!globalVar->hasInitializer())
                        globalVar->setInitializer(initializer);
                }
                else
                {
                    globalVar->setLinkage(llvm::GlobalValue::CommonLinkage);
                    if (!globalVar->hasInitializer())
                        globalVar->setInitializer(initializer);
                }
            }
            else
            {
                globalVar->setLinkage(declaration.getScopeDepth() > 3 ? llvm::GlobalValue::InternalLinkage :
                                                                        llvm::GlobalValue::ExternalLinkage);
                if (!globalVar->hasInitializer())
                {
                    globalVar->setInitializer(initializer);
                }
            }
            m_globals[var] = globalVar;
        }
    }
    else if (auto* arr = dynamic_cast<const semantics::ArrayDeclaration*>(&declaration))
    {
        auto* llvmType = convertType(arr->getType());
        llvm::Constant* initializer = llvm::Constant::getNullValue(llvmType);
        bool hasExplicitPreset = false;

        if (auto* astNode = dynamic_cast<const syntax::ArrayDeclaration*>(arr->getAstNode()))
        {
            if (astNode->hasPreset() && !astNode->getPresetTokens().empty())
            {
                std::size_t numElements = 1;
                for (auto dim : arr->getType().as<semantics::ArrayType>().getDimensions())
                    numElements *= dim;
                initializer = parsePresetList(astNode->getPresetTokens(), llvmType, numElements, arr->getPresetIndex(), &arr->getType());
                hasExplicitPreset = true;
            }
        }

        if (m_options.baremetal && arr->getAbsoluteAddress().has_value())
        {
            llvm::Constant* addrInt =
                llvm::ConstantInt::get(m_module.getContext(), llvm::APInt(64, arr->getAbsoluteAddress().value()));
            llvm::Constant* ptr =
                llvm::ConstantExpr::getIntToPtr(addrInt, llvm::PointerType::getUnqual(m_module.getContext()));
            m_globals[arr] = ptr;
        }
        else
        {
            m_module.getOrInsertGlobal(arr->getMangledName(), llvmType);
            llvm::GlobalVariable* globalVar = m_module.getNamedGlobal(arr->getMangledName());
            if (declaration.getCommunicatorType() == semantics::CommunicatorType::External ||
                declaration.getCommunicatorType() == semantics::CommunicatorType::Library)
            {
                globalVar->setLinkage(llvm::GlobalValue::ExternalLinkage);
            }
            else if (declaration.getCommunicatorType() == semantics::CommunicatorType::Common)
            {
                if (hasExplicitPreset)
                {
                    globalVar->setLinkage(llvm::GlobalValue::ExternalLinkage);
                    if (!globalVar->hasInitializer())
                        globalVar->setInitializer(initializer);
                }
                else
                {
                    globalVar->setLinkage(llvm::GlobalValue::CommonLinkage);
                    if (!globalVar->hasInitializer())
                        globalVar->setInitializer(initializer);
                }
            }
            else
            {
                globalVar->setLinkage(declaration.getScopeDepth() > 3 ? llvm::GlobalValue::InternalLinkage :
                                                                        llvm::GlobalValue::ExternalLinkage);
                if (!globalVar->hasInitializer())
                {
                    globalVar->setInitializer(initializer);
                }
            }
            m_globals[arr] = globalVar;
        }
    }
    else if (auto* tab = dynamic_cast<const semantics::TableDeclaration*>(&declaration))
    {
        auto* llvmType = convertType(tab->getType());
        llvm::Constant* initializer = llvm::Constant::getNullValue(llvmType);
        bool hasExplicitPreset = false;

        if (auto* astNode = dynamic_cast<const syntax::TableDeclaration*>(tab->getAstNode()))
        {
            if (astNode->hasPreset() && !astNode->getPresetTokens().empty())
            {
                std::size_t numElements = 1;
                if (const auto* arrayType = llvm::dyn_cast<llvm::ArrayType>(llvmType))
                    numElements = arrayType->getNumElements();
                
                bool isFieldPreset = false;
                if (!astNode->getPresetTokens().empty())
                {
                    auto it = astNode->getPresetTokens().front();
                    --it;
                    if (it->getTokenType() == lexer::TokenType::PresetKeyword)
                    {
                        isFieldPreset = true;
                    }
                }
                
                initializer = parsePresetList(astNode->getPresetTokens(), llvmType, numElements, tab->getPresetIndex(), &tab->getType(), isFieldPreset ? astNode : nullptr);
                hasExplicitPreset = true;
            }
        }

        if (m_options.baremetal && tab->getAbsoluteAddress().has_value())
        {
            llvm::Constant* addrInt =
                llvm::ConstantInt::get(m_module.getContext(), llvm::APInt(64, tab->getAbsoluteAddress().value()));
            llvm::Constant* ptr =
                llvm::ConstantExpr::getIntToPtr(addrInt, llvm::PointerType::getUnqual(m_module.getContext()));
            m_globals[tab] = ptr;
        }
        else
        {
            m_module.getOrInsertGlobal(tab->getMangledName(), llvmType);
            llvm::GlobalVariable* globalVar = m_module.getNamedGlobal(tab->getMangledName());
            if (declaration.getCommunicatorType() == semantics::CommunicatorType::External ||
                declaration.getCommunicatorType() == semantics::CommunicatorType::Library)
            {
                globalVar->setLinkage(llvm::GlobalValue::ExternalLinkage);
            }
            else if (declaration.getCommunicatorType() == semantics::CommunicatorType::Common)
            {
                if (hasExplicitPreset)
                {
                    globalVar->setLinkage(llvm::GlobalValue::ExternalLinkage);
                    if (!globalVar->hasInitializer())
                        globalVar->setInitializer(initializer);
                }
                else
                {
                    globalVar->setLinkage(llvm::GlobalValue::CommonLinkage);
                    if (!globalVar->hasInitializer())
                        globalVar->setInitializer(initializer);
                }
            }
            else
            {
                globalVar->setLinkage(declaration.getScopeDepth() > 3 ? llvm::GlobalValue::InternalLinkage :
                                                                        llvm::GlobalValue::ExternalLinkage);
                if (!globalVar->hasInitializer())
                {
                    globalVar->setInitializer(initializer);
                }
            }
            m_globals[tab] = globalVar;
        }
    }
    else if (auto* over = dynamic_cast<const semantics::OverlayDeclaration*>(&declaration))
    {
        if (auto* astNode = dynamic_cast<const syntax::OverlayDeclaration*>(over->getAstNode()))
        {
            const auto baseName = std::get<std::string>(astNode->getBase()->getValue());
            if (const semantics::Usable* baseUsable = m_program.getContext().symbolTable.lookup(baseName);
                baseUsable && m_globals.contains(baseUsable))
            {
                auto* baseAddr = llvm::cast<llvm::Constant>(m_globals[baseUsable]);
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
        llvm::FunctionCallee callee = m_module.getOrInsertFunction(proc->getMangledName(), ft);
        llvm::Function* f = llvm::cast<llvm::Function>(callee.getCallee());
        f->setLinkage(llvm::Function::ExternalLinkage);
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
                std::string usableName;
                const syntax::Node* usableAst = nullptr;
                usable->match(
                    [&](const semantics::VariableDeclaration& v)
                    {
                        usableName = v.getMangledName();
                        usableAst = v.getAstNode();
                    },
                    [&](const semantics::ArrayDeclaration& a)
                    {
                        usableName = a.getMangledName();
                        usableAst = a.getAstNode();
                    },
                    [&](const semantics::TableDeclaration& t)
                    {
                        usableName = t.getMangledName();
                        usableAst = t.getAstNode();
                    },
                    [](const auto&) {});

                if (usableName == param.name)
                {
                    bool found = false;
                    for (const auto& p : astNode.getParameters())
                    {
                        if (usableAst == p.get())
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
            auto* alloca = m_builder.CreateAlloca(llvmType, nullptr, semVar->getMangledName());
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
            auto* alloca = m_builder.CreateAlloca(llvmType, nullptr, semArr->getMangledName());
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
            auto* alloca = m_builder.CreateAlloca(llvmType, nullptr, semTab->getMangledName());
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

} // namespace coral66::cgllvm
