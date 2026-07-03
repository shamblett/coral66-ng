/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>
#include <iostream>

#include "common.hpp"
#include "lib/backend/code_generation.hpp"
#include "lib/frontend/lexer/lexer.hpp"
#include "lib/frontend/parser/parser.hpp"
#include "lib/frontend/preprocessor/preprocessor.hpp"
#include "lib/frontend/semantics/program.hpp"
#include "test/common/test_config.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace Catch::Matchers;

TEST_CASE("Review 1", "[Backend][Execution]")
{
    SECTION("Array Bounds Execution")
    {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("review/1/array_bounds.cor", "BACKEND_DIR");
        auto source = readFile(sourcePath);
        REQUIRE_FALSE(source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);

        bool errors = false;
        auto ppTokens = coral66::lexer::tokenize(source, &ss, &errors);
        REQUIRE_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto parsed = coral66::parser::parseProgram(cTokens, &ss, &errors);
        if (errors)
        {
            WARN(ss.str());
        }
        REQUIRE_FALSE(errors);

        auto program = coral66::semantics::analyse(parsed, std::move(cTokens), &ss, &errors);
        if (errors)
        {
            WARN(ss.str());
        }
        REQUIRE_FALSE(errors);

        // Code Generation
        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("review1_test", context);

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        module->print(llvm::errs(), nullptr);
        REQUIRE_FALSE(llvm::verifyModule(*module, &llvm::errs()));

        std::string engineError;
        llvm::ExecutionEngine* ee =
            llvm::EngineBuilder(std::move(module)).setErrorStr(&engineError).setEngineKind(llvm::EngineKind::JIT).create();
        if (!ee)
        {
            llvm::errs() << "JIT Error: " << engineError << "\n";
        }
        REQUIRE(ee != nullptr);

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        REQUIRE(mainFunc != nullptr);

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        auto ansAddr = reinterpret_cast<void*>(ee->getGlobalValueAddress("ans"));
        REQUIRE(ansAddr != nullptr);

        int32_t finalAns = *static_cast<int32_t*>(ansAddr);
        REQUIRE(finalAns == 17); // 5 + 12

        delete ee;
    }

    SECTION("Overlay Aliasing Read Execution")
    {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("review/1/overlay_aliasing.cor", "BACKEND_DIR");
        auto source = readFile(sourcePath);
        REQUIRE_FALSE(source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);

        bool errors = false;
        auto ppTokens = coral66::lexer::tokenize(source, &ss, &errors);
        REQUIRE_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto parsed = coral66::parser::parseProgram(cTokens, &ss, &errors);
        if (errors)
        {
            WARN(ss.str());
        }
        REQUIRE_FALSE(errors);

        auto program = coral66::semantics::analyse(parsed, std::move(cTokens), &ss, &errors);
        if (errors)
        {
            WARN(ss.str());
        }
        REQUIRE_FALSE(errors);

        // Code Generation
        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("review1_test", context);

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        module->print(llvm::errs(), nullptr);
        REQUIRE_FALSE(llvm::verifyModule(*module, &llvm::errs()));

        std::string engineError;
        llvm::ExecutionEngine* ee =
            llvm::EngineBuilder(std::move(module)).setErrorStr(&engineError).setEngineKind(llvm::EngineKind::JIT).create();
        if (!ee)
        {
            llvm::errs() << "JIT Error: " << engineError << "\n";
        }
        REQUIRE(ee != nullptr);

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        REQUIRE(mainFunc != nullptr);

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        auto ansAddr = reinterpret_cast<void*>(ee->getGlobalValueAddress("ans"));
        REQUIRE(ansAddr != nullptr);

        int32_t finalAns = *static_cast<int32_t*>(ansAddr);
        REQUIRE(finalAns == 1);

        delete ee;
    }

    SECTION("Overlay Aliasing Write Execution")
    {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("review/1/overlay_aliasing_mod.cor", "BACKEND_DIR");
        auto source = readFile(sourcePath);
        REQUIRE_FALSE(source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);

        bool errors = false;
        auto ppTokens = coral66::lexer::tokenize(source, &ss, &errors);
        REQUIRE_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto parsed = coral66::parser::parseProgram(cTokens, &ss, &errors);
        if (errors)
        {
            WARN(ss.str());
        }
        REQUIRE_FALSE(errors);

        auto program = coral66::semantics::analyse(parsed, std::move(cTokens), &ss, &errors);
        if (errors)
        {
            WARN(ss.str());
        }
        REQUIRE_FALSE(errors);

        // Code Generation
        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("review1_test", context);

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        module->print(llvm::errs(), nullptr);
        REQUIRE_FALSE(llvm::verifyModule(*module, &llvm::errs()));

        std::string engineError;
        llvm::ExecutionEngine* ee =
            llvm::EngineBuilder(std::move(module)).setErrorStr(&engineError).setEngineKind(llvm::EngineKind::JIT).create();
        if (!ee)
        {
            llvm::errs() << "JIT Error: " << engineError << "\n";
        }
        REQUIRE(ee != nullptr);

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        REQUIRE(mainFunc != nullptr);

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        auto ansAddr = reinterpret_cast<void*>(ee->getGlobalValueAddress("ans"));
        REQUIRE(ansAddr != nullptr);

        int32_t finalAns = *static_cast<int32_t*>(ansAddr);
        REQUIRE(finalAns == 1);

        delete ee;
    }
}
TEST_CASE("Phase 1", "[Backend][Execution]")
{
    SECTION("Sieve End-to-End Execution")
    {
        // Initialize LLVM Native Target for JIT Execution
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("p1/sieve/sieve.cor", "BACKEND_DIR");
        auto source = readFile(sourcePath);
        REQUIRE_FALSE(source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);

        // Frontend passes
        bool errors = false;
        auto ppTokens = coral66::lexer::tokenize(source, &ss, &errors);
        REQUIRE_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto parsed = coral66::parser::parseProgram(cTokens, &ss, &errors);
        if (errors)
        {
            WARN(ss.str());
        }
        REQUIRE_FALSE(errors);

        auto program = coral66::semantics::analyse(parsed, std::move(cTokens), &ss, &errors);
        if (errors)
        {
            WARN(ss.str());
        }
        REQUIRE_FALSE(errors);

        // Code Generation
        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("sieve", context);

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        module->print(llvm::errs(), nullptr);
        REQUIRE_FALSE(llvm::verifyModule(*module, &llvm::errs()));
        module->print(llvm::errs(), nullptr);

        // JIT Execution
        std::string errStr;
        llvm::ExecutionEngine* ee =
            llvm::EngineBuilder(std::move(module)).setErrorStr(&errStr).setEngineKind(llvm::EngineKind::JIT).create();
        REQUIRE(ee != nullptr);

        // Run the `main` or `sieve` function

        // Ensure all globals are allocated and JIT compilation is done before running
        ee->finalizeObject();

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        REQUIRE(mainFunc != nullptr);

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        // The ExecutionEngine took ownership of the module, but we can access globals by name
        auto ansAddr = reinterpret_cast<void*>(ee->getGlobalValueAddress("ans"));

        REQUIRE(ansAddr != nullptr);

        // Dereference to get the actual value
        int32_t finalAns = *static_cast<int32_t*>(ansAddr);

        // The sieve algorithm in sieve.cor produces exactly 1899 prime numbers under 8191.
        REQUIRE(finalAns == 1899);

        delete ee;
    }
}

TEST_CASE("Phase 2", "[Backend][Execution]")
{
    SECTION("Phase 2 Test End-to-End Execution")
    {
        // Initialize LLVM Native Target for JIT Execution
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("p2/phase2.cor", "BACKEND_DIR");
        auto source = readFile(sourcePath);
        REQUIRE_FALSE(source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);

        // Frontend passes
        bool errors = false;
        auto ppTokens = coral66::lexer::tokenize(source, &ss, &errors);
        REQUIRE_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto parsed = coral66::parser::parseProgram(cTokens, &ss, &errors);
        if (errors)
        {
            WARN(ss.str());
        }
        REQUIRE_FALSE(errors);

        auto program = coral66::semantics::analyse(parsed, std::move(cTokens), &ss, &errors);
        if (errors)
        {
            WARN(ss.str());
        }
        REQUIRE_FALSE(errors);

        // Code Generation
        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("phase2", context);

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        module->print(llvm::errs(), nullptr);
        REQUIRE_FALSE(llvm::verifyModule(*module, &llvm::errs()));
        module->print(llvm::errs(), nullptr);

        // JIT Execution
        std::string errStr;
        llvm::ExecutionEngine* ee =
            llvm::EngineBuilder(std::move(module)).setErrorStr(&errStr).setEngineKind(llvm::EngineKind::JIT).create();
        REQUIRE(ee != nullptr);

        // Run the `main` or `Phase2Test` function

        // Ensure all globals are allocated and JIT compilation is done before running
        ee->finalizeObject();

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        if (!mainFunc)
            mainFunc = ee->FindFunctionNamed("Phase2Test");
        REQUIRE(mainFunc != nullptr);

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        // The ExecutionEngine took ownership of the module, but we can access globals by name
        auto ansAddr = reinterpret_cast<void*>(ee->getGlobalValueAddress("ans"));

        REQUIRE(ansAddr != nullptr);

        // Dereference to get the actual value
        int32_t finalAns = *static_cast<int32_t*>(ansAddr);

        REQUIRE(finalAns == 42);

        delete ee;
    }
}

TEST_CASE("Phase 3", "[Backend][Execution]")
{
    SECTION("Phase 3 Test End-to-End Execution")
    {
        // Initialize LLVM Native Target for JIT Execution
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("p3/phase3.cor", "BACKEND_DIR");
        auto source = readFile(sourcePath);
        REQUIRE_FALSE(source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);

        // Frontend passes
        bool errors = false;
        auto ppTokens = coral66::lexer::tokenize(source, &ss, &errors);
        REQUIRE_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto parsed = coral66::parser::parseProgram(cTokens, &ss, &errors);
        if (errors)
        {
            WARN(ss.str());
        }
        REQUIRE_FALSE(errors);

        auto program = coral66::semantics::analyse(parsed, std::move(cTokens), &ss, &errors);
        if (errors)
        {
            WARN(ss.str());
        }
        REQUIRE_FALSE(errors);

        // Code Generation
        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("phase3", context);

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        module->print(llvm::errs(), nullptr);
        REQUIRE_FALSE(llvm::verifyModule(*module, &llvm::errs()));
        module->print(llvm::errs(), nullptr);

        // JIT Execution
        std::string errStr;
        llvm::ExecutionEngine* ee =
            llvm::EngineBuilder(std::move(module)).setErrorStr(&errStr).setEngineKind(llvm::EngineKind::JIT).create();
        REQUIRE(ee != nullptr);

        // Run the `main` or `Phase3Test` function

        // Ensure all globals are allocated and JIT compilation is done before running
        ee->finalizeObject();

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        if (!mainFunc)
            mainFunc = ee->FindFunctionNamed("Phase3Test");
        REQUIRE(mainFunc != nullptr);

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        // The ExecutionEngine took ownership of the module, but we can access globals by name
        auto ansAddr = reinterpret_cast<void*>(ee->getGlobalValueAddress("ans"));

        REQUIRE(ansAddr != nullptr);

        // Dereference to get the actual value
        int32_t finalAns = *static_cast<int32_t*>(ansAddr);

        REQUIRE(finalAns == 42);

        delete ee;
    }
}

TEST_CASE("Phase 4", "[Backend][Execution]")
{
    SECTION("Phase 4 Test End-to-End Execution")
    {
        // Initialize LLVM Native Target for JIT Execution
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("p4/phase4.cor", "BACKEND_DIR");
        auto source = readFile(sourcePath);
        REQUIRE_FALSE(source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);

        // Frontend passes
        bool errors = false;
        auto ppTokens = coral66::lexer::tokenize(source, &ss, &errors);
        REQUIRE_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto parsed = coral66::parser::parseProgram(cTokens, &ss, &errors);
        if (errors)
        {
            WARN(ss.str());
        }
        REQUIRE_FALSE(errors);

        auto program = coral66::semantics::analyse(parsed, std::move(cTokens), &ss, &errors);
        if (errors)
        {
            WARN(ss.str());
        }
        REQUIRE_FALSE(errors);

        // Code Generation
        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("phase4", context);

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        module->print(llvm::errs(), nullptr);
        REQUIRE_FALSE(llvm::verifyModule(*module, &llvm::errs()));
        module->print(llvm::errs(), nullptr);

        // JIT Execution
        std::string errStr;
        llvm::ExecutionEngine* ee =
            llvm::EngineBuilder(std::move(module)).setErrorStr(&errStr).setEngineKind(llvm::EngineKind::JIT).create();
        REQUIRE(ee != nullptr);

        // Run the `main` or `Phase4Test` function

        // Ensure all globals are allocated and JIT compilation is done before running
        ee->finalizeObject();

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        if (!mainFunc)
            mainFunc = ee->FindFunctionNamed("Phase4Test");
        REQUIRE(mainFunc != nullptr);

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        // The ExecutionEngine took ownership of the module, but we can access globals by name
        auto ansAddr = reinterpret_cast<void*>(ee->getGlobalValueAddress("ans"));

        REQUIRE(ansAddr != nullptr);

        // Dereference to get the actual value
        int32_t finalAns = *static_cast<int32_t*>(ansAddr);

        REQUIRE(finalAns == 5);

        delete ee;
    }
}
TEST_CASE("Phase 5", "[Backend][Execution]")
{
    SECTION("Ackermann End-to-End Execution")
    {
        // Initialize LLVM Native Target for JIT Execution
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("p5/ackermann/ackermann.cor", "BACKEND_DIR");
        auto source = readFile(sourcePath);
        REQUIRE_FALSE(source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);

        // Frontend passes
        bool errors = false;
        auto ppTokens = coral66::lexer::tokenize(source, &ss, &errors);
        REQUIRE_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto parsed = coral66::parser::parseProgram(cTokens, &ss, &errors);
        if (errors)
        {
            WARN(ss.str());
        }
        REQUIRE_FALSE(errors);

        auto program = coral66::semantics::analyse(parsed, std::move(cTokens), &ss, &errors);
        if (errors)
        {
            WARN(ss.str());
        }
        REQUIRE_FALSE(errors);

        // Code Generation
        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("ackermann", context);

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        module->print(llvm::errs(), nullptr);
        REQUIRE_FALSE(llvm::verifyModule(*module, &llvm::errs()));
        module->print(llvm::errs(), nullptr);

        // JIT Execution
        std::string errStr;
        llvm::ExecutionEngine* ee =
            llvm::EngineBuilder(std::move(module)).setErrorStr(&errStr).setEngineKind(llvm::EngineKind::JIT).create();
        REQUIRE(ee != nullptr);

        // Run the `main` or `ackermann` function

        // Ensure all globals are allocated and JIT compilation is done before running
        ee->finalizeObject();

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        REQUIRE(mainFunc != nullptr);

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        // The ExecutionEngine took ownership of the module, but we can access globals by name
        auto ansAddr = reinterpret_cast<void*>(ee->getGlobalValueAddress("ans"));

        REQUIRE(ansAddr != nullptr);

        // Dereference to get the actual value
        int32_t finalAns = *static_cast<int32_t*>(ansAddr);

        // The answer for the Ackermann function (3,6) is 509
        REQUIRE(finalAns == 509);

        delete ee;
    }

    SECTION("Swap End-to-End Execution")
    {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("p5/swap/swap.cor", "BACKEND_DIR");
        auto source = readFile(sourcePath);
        REQUIRE_FALSE(source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);

        bool errors = false;
        auto ppTokens = coral66::lexer::tokenize(source, &ss, &errors);
        REQUIRE_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto parsed = coral66::parser::parseProgram(cTokens, &ss, &errors);
        if (errors)
            llvm::errs() << ss.str() << "\n";
        REQUIRE_FALSE(errors);

        auto program = coral66::semantics::analyse(parsed, std::move(cTokens), &ss, &errors);
        if (errors)
            llvm::errs() << ss.str() << "\n";
        REQUIRE_FALSE(errors);

        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("swap", context);

        coral66::cgllvm::Options codegenOptions;
        coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        REQUIRE_FALSE(llvm::verifyModule(*module, &llvm::errs()));
        module->print(llvm::errs(), nullptr);

        std::string errStr;
        llvm::ExecutionEngine* ee =
            llvm::EngineBuilder(std::move(module)).setErrorStr(&errStr).setEngineKind(llvm::EngineKind::JIT).create();
        REQUIRE(ee != nullptr);

        ee->finalizeObject();

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        if (!mainFunc)
            mainFunc = ee->FindFunctionNamed("SwapTest");
        REQUIRE(mainFunc != nullptr);

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        auto ansAddr = reinterpret_cast<void*>(ee->getGlobalValueAddress("ans"));
        REQUIRE(ansAddr != nullptr);

        int32_t finalAns = *static_cast<int32_t*>(ansAddr);
        REQUIRE(finalAns == 1);

        delete ee;
    }

    SECTION("Inline Asm End-to-End Execution")
    {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("p5/inlineasm/inlineasm.cor", "BACKEND_DIR");
        auto source = readFile(sourcePath);
        REQUIRE_FALSE(source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);

        bool errors = false;
        auto ppTokens = coral66::lexer::tokenize(source, &ss, &errors);
        REQUIRE_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto parsed = coral66::parser::parseProgram(cTokens, &ss, &errors);
        if (errors)
            llvm::errs() << ss.str() << "\n";
        REQUIRE_FALSE(errors);

        auto program = coral66::semantics::analyse(parsed, std::move(cTokens), &ss, &errors);
        if (errors)
            llvm::errs() << ss.str() << "\n";
        REQUIRE_FALSE(errors);

        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("inlineasm", context);

        coral66::cgllvm::Options codegenOptions;
        coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        REQUIRE_FALSE(llvm::verifyModule(*module, &llvm::errs()));
        module->print(llvm::errs(), nullptr);

        std::string errStr;
        llvm::ExecutionEngine* ee =
            llvm::EngineBuilder(std::move(module)).setErrorStr(&errStr).setEngineKind(llvm::EngineKind::JIT).create();
        REQUIRE(ee != nullptr);

        ee->finalizeObject();

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        if (!mainFunc)
            mainFunc = ee->FindFunctionNamed("InlineAsmTest");
        REQUIRE(mainFunc != nullptr);

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        auto ansAddr = reinterpret_cast<void*>(ee->getGlobalValueAddress("ans"));
        REQUIRE(ansAddr != nullptr);

        int32_t finalAns = *static_cast<int32_t*>(ansAddr);
        REQUIRE(finalAns == 40);

        delete ee;
    }
}

TEST_CASE("Phase 6", "[Backend][Execution]")
{
    SECTION("Whetstone End-to-End Execution")
    {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("p6/whetstone/whetstone.cor", "BACKEND_DIR");
        auto source = readFile(sourcePath);
        REQUIRE_FALSE(source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);

        bool errors = false;
        auto ppTokens = coral66::lexer::tokenize(source, &ss, &errors);
        REQUIRE_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto parsed = coral66::parser::parseProgram(cTokens, &ss, &errors);
        if (errors)
        {
            WARN(ss.str());
        }
        REQUIRE_FALSE(errors);

        auto program = coral66::semantics::analyse(parsed, std::move(cTokens), &ss, &errors);
        if (errors)
        {
            WARN(ss.str());
        }
        REQUIRE_FALSE(errors);

        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("whetstone", context);

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        module->print(llvm::errs(), nullptr);
        REQUIRE_FALSE(llvm::verifyModule(*module, &llvm::errs()));
        module->print(llvm::errs(), nullptr);

        std::string errStr;
        llvm::ExecutionEngine* ee =
            llvm::EngineBuilder(std::move(module)).setErrorStr(&errStr).setEngineKind(llvm::EngineKind::JIT).create();
        REQUIRE(ee != nullptr);

        ee->finalizeObject();

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        if (!mainFunc)
            mainFunc = ee->FindFunctionNamed("Whetstone");
        REQUIRE(mainFunc != nullptr);

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        // Square root
        auto sqrtAnsAddr = reinterpret_cast<void*>(ee->getGlobalValueAddress("sqrtans"));
        REQUIRE(sqrtAnsAddr != nullptr);
        int32_t sqrtAns = *static_cast<int32_t*>(sqrtAnsAddr);
        REQUIRE(sqrtAns == 2.0);

        delete ee;
    }

    SECTION("Phase 6 Test End-to-End Execution")
    {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("p6/phase6.cor", "BACKEND_DIR");
        auto source = readFile(sourcePath);
        REQUIRE_FALSE(source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);

        bool errors = false;
        auto ppTokens = coral66::lexer::tokenize(source, &ss, &errors);
        REQUIRE_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto parsed = coral66::parser::parseProgram(cTokens, &ss, &errors);
        if (errors)
        {
            WARN(ss.str());
        }
        REQUIRE_FALSE(errors);

        auto program = coral66::semantics::analyse(parsed, std::move(cTokens), &ss, &errors);
        if (errors)
        {
            WARN(ss.str());
        }
        REQUIRE_FALSE(errors);

        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("phase6", context);

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        module->print(llvm::errs(), nullptr);
        REQUIRE_FALSE(llvm::verifyModule(*module, &llvm::errs()));
        module->print(llvm::errs(), nullptr);

        std::string errStr;
        llvm::ExecutionEngine* ee =
            llvm::EngineBuilder(std::move(module)).setErrorStr(&errStr).setEngineKind(llvm::EngineKind::JIT).create();
        REQUIRE(ee != nullptr);

        ee->finalizeObject();

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        if (!mainFunc)
            mainFunc = ee->FindFunctionNamed("Phase6Test");
        REQUIRE(mainFunc != nullptr);

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        auto ansAddr = reinterpret_cast<void*>(ee->getGlobalValueAddress("ans"));
        REQUIRE(ansAddr != nullptr);

        int32_t finalAns = *static_cast<int32_t*>(ansAddr);
        REQUIRE(finalAns == 42);

        delete ee;
    }
}
