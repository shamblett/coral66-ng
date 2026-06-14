/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "code_generator.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Module.h>

#include <ranges>

#include "lib/frontend/semantics/program.hpp"

namespace coral66::cgllvm
{

code_generator::code_generator(llvm::Module& module, const syntax::Program& tree, const semantics::Program& program,
                               const source_interface& sourceInterface, const Options& options)
    : m_module(module), m_tree(tree), m_program(program), m_sourceInterface(sourceInterface), m_options(options)
{
}

void code_generator::generate()
{
    // First pass: Global Declarations
    // We iterate over semantic usables because the semantic pass already flattened and lifted
    // them. We only declare the ones that are at scope depth 1 (global).
    for (const auto& global : m_program.getGlobals())
    {
        global->match([&](const semantics::VariableDeclaration& var) { visit(var); },
                      [&](const semantics::ArrayDeclaration& arr) { visit(arr); },
                      [&](const semantics::ProcedureDeclaration& proc) { visit(proc); }, [](const auto&) {});
    }

    // Second pass: Generate body for the main program and all procedures
    visit(m_tree);
}

llvm::Type* code_generator::convertType(const semantics::Type& type)
{
    auto& ctx = m_module.getContext();
    llvm::Type* baseType = nullptr;

    if (auto* prim = type.tryAs<semantics::PrimitiveType>())
    {
        switch (prim->getKind())
        {
            case semantics::PrimitiveType::Integer: baseType = llvm::Type::getInt32Ty(ctx); break;
            case semantics::PrimitiveType::Floating: baseType = llvm::Type::getFloatTy(ctx); break;
            case semantics::PrimitiveType::Bits: baseType = llvm::Type::getInt32Ty(ctx); break;
        }
    }
    else if (auto* arr = type.tryAs<semantics::ArrayType>())
    {
        llvm::Type* elemType = convertType(arr->getElementType());
        if (arr->getDimensions().empty()) {
            baseType = llvm::PointerType::getUnqual(ctx);
        } else {
            llvm::Type* arrType = elemType;
            for (const unsigned long it : std::views::reverse(arr->getDimensions()))
            {
                arrType = llvm::ArrayType::get(arrType, it);
            }
            baseType = arrType;
        }
    }
    else if (auto* fixed = type.tryAs<semantics::FixedType>())
    {
        if (fixed->getTotalBits() <= 16)
            baseType = llvm::Type::getInt16Ty(ctx);
        else if (fixed->getTotalBits() <= 32)
            baseType = llvm::Type::getInt32Ty(ctx);
        else
            baseType = llvm::Type::getInt64Ty(ctx);
    }
    else if (auto* tab = type.tryAs<semantics::TableType>())
    {
        if (auto* def = std::get_if<semantics::TableDefinition>(&tab->getInfo().type))
        {
            std::vector<llvm::Type*> fieldTypes;
            for (const auto& [type, name] : def->getFields())
            {
                fieldTypes.push_back(convertType(*type));
            }
            llvm::Type* structType = llvm::StructType::get(ctx, fieldTypes);
            if (def->getLength() > 0)
                baseType = llvm::ArrayType::get(structType, def->getLength());
            else
                baseType = structType;
        }
        else
        {
            baseType = llvm::Type::getVoidTy(ctx);
        }
    }
    else if (type.is<semantics::OverlayType>())
    {
        baseType = llvm::Type::getInt32Ty(ctx);
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

void code_generator::visit(const syntax::Program& program)
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
    if (program.hasBlock())
    {
        m_labels.clear();
        m_namedLabels.clear();
        m_locals.clear();

        llvm::FunctionType* ft = llvm::FunctionType::get(llvm::Type::getInt32Ty(m_module.getContext()), false);
        llvm::Function* f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "main", m_module);
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
