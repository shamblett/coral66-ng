/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 04/07/2026
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

///
/// Test suite added as a result of backend review 1
///
TEST_CASE("Review 1", "[Backend][Execution]")
{
    SECTION("Bits Part-Word Execution")
    {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("review/1/bits_partword.cor", "BACKEND_DIR");
        auto source = readFile(sourcePath);
        REQUIRE_FALSE(source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);

        bool errors = false;
        auto ppTokens = coral66::lexer::tokenize(source, &ss, &errors);
        if (errors)
        {
            std::cerr << "LEXER ERROR: " << ss.str() << std::endl;
        }
        REQUIRE_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        if (errors)
            WARN(ss.str());
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
        auto module = std::make_unique<llvm::Module>("review1_bits", context);

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        module->print(llvm::errs(), nullptr);
        REQUIRE_FALSE(llvm::verifyModule(*module, &llvm::errs()));
        module->print(llvm::errs(), nullptr);

        std::string engineError;
        llvm::ExecutionEngine* ee = llvm::EngineBuilder(std::move(module))
                                        .setErrorStr(&engineError)
                                        .setEngineKind(llvm::EngineKind::JIT)
                                        .create();
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

    SECTION("Fixed Point Execution - Variables")
    {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("review/1/fixed_point_variables.cor", "BACKEND_DIR");
        auto source = readFile(sourcePath);
        REQUIRE_FALSE(source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);

        bool errors = false;
        auto ppTokens = coral66::lexer::tokenize(source, &ss, &errors);
        if (errors)
        {
            std::cerr << "LEXER ERROR: " << ss.str() << std::endl;
        }
        REQUIRE_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        if (errors)
            WARN(ss.str());
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
        auto module = std::make_unique<llvm::Module>("review1_fixed", context);

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        module->print(llvm::errs(), nullptr);
        REQUIRE_FALSE(llvm::verifyModule(*module, &llvm::errs()));
        module->print(llvm::errs(), nullptr);

        std::string engineError;
        llvm::ExecutionEngine* ee = llvm::EngineBuilder(std::move(module))
                                        .setErrorStr(&engineError)
                                        .setEngineKind(llvm::EngineKind::JIT)
                                        .create();
        if (!ee)
        {
            llvm::errs() << "JIT Error: " << engineError << "\n";
        }
        REQUIRE(ee != nullptr);

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        REQUIRE(mainFunc != nullptr);

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        // f1 4 fractional bits
        auto varAddr = reinterpret_cast<void*>(ee->getGlobalValueAddress("f1"));
        REQUIRE(varAddr != nullptr);
        int32_t final = *static_cast<int32_t*>(varAddr);
        /// 2 * 16 = 32
        CHECK(final == 32);

        // f2 6 fractional bits
        varAddr = reinterpret_cast<void*>(ee->getGlobalValueAddress("f2"));
        REQUIRE(varAddr != nullptr);
        final = *static_cast<int32_t*>(varAddr);
        /// 4 * 64 = 256
        CHECK(final == 256);

        // f3 4 fractional bits
        varAddr = reinterpret_cast<void*>(ee->getGlobalValueAddress("f3"));
        REQUIRE(varAddr != nullptr);
        final = *static_cast<int32_t*>(varAddr);
        /// (256<<2) / 32 = 1024 / 32 = 32
        CHECK(final == 32);

        // Ans
        varAddr = reinterpret_cast<void*>(ee->getGlobalValueAddress("ans"));
        REQUIRE(varAddr != nullptr);
        final = *static_cast<int32_t*>(varAddr);
        CHECK(final == 1);

        delete ee;
    }

    SECTION("Fixed Point Execution - Constants")
    {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("review/1/fixed_point_constants.cor", "BACKEND_DIR");
        auto source = readFile(sourcePath);
        REQUIRE_FALSE(source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);

        bool errors = false;
        auto ppTokens = coral66::lexer::tokenize(source, &ss, &errors);
        if (errors)
        {
            std::cerr << "LEXER ERROR: " << ss.str() << std::endl;
        }
        REQUIRE_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        if (errors)
            WARN(ss.str());
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
        auto module = std::make_unique<llvm::Module>("review1_fixed", context);

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        module->print(llvm::errs(), nullptr);
        REQUIRE_FALSE(llvm::verifyModule(*module, &llvm::errs()));
        module->print(llvm::errs(), nullptr);

        std::string engineError;
        llvm::ExecutionEngine* ee = llvm::EngineBuilder(std::move(module))
                                        .setErrorStr(&engineError)
                                        .setEngineKind(llvm::EngineKind::JIT)
                                        .create();
        if (!ee)
        {
            llvm::errs() << "JIT Error: " << engineError << "\n";
        }
        REQUIRE(ee != nullptr);

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        REQUIRE(mainFunc != nullptr);

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        // f1 4 fractional bits
        auto varAddr = reinterpret_cast<void*>(ee->getGlobalValueAddress("f1"));
        REQUIRE(varAddr != nullptr);
        int32_t final = *static_cast<int32_t*>(varAddr);
        /// 2 * 16 = 32
        CHECK(final == 32);

        // f2 6 fractional bits
        varAddr = reinterpret_cast<void*>(ee->getGlobalValueAddress("f2"));
        REQUIRE(varAddr != nullptr);
        final = *static_cast<int32_t*>(varAddr);
        /// 4 * 64 = 256
        CHECK(final == 256);

        // f3 4 fractional bits
        varAddr = reinterpret_cast<void*>(ee->getGlobalValueAddress("f3"));
        REQUIRE(varAddr != nullptr);
        final = *static_cast<int32_t*>(varAddr);
        /// (256<<2) / 32 = 1024 / 32 = 32
        CHECK(final == 32);

        // Ans
        varAddr = reinterpret_cast<void*>(ee->getGlobalValueAddress("ans"));
        REQUIRE(varAddr != nullptr);
        final = *static_cast<int32_t*>(varAddr);
        CHECK(final == 1);

        delete ee;
    }

    SECTION("Fixed Point Execution - Operators")
    {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("review/1/fixed_point_operators.cor", "BACKEND_DIR");
        auto source = readFile(sourcePath);
        REQUIRE_FALSE(source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);

        bool errors = false;
        auto ppTokens = coral66::lexer::tokenize(source, &ss, &errors);
        if (errors)
        {
            std::cerr << "LEXER ERROR: " << ss.str() << std::endl;
        }
        REQUIRE_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        if (errors)
            WARN(ss.str());
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
        auto module = std::make_unique<llvm::Module>("review1_fixed", context);

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        module->print(llvm::errs(), nullptr);
        REQUIRE_FALSE(llvm::verifyModule(*module, &llvm::errs()));
        module->print(llvm::errs(), nullptr);

        std::string engineError;
        llvm::ExecutionEngine* ee = llvm::EngineBuilder(std::move(module))
                                        .setErrorStr(&engineError)
                                        .setEngineKind(llvm::EngineKind::JIT)
                                        .create();
        if (!ee)
        {
            llvm::errs() << "JIT Error: " << engineError << "\n";
        }
        REQUIRE(ee != nullptr);

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        REQUIRE(mainFunc != nullptr);

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        // gtv
        auto varAddr = reinterpret_cast<void*>(ee->getGlobalValueAddress("gtv"));
        REQUIRE(varAddr != nullptr);
        int32_t final = *static_cast<int32_t*>(varAddr);
        CHECK(final == 1);

        // gtc
        varAddr = reinterpret_cast<void*>(ee->getGlobalValueAddress("gtc"));
        REQUIRE(varAddr != nullptr);
        final = *static_cast<int32_t*>(varAddr);
        CHECK(final == 1);

        // ltv
        varAddr = reinterpret_cast<void*>(ee->getGlobalValueAddress("ltv"));
        REQUIRE(varAddr != nullptr);
        final = *static_cast<int32_t*>(varAddr);
        CHECK(final == 1);

        // ltc
        varAddr = reinterpret_cast<void*>(ee->getGlobalValueAddress("ltc"));
        REQUIRE(varAddr != nullptr);
        final = *static_cast<int32_t*>(varAddr);
        CHECK(final == 1);

        delete ee;
    }

    SECTION("Procedure Label Parameter Execution")
    {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("review/1/proc_label.cor", "BACKEND_DIR");
        auto source = readFile(sourcePath);
        REQUIRE_FALSE(source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);

        bool errors = false;
        auto ppTokens = coral66::lexer::tokenize(source, &ss, &errors);
        if (errors)
        {
            std::cerr << "LEXER ERROR: " << ss.str() << std::endl;
        }
        REQUIRE_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        if (errors)
            WARN(ss.str());
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

        auto* context = new llvm::LLVMContext();
        auto module = std::make_unique<llvm::Module>("review1_proc_label", *context);

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        module->print(llvm::errs(), nullptr);
        REQUIRE_FALSE(llvm::verifyModule(*module, &llvm::errs()));

        module.release();
    }

    SECTION("Procedure Switch Parameter Execution")
    {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("review/1/proc_switch.cor", "BACKEND_DIR");
        auto source = readFile(sourcePath);
        REQUIRE_FALSE(source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);

        bool errors = false;
        auto ppTokens = coral66::lexer::tokenize(source, &ss, &errors);
        if (errors)
        {
            std::cerr << "LEXER ERROR: " << ss.str() << std::endl;
        }
        REQUIRE_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        if (errors)
            WARN(ss.str());
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

        auto* context = new llvm::LLVMContext();
        auto module = std::make_unique<llvm::Module>("review1_proc_switch", *context);

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        module->print(llvm::errs(), nullptr);
        REQUIRE_FALSE(llvm::verifyModule(*module, &llvm::errs()));

        module.release();
    }

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
        if (errors)
        {
            std::cerr << "LEXER ERROR: " << ss.str() << std::endl;
        }
        REQUIRE_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        if (errors)
            WARN(ss.str());
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
        module->print(llvm::errs(), nullptr);

        std::string engineError;
        llvm::ExecutionEngine* ee = llvm::EngineBuilder(std::move(module))
                                        .setErrorStr(&engineError)
                                        .setEngineKind(llvm::EngineKind::JIT)
                                        .create();
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
        if (errors)
        {
            std::cerr << "LEXER ERROR: " << ss.str() << std::endl;
        }
        REQUIRE_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        if (errors)
            WARN(ss.str());
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
        module->print(llvm::errs(), nullptr);

        std::string engineError;
        llvm::ExecutionEngine* ee = llvm::EngineBuilder(std::move(module))
                                        .setErrorStr(&engineError)
                                        .setEngineKind(llvm::EngineKind::JIT)
                                        .create();
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
        if (errors)
        {
            std::cerr << "LEXER ERROR: " << ss.str() << std::endl;
        }
        REQUIRE_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        if (errors)
            WARN(ss.str());
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
        module->print(llvm::errs(), nullptr);

        std::string engineError;
        llvm::ExecutionEngine* ee = llvm::EngineBuilder(std::move(module))
                                        .setErrorStr(&engineError)
                                        .setEngineKind(llvm::EngineKind::JIT)
                                        .create();
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
    SECTION("Table Bit Packing Execution")
    {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("review/1/table_bit_packing.cor", "BACKEND_DIR");
        auto source = readFile(sourcePath);
        REQUIRE_FALSE(source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);

        bool errors = false;
        auto ppTokens = coral66::lexer::tokenize(source, &ss, &errors);
        if (errors)
        {
            std::cerr << "LEXER ERROR: " << ss.str() << std::endl;
        }
        REQUIRE_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        if (errors)
            WARN(ss.str());
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
        auto module = std::make_unique<llvm::Module>("test_module", context);

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        REQUIRE_FALSE(llvm::verifyModule(*module, &llvm::errs()));

        module->print(llvm::errs(), nullptr);
        std::string engineError;
        llvm::ExecutionEngine* ee = llvm::EngineBuilder(std::move(module))
                                        .setErrorStr(&engineError)
                                        .setEngineKind(llvm::EngineKind::JIT)
                                        .create();
        if (!ee)
        {
            llvm::errs() << "JIT Error: " << engineError << "\n";
        }
        REQUIRE(ee != nullptr);

        auto mainFunc = ee->FindFunctionNamed("main");
        if (!mainFunc)
            mainFunc = ee->FindFunctionNamed("TEST");
        REQUIRE(mainFunc != nullptr);

        ee->finalizeObject();
        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        auto ansAddr = reinterpret_cast<void*>(ee->getGlobalValueAddress("ans"));
        REQUIRE(ansAddr != nullptr);

        int32_t finalAns = *static_cast<int32_t*>(ansAddr);
        REQUIRE(finalAns == 1);

        delete ee;
    }

    SECTION("Preset Data Execution")
    {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("review/1/preset_data.cor", "BACKEND_DIR");
        auto source = readFile(sourcePath);
        REQUIRE_FALSE(source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);

        bool errors = false;
        auto ppTokens = coral66::lexer::tokenize(source, &ss, &errors);
        if (errors)
        {
            std::cerr << "LEXER ERROR: " << ss.str() << std::endl;
        }
        REQUIRE_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        if (errors)
            WARN(ss.str());
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
        auto module = std::make_unique<llvm::Module>("test_module", context);

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        REQUIRE_FALSE(llvm::verifyModule(*module, &llvm::errs()));

        module->print(llvm::errs(), nullptr);
        std::string engineError;
        llvm::ExecutionEngine* ee = llvm::EngineBuilder(std::move(module))
                                        .setErrorStr(&engineError)
                                        .setEngineKind(llvm::EngineKind::JIT)
                                        .create();
        if (!ee)
        {
            llvm::errs() << "JIT Error: " << engineError << "\n";
        }
        REQUIRE(ee != nullptr);

        auto mainFunc = ee->FindFunctionNamed("main");
        if (!mainFunc)
            mainFunc = ee->FindFunctionNamed("TEST");
        REQUIRE(mainFunc != nullptr);

        ee->finalizeObject();
        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        auto ansAddr = reinterpret_cast<void*>(ee->getGlobalValueAddress("ans"));
        REQUIRE(ansAddr != nullptr);

        int32_t finalAns = *static_cast<int32_t*>(ansAddr);
        REQUIRE(finalAns == 1);

        delete ee;
    }
}
