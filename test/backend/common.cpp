/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "common.hpp"

#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/RTDyldMemoryManager.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>

#include <stdarg.h>

void coral66::Tests::detail::computeAndGet(std::unique_ptr<llvm::Module>&& module, std::string_view functionName,
                                           coral66::FunctionRef<void(std::uintptr_t)> symbolCallback)
{
    llvm::orc::LLJITBuilder builder;
    auto jit = llvm::cantFail(builder.create());
    {
    }
    llvm::orc::SymbolStringPtr pointer = jit->getExecutionSession().intern("printf");
    // llvm::cantFail(jit->getMainJITDylib().define(
    //   llvm::orc::absoluteSymbols({{pointer, llvm::JITEvaluatedSymbol::fromPointer(debugging_printf)}})));

    llvm::orc::ThreadSafeContext ctx(std::make_unique<llvm::LLVMContext>());
    llvm::cantFail(jit->addIRModule(llvm::orc::ThreadSafeModule{std::move(module), ctx}));
    auto sym = llvm::cantFail(jit->lookup(functionName.data()));
    symbolCallback(sym.getValue());
}
