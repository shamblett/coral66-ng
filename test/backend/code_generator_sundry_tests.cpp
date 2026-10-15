/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 02/09/2026
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
/// Sundry : Ad hoc tests
///
TEST_CASE("Sundry", "[Backend][Execution]")
{
    SECTION("Step Until")
    {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("sundry/step.cor", "BACKEND_DIR");
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
        auto module = std::make_unique<llvm::Module>("step", context);

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        module->print(llvm::errs(), nullptr);
        REQUIRE_FALSE(llvm::verifyModule(*module, &llvm::errs()));
        module->print(llvm::errs(), nullptr);
        module->print(llvm::errs(), nullptr);

        std::string errStr;
        llvm::ExecutionEngine* ee =
            llvm::EngineBuilder(std::move(module)).setErrorStr(&errStr).setEngineKind(llvm::EngineKind::JIT).create();
        REQUIRE(ee != nullptr);
        ee->finalizeObject();

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        if (!mainFunc)
            mainFunc = ee->FindFunctionNamed("Step");
        REQUIRE(mainFunc != nullptr);

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        // Cycle no
        auto cyclenoAddr = reinterpret_cast<void*>(ee->getGlobalValueAddress("cycleno"));
        REQUIRE(cyclenoAddr != nullptr);
        int32_t cyclenoAns = *static_cast<int32_t*>(cyclenoAddr);
        REQUIRE(cyclenoAns == 11);

        // Loop count
        auto loopcountAddr = reinterpret_cast<void*>(ee->getGlobalValueAddress("loopcount"));
        REQUIRE(loopcountAddr != nullptr);
        int32_t loopcountAns = *static_cast<int32_t*>(loopcountAddr);
        REQUIRE(loopcountAns == 10);

        delete ee;
    }

    SECTION("Octal Literals and Octal Points")
    {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        const std::string source = R"(
        PROGRAM OctalTest
        BEGIN
            INTEGER octint := OCTAL(1234);
            FLOATING octfl := OCTAL(12.34);
            FLOATING octexpr;
            FIXED(16, 4) octfx := OCTAL(1.4);
            FIXED(16, 4) octnegfx := -OCTAL(1.4);
            octexpr := OCTAL(1.4) + OCTAL(0.2);
        END;
        )";

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);

        bool errors = false;
        auto ppTokens = coral66::lexer::tokenize(source, &ss, &errors);
        if (errors)
            WARN(ss.str());
        REQUIRE_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        if (errors)
            WARN(ss.str());
        REQUIRE_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        if (errors)
            WARN(ss.str());
        REQUIRE_FALSE(errors);

        auto parsed = coral66::parser::parseProgram(cTokens, &ss, &errors);
        if (errors)
            WARN(ss.str());
        REQUIRE_FALSE(errors);

        auto program = coral66::semantics::analyse(parsed, std::move(cTokens), &ss, &errors);
        if (errors)
            WARN(ss.str());
        REQUIRE_FALSE(errors);

        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("octal_test", context);

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);
        REQUIRE_FALSE(llvm::verifyModule(*module, &llvm::errs()));

        std::string errStr;
        llvm::ExecutionEngine* ee =
            llvm::EngineBuilder(std::move(module)).setErrorStr(&errStr).setEngineKind(llvm::EngineKind::JIT).create();
        REQUIRE(ee != nullptr);
        ee->finalizeObject();

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        if (!mainFunc)
            mainFunc = ee->FindFunctionNamed("OctalTest");
        REQUIRE(mainFunc != nullptr);

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        auto octintAddr = reinterpret_cast<int32_t*>(ee->getGlobalValueAddress("octint"));
        REQUIRE(octintAddr != nullptr);
        CHECK(*octintAddr == 01234);

        auto octflAddr = reinterpret_cast<float*>(ee->getGlobalValueAddress("octfl"));
        REQUIRE(octflAddr != nullptr);
        CHECK(*octflAddr == 10.4375f);

        auto octexprAddr = reinterpret_cast<float*>(ee->getGlobalValueAddress("octexpr"));
        REQUIRE(octexprAddr != nullptr);
        CHECK(*octexprAddr == 1.75f);

        auto octfxAddr = reinterpret_cast<int32_t*>(ee->getGlobalValueAddress("octfx"));
        REQUIRE(octfxAddr != nullptr);
        CHECK(*octfxAddr == 24); // 1.5 * 2^4 = 24

        auto octnegfxAddr = reinterpret_cast<int32_t*>(ee->getGlobalValueAddress("octnegfx"));
        REQUIRE(octnegfxAddr != nullptr);
        CHECK(*octnegfxAddr == -24); // -1.5 * 2^4 = -24

        delete ee;
    }

    SECTION("PartWord Assignment with Fixed-Point RHS")
    {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        const std::string source = R"(
        PROGRAM PartWordFixedTest
        BEGIN
            INTEGER word := 1;
            INTEGER extracted := 0;
            FIXED(16, 4) fx := 3.5;
            BITS[4, 4] word := fx;
            extracted := BITS[4, 4] word;
        END;
        )";

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);

        bool errors = false;
        auto ppTokens = coral66::lexer::tokenize(source, &ss, &errors);
        if (errors)
            WARN(ss.str());
        REQUIRE_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        if (errors)
            WARN(ss.str());
        REQUIRE_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        if (errors)
            WARN(ss.str());
        REQUIRE_FALSE(errors);

        auto parsed = coral66::parser::parseProgram(cTokens, &ss, &errors);
        if (errors)
            WARN(ss.str());
        REQUIRE_FALSE(errors);

        auto program = coral66::semantics::analyse(parsed, std::move(cTokens), &ss, &errors);
        if (errors)
            WARN(ss.str());
        REQUIRE_FALSE(errors);

        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("partword_fixed_test", context);

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);
        REQUIRE_FALSE(llvm::verifyModule(*module, &llvm::errs()));

        std::string errStr;
        llvm::ExecutionEngine* ee =
            llvm::EngineBuilder(std::move(module)).setErrorStr(&errStr).setEngineKind(llvm::EngineKind::JIT).create();
        REQUIRE(ee != nullptr);
        ee->finalizeObject();

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        if (!mainFunc)
            mainFunc = ee->FindFunctionNamed("PartWordFixedTest");
        REQUIRE(mainFunc != nullptr);

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        auto wordAddr = reinterpret_cast<int32_t*>(ee->getGlobalValueAddress("word"));
        REQUIRE(wordAddr != nullptr);
        CHECK(*wordAddr == 49); // bit 0 preserved (1) | (int(3.5) << 4) = 1 | 48 = 49

        auto extractedAddr = reinterpret_cast<int32_t*>(ee->getGlobalValueAddress("extracted"));
        REQUIRE(extractedAddr != nullptr);
        CHECK(*extractedAddr == 3);

        delete ee;
    }
}
