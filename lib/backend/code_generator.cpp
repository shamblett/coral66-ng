/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "code_generator.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Module.h>

#include <ranges>

#include "lib/frontend/semantics/program.hpp"

namespace coral66::semantics
{
std::optional<std::int64_t> evaluateConstantInteger(const syntax::Expression& expr);
}

namespace coral66::cgllvm
{

CodeGenerator::CodeGenerator(llvm::Module& module, const syntax::Program& tree, const semantics::Program& program,
                             const source_interface& sourceInterface, const Options& options)
    : m_module(module), m_tree(tree), m_program(program), m_sourceInterface(sourceInterface), m_options(options)
{
}

void CodeGenerator::generate()
{
    // First pass: Find global overlays to prevent double-allocation
    if (m_tree.hasBlock())
    {
        for (const auto& decl : m_tree.getBlock().getDeclarations())
        {
            if (auto* overDecl = dynamic_cast<const syntax::OverlayDeclaration*>(decl.get()))
            {
                m_globalOverlays.push_back(overDecl);
            }
        }
    }

    // First pass: Global Declarations
    for (const auto& global : m_program.getGlobals())
    {
        bool isOverlayInner = false;

        for (const auto* overDecl : m_globalOverlays)
        {
            const auto& innerDecl = overDecl->getOverlayDeclaration();
            // Try to extract the name from the inner declaration to match
            std::string innerName;
            if (auto* varDecl = dynamic_cast<const syntax::VariableDeclaration*>(&innerDecl))
            {
                if (!varDecl->getIdentifiers().empty())
                    innerName = std::get<std::string>(varDecl->getIdentifiers()[0]->getValue());
            }
            else if (auto* arrDecl = dynamic_cast<const syntax::ArrayDeclaration*>(&innerDecl))
            {
                if (!arrDecl->getIdentifiers().empty())
                    innerName = std::get<std::string>(arrDecl->getIdentifiers()[0]->getValue());
            }

            if (!innerName.empty())
            {
                std::string globalName;
                global->match([&](const semantics::VariableDeclaration& var) { globalName = var.getMangledName(); },
                              [&](const semantics::ArrayDeclaration& arr) { globalName = arr.getMangledName(); },
                              [&](const semantics::TableDeclaration& tab) { globalName = tab.getMangledName(); },
                              [&](const semantics::OverlayDeclaration& over) { globalName = over.getMangledName(); },
                              [&](const semantics::ProcedureDeclaration& proc) { globalName = proc.getMangledName(); },
                              [](const auto&) {});

                if (globalName == innerName)
                {
                    isOverlayInner = true;
                    break;
                }
            }
        }

        if (isOverlayInner)
            continue;

        global->match([&](const semantics::VariableDeclaration& var) { visit(var); },
                      [&](const semantics::ArrayDeclaration& arr) { visit(arr); },
                      [&](const semantics::TableDeclaration& tab) { visit(tab); },
                      [&](const semantics::OverlayDeclaration& over) { visit(over); },
                      [&](const semantics::ProcedureDeclaration& proc) { visit(proc); }, [](const auto&) {});
    }

    // After all normal globals are set up, resolve global overlays
    for (const auto* overDecl : m_globalOverlays)
    {
        const auto baseName = std::get<std::string>(overDecl->getBase()->getValue());

        const semantics::Usable* baseUsable = nullptr;
        for (const auto& g : m_program.getGlobals())
        {
            std::string gName;
            g->match([&](const semantics::VariableDeclaration& var) { gName = var.getMangledName(); },
                     [&](const semantics::ArrayDeclaration& arr) { gName = arr.getMangledName(); },
                     [&](const semantics::TableDeclaration& tab) { gName = tab.getMangledName(); },
                     [&](const semantics::OverlayDeclaration& over) { gName = over.getMangledName(); },
                     [&](const semantics::ProcedureDeclaration& proc) { gName = proc.getMangledName(); },
                     [](const auto&) {});
            if (gName == baseName)
            {
                baseUsable = g.get();
                break;
            }
        }

        if (baseUsable && m_globals.contains(baseUsable))
        {
            const auto baseAddr = llvm::cast<llvm::Constant>(m_globals[baseUsable]);
            std::int64_t offsetVal = 0;
            if (overDecl->getOffset()->getType() == lexer::CToken::Integer)
            {
                if (auto* intVal = std::get_if<llvm::APSInt>(&overDecl->getOffset()->getValue()))
                    offsetVal = intVal->getExtValue();
            }

            std::int64_t baseLowerBound = 0;
            if (auto* arrayDecl = baseUsable->tryAs<semantics::ArrayDeclaration>())
            {
                if (auto* astNode = dynamic_cast<const syntax::ArrayDeclaration*>(arrayDecl->getAstNode()))
                {
                    if (!astNode->getDimensions().empty())
                    {
                        if (auto lower = semantics::evaluateConstantInteger(*astNode->getDimensions()[0].first))
                            baseLowerBound = *lower;
                    }
                }
            }

            const syntax::Declaration& innerDecl = overDecl->getOverlayDeclaration();
            std::int64_t targetLowerBound = 0;
            if (auto* arrDecl = dynamic_cast<const syntax::ArrayDeclaration*>(&innerDecl))
            {
                if (!arrDecl->getDimensions().empty())
                {
                    if (auto lower = semantics::evaluateConstantInteger(*arrDecl->getDimensions()[0].first))
                        targetLowerBound = *lower;
                }
            }

            std::int64_t totalOffset = offsetVal - baseLowerBound + targetLowerBound;

            llvm::Constant* finalAddr = baseAddr;
            if (totalOffset != 0)
            {
                llvm::Type* i32Ty = llvm::Type::getInt32Ty(m_module.getContext());
                llvm::Constant* idx = llvm::ConstantInt::get(i32Ty, totalOffset, true);
                finalAddr = llvm::ConstantExpr::getGetElementPtr(i32Ty, baseAddr, idx);
            }
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
            else if (auto* tabDecl = dynamic_cast<const syntax::TableDeclaration*>(&innerDecl))
            {
                targetName = std::get<std::string>(tabDecl->getIdentifier()->getValue());
            }

            if (!targetName.empty())
            {
                const semantics::Usable* targetUsable = nullptr;
                for (const auto& g : m_program.getGlobals())
                {
                    std::string gName;
                    g->match([&](const semantics::VariableDeclaration& var) { gName = var.getMangledName(); },
                             [&](const semantics::ArrayDeclaration& arr) { gName = arr.getMangledName(); },
                             [&](const semantics::TableDeclaration& tab) { gName = tab.getMangledName(); },
                             [&](const semantics::OverlayDeclaration& over) { gName = over.getMangledName(); },
                             [&](const semantics::ProcedureDeclaration& proc) { gName = proc.getMangledName(); },
                             [](const auto&) {});
                    if (gName == targetName)
                    {
                        targetUsable = g.get();
                        break;
                    }
                }

                if (targetUsable)
                {
                    llvm::Type* valueTy = nullptr;
                    targetUsable->match([&](const semantics::VariableDeclaration& var)
                                        { valueTy = convertType(var.getType()); },
                                        [&](const semantics::ArrayDeclaration& arr)
                                        { valueTy = convertType(arr.getType()); },
                                        [&](const semantics::TableDeclaration& tab)
                                        { valueTy = convertType(tab.getType()); },
                                        [](const auto&) {});

                    llvm::Value* aliasOrAddr = finalAddr;
                    if (valueTy)
                    {
                        aliasOrAddr = llvm::GlobalAlias::create(valueTy, 0, llvm::GlobalValue::ExternalLinkage,
                                                                targetName, llvm::cast<llvm::Constant>(finalAddr),
                                                                &m_module);
                    }
                    
                    m_globals[targetUsable] = aliasOrAddr;
                }
            }
        }
    }

    // Second pass: Generate body for the main program and all procedures
    visit(m_tree);
}

llvm::Type* CodeGenerator::convertType(const semantics::Type& type)
{
    auto& ctx = m_module.getContext();
    llvm::Type* baseType = nullptr;

    if (auto* prim = type.tryAs<semantics::PrimitiveType>())
    {
        switch (prim->getKind())
        {
            case semantics::PrimitiveType::Integer: baseType = llvm::Type::getInt32Ty(ctx); break;
            case semantics::PrimitiveType::Pointer: baseType = llvm::PointerType::getUnqual(ctx); break;
            case semantics::PrimitiveType::Floating: baseType = llvm::Type::getFloatTy(ctx); break;
            case semantics::PrimitiveType::Bits: baseType = llvm::Type::getInt32Ty(ctx); break;
        }
    }
    else if (auto* arr = type.tryAs<semantics::ArrayType>())
    {
        llvm::Type* elemType = convertType(arr->getElementType());
        if (arr->getDimensions().empty())
        {
            baseType = llvm::PointerType::getUnqual(ctx);
        }
        else
        {
            llvm::Type* arrType = elemType;
            for (const unsigned long it : std::views::reverse(arr->getDimensions()))
            {
                arrType = llvm::ArrayType::get(arrType, it);
            }
            baseType = arrType;
        }
    }
    else if (type.is<semantics::FixedType>() || type.is<semantics::OverlayType>())
    {
        baseType = llvm::Type::getInt32Ty(ctx);
    }
    else if (auto* tab = type.tryAs<semantics::TableType>())
    {
        if (auto* def = std::get_if<semantics::TableDefinition>(&tab->getInfo().type))
        {
            llvm::Type* wordType = llvm::Type::getInt32Ty(ctx);
            if (def->getLength() > 0)
            {
                baseType = llvm::ArrayType::get(wordType, def->getWidth() * def->getLength());
            }
            else
            {
                // Just for references or dynamically sized
                if (def->getWidth() > 0)
                    baseType = llvm::ArrayType::get(wordType, def->getWidth());
                else
                    baseType = llvm::PointerType::getUnqual(ctx);
            }
        }
        else
        {
            baseType = llvm::Type::getVoidTy(ctx);
        }
    }
    else if (type.is<semantics::LabelType>() || type.is<semantics::SwitchType>())
    {
        baseType = llvm::PointerType::getUnqual(ctx);
    }
    else
    {
        baseType = llvm::Type::getVoidTy(ctx);
    }

    if (type.isLocation() && !(baseType != nullptr && baseType->isVoidTy()))
    {
        return llvm::PointerType::getUnqual(ctx);
    }

    return baseType;
}

void CodeGenerator::visit(const syntax::Program& program)
{
    // Generate code for all top-level procedures
    for (const auto& global : m_program.getGlobals())
    {
        if (const auto* procedureDeclaration = global->tryAs<semantics::ProcedureDeclaration>())
        {
            if (procedureDeclaration->getAstNode())
            {
                if (auto* syntaxProc =
                        dynamic_cast<const syntax::ProcedureDeclaration*>(procedureDeclaration->getAstNode()))
                {
                    generateProcedure(*procedureDeclaration, *syntaxProc);
                }
            }
        }
    }

    // Now generate the main program block (if any)
    if (program.hasBlock() && !program.getIsSegment())
    {
        m_labels.clear();
        m_namedLabels.clear();
        m_locals.clear();

        llvm::FunctionType* ft = llvm::FunctionType::get(llvm::Type::getInt32Ty(m_module.getContext()), false);
        llvm::FunctionCallee callee = m_module.getOrInsertFunction("main", ft);
        llvm::Function* f = llvm::cast<llvm::Function>(callee.getCallee());
        f->setLinkage(llvm::Function::ExternalLinkage);
        m_currentFunction = f;

        llvm::BasicBlock* bb = llvm::BasicBlock::Create(m_module.getContext(), "entry", f);
        m_builder.SetInsertPoint(bb);

        for (const auto& stmt : program.getBlock().getStatements())
            preScanLabels(*stmt);

        visit(program.getBlock());

        if (!m_builder.GetInsertBlock()->getTerminator())
        {
            m_builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_module.getContext()), 0));
        }
    }
}

} // namespace coral66::cgllvm
