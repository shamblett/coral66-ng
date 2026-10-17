/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 16/10/2026
 */

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>

#include <iostream>

#include "lib/backend/code_generation.hpp"
#include "lib/frontend/common/messages/errors.hpp"
#include "lib/frontend/lexer/lexer.hpp"
#include "lib/frontend/parser/parser.hpp"
#include "lib/frontend/preprocessor/preprocessor.hpp"
#include "lib/frontend/semantics/program.hpp"
#include "llvm/IR/DataLayout.h"
#include "test/common/test_config.hpp"
#include "test/compliance/common/common.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace Catch::Matchers;
using namespace coral66::errors::semantics;

///
/// Section 6
///
TEST_CASE("Section 6", "[Compliance][Section-6]")
{
    /// The computer location of any word reference is obtainable by the location
    /// operator which is written in the form
    ///
    ///  LOCATION ( Wordreference )
    ///
    /// and has a value of type INTEGER.
    /// Note: In this immplementation a LOCATION returns a POINTER type with a length of 64 bits.
    SECTION("6-1-1")
    {
        // Initialize LLVM Native Target for JIT Execution
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("section-6/6-1-1.cor", "COMPLIANCE_DATA_DIR");
        auto source = readFile(sourcePath);
        CHECK_FALSE(source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);

        // Frontend passes
        bool errors = false;
        auto ppTokens = coral66::lexer::tokenize(source, &ss, &errors);
        CHECK_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        CHECK_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        CHECK_FALSE(errors);

        auto parsed = coral66::parser::parseProgram(cTokens, &ss, &errors);
        CHECK_FALSE(errors);

        auto program = coral66::semantics::analyse(parsed, std::move(cTokens), &ss, &errors);
        CHECK_FALSE(errors);

        // Code Generation
        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("sixoneone", context);
        REQUIRE(module.get() != nullptr); // Needs to be REQUIRE

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        CHECK_FALSE(llvm::verifyModule(*module, &llvm::errs()));

        // JIT Execution
        std::string errStr;
        llvm::ExecutionEngine* ee =
            llvm::EngineBuilder(std::move(module)).setErrorStr(&errStr).setEngineKind(llvm::EngineKind::JIT).create();
        REQUIRE(ee != nullptr); // Needs to be REQUIRE

        // Run the `main` function

        // Ensure all globals are allocated and JIT compilation is done before running
        ee->finalizeObject();

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        REQUIRE(mainFunc != nullptr); // Needs to be REQUIRE

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        // baseAddr
        if (const llvm::GlobalVariable* gVal = ee->FindGlobalVariableNamed("baseAddr"))
        {
            const llvm::DataLayout& dl = ee->getDataLayout();
            uint64_t sizeInBytes = dl.getTypeAllocSize(gVal->getValueType()).getFixedValue();
            CHECK(sizeInBytes == 8);
        }

        // Verify that LOCATION(base) actually stored the 64-bit runtime address of base
        uint64_t baseAddress = ee->getGlobalValueAddress("base");
        REQUIRE(baseAddress != 0);

        auto* baseAddrPtr = reinterpret_cast<uint64_t*>(ee->getGlobalValueAddress("baseAddr"));
        REQUIRE(baseAddrPtr != nullptr);
        CHECK(*baseAddrPtr == baseAddress);

        // Dereference the 64-bit address stored in baseAddr to verify it points to base (20)
        auto* deRefBase = reinterpret_cast<int32_t*>(*baseAddrPtr);
        REQUIRE(deRefBase != nullptr);
        CHECK(*deRefBase == 20);

        // Clean up
        delete ee;
    }

    /// A typed primary may have its type changed, and an untyped primary may
    /// be typed, by enclosure within round brackets preceded by a specific Number-
    /// type as described in section 6.1.3.
    SECTION("6-1-2")
    {
        // Initialize LLVM Native Target for JIT Execution
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("section-6/6-1-2.cor", "COMPLIANCE_DATA_DIR");
        auto source = readFile(sourcePath);
        CHECK_FALSE(source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);

        // Frontend passes
        bool errors = false;
        auto ppTokens = coral66::lexer::tokenize(source, &ss, &errors);
        CHECK_FALSE(errors);

        ppTokens = coral66::pp::preprocess(std::move(ppTokens), &ss, &errors);
        CHECK_FALSE(errors);

        auto cTokens = coral66::lexer::toCTokens(std::move(ppTokens), &ss, &errors);
        CHECK_FALSE(errors);

        auto parsed = coral66::parser::parseProgram(cTokens, &ss, &errors);
        CHECK_FALSE(errors);
        std::cout << ss.str() << std::endl;

        auto program = coral66::semantics::analyse(parsed, std::move(cTokens), &ss, &errors);
        CHECK_FALSE(errors);

        // Code Generation
        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("sixonetwo", context);
        REQUIRE(module.get() != nullptr); // Needs to be REQUIRE

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        CHECK_FALSE(llvm::verifyModule(*module, &llvm::errs()));

        // JIT Execution
        std::string errStr;
        llvm::ExecutionEngine* ee =
            llvm::EngineBuilder(std::move(module)).setErrorStr(&errStr).setEngineKind(llvm::EngineKind::JIT).create();
        REQUIRE(ee != nullptr); // Needs to be REQUIRE

        // Run the `main` function

        // Ensure all globals are allocated and JIT compilation is done before running
        ee->finalizeObject();

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        REQUIRE(mainFunc != nullptr); // Needs to be REQUIRE

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        // floatBase
        auto variableAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("floatBase"));
        REQUIRE(variableAddress != nullptr);

        auto variable = *static_cast<float*>(variableAddress);
        CHECK(variable == 280.0);

        // fixedBase
        variableAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("fixedBase"));
        REQUIRE(variableAddress != nullptr);

        auto fixedRaw = *static_cast<int32_t*>(variableAddress);
        float fixedFloat = static_cast<float>(fixedRaw) / static_cast<float>(1 << 5); // 280 / 32.0f = 8.75f
        CHECK(fixedFloat == 280.0f);

        // Clean up
        delete ee;
    }


}
