/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "common.hpp"

#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>

void detail::computeAndGet(std::unique_ptr<llvm::Module>&& module, const std::string_view functionName,
                           const FunctionRef<void(std::uintptr_t)> symbolCallback)
{
    llvm::orc::LLJITBuilder builder;
    const auto jit = llvm::cantFail(builder.create());
    {
    }
    llvm::orc::SymbolStringPtr pointer = jit->getExecutionSession().intern("printf");

    const llvm::orc::ThreadSafeContext ctx(std::make_unique<llvm::LLVMContext>());
    llvm::cantFail(jit->addIRModule(llvm::orc::ThreadSafeModule{std::move(module), ctx}));
    const auto sym = llvm::cantFail(jit->lookup(functionName.data()));
    symbolCallback(sym.getValue());
}
