/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "code_generator.hpp"

#include "lib/frontend/semantics/program.hpp"

namespace coral66::cgllvm
{

void code_generator::visit(const semantics::Declaration& declaration)
{
    // Only process global declarations here.

    if (auto* var = dynamic_cast<const semantics::VariableDeclaration*>(&declaration))
    {
        auto* llvmType = convertType(var->getType());
        auto* globalVar = new llvm::GlobalVariable(m_module, llvmType, false, llvm::GlobalValue::ExternalLinkage,
                                                   llvm::Constant::getNullValue(llvmType), var->getName());
        m_globals[var] = globalVar;
    }
    else if (auto* arr = dynamic_cast<const semantics::ArrayDeclaration*>(&declaration))
    {
        auto* llvmType = convertType(arr->getType());
        auto* globalVar = new llvm::GlobalVariable(m_module, llvmType, false, llvm::GlobalValue::ExternalLinkage,
                                                   llvm::Constant::getNullValue(llvmType), arr->getName());
        m_globals[arr] = globalVar;
    }
    else if (auto* tab = dynamic_cast<const semantics::TableDeclaration*>(&declaration))
    {
        auto* llvmType = convertType(tab->getType());
        auto* globalVar = new llvm::GlobalVariable(m_module, llvmType, false, llvm::GlobalValue::ExternalLinkage,
                                                   llvm::Constant::getNullValue(llvmType), tab->getName());
        m_globals[tab] = globalVar;
    }
    else if (auto* over = dynamic_cast<const semantics::OverlayDeclaration*>(&declaration))
    {
        auto* llvmType = convertType(over->getType());
        auto* globalVar = new llvm::GlobalVariable(m_module, llvmType, false, llvm::GlobalValue::ExternalLinkage,
                                                   llvm::Constant::getNullValue(llvmType), over->getName());
        m_globals[over] = globalVar;
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

void code_generator::generateProcedure(const semantics::ProcedureDeclaration& procedureDeclaration,
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
                    param.type->isLocation() || llvmType->isPointerTy())
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

void code_generator::visit(const syntax::Block& block)
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

void code_generator::visit(const syntax::Declaration& declaration)
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

void code_generator::visit(const syntax::VariableDeclaration& declaration)
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

void code_generator::visit(const syntax::ArrayDeclaration& declaration)
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

void code_generator::visit(const syntax::TableDeclaration& declaration)
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

void code_generator::visit(const syntax::OverlayDeclaration& declaration)
{
    if (const auto it = m_program.getContext().resolvedIdentifiers.find(&declaration);
        it != m_program.getContext().resolvedIdentifiers.end())
    {
        if (auto* semOver = it->second->tryAs<semantics::OverlayDeclaration>())
        {
            auto* llvmType = convertType(semOver->getType());
            auto* alloca = m_builder.CreateAlloca(llvmType, nullptr, semOver->getName());
            m_locals[it->second] = alloca;
        }
    }
}

void code_generator::visit(const syntax::ProcedureDeclaration& declaration)
{
    // Nested procedures can be handled here if needed.
    // For Phase 1, we only need top-level procedures.
}

} // namespace coral66::cgllvm
