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
/// Phase 6: Advanced Data Structures
///
TEST_CASE("Phase 6", "[Backend][Execution]")
{
    SECTION("Whetstone End-to-End Execution")
    {
        // TODO
        //SKIP();
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("p6/whetstone/whetstone.cor", "BACKEND_DIR");
        auto source = readFile(sourcePath);
        REQUIRE_FALSE(source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);

        bool errors = false;
        auto ppTokens = coral66::lexer::tokenize(source, &ss, &errors);
        if (errors) { std::cerr << "LEXER ERROR: " << ss.str() << std::endl; }
        REQUIRE_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        if (errors) WARN(ss.str());
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
        if (errors) { std::cerr << "LEXER ERROR: " << ss.str() << std::endl; }
        REQUIRE_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        REQUIRE_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        if (errors) WARN(ss.str());
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
