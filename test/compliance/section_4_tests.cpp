/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 21/07/2026
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
/// Section 4
///
TEST_CASE("Section 4", "[Compliance][Section-4]")
{
    /// There are three types of number, floating-point, fixed-point and integer.
    /// Except in certain part-word table-elements (section 4.4.2.2), all three types
    /// are signed. Numeric type is indicated by the word FLOATING or INTEGER,
    /// or by the word FIXED followed by scaling constants which must be given
    /// numerically.
    /// It is assumedthroughout this definition that a number is confined within a
    /// single computer word which in this implementation is 32 bits long.
    SECTION("4-1-1")
    {
        // Initialize LLVM Native Target for JIT Execution
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("section-4/4-1-1.cor", "COMPLIANCE_DATA_DIR");
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
        auto module = std::make_unique<llvm::Module>("fouroneone", context);
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

        // Integer
        uint64_t rawAddress = ee->getGlobalValueAddress("theInteger");
        if (const llvm::GlobalValue* gVal = ee->getGlobalValueAtAddress(reinterpret_cast<void*>(rawAddress)))
        {
            const llvm::DataLayout& dl = ee->getDataLayout();
            uint64_t sizeInBytes = dl.getTypeAllocSize(gVal->getValueType()).getFixedValue();
            CHECK(sizeInBytes == 5);
        }

        // Floating
        rawAddress = ee->getGlobalValueAddress("theFloat");
        if (const llvm::GlobalValue* gVal = ee->getGlobalValueAtAddress(reinterpret_cast<void*>(rawAddress)))
        {
            const llvm::DataLayout& dl = ee->getDataLayout();
            uint64_t sizeInBytes = dl.getTypeAllocSize(gVal->getValueType()).getFixedValue();
            CHECK(sizeInBytes == 5);
        }

        // Fixed
        rawAddress = ee->getGlobalValueAddress("theFixed");
        if (const llvm::GlobalValue* gVal = ee->getGlobalValueAtAddress(reinterpret_cast<void*>(rawAddress)))
        {
            const llvm::DataLayout& dl = ee->getDataLayout();
            uint64_t sizeInBytes = dl.getTypeAllocSize(gVal->getValueType()).getFixedValue();
            CHECK(sizeInBytes == 5);
        }

        // Clean up
        delete ee;
    }

    /// A fixed Numeric type is indicated by the word FIXED followed by scaling
    /// constants which must be given numerically, e.g.

    /// FIXED (13,5)

    /// This specifies five fractional bits and a minimum of 13 bits to represent the
    /// number as a whole, including the fractional bits and a sign.
    SECTION("4-1-2")
    {
        // Initialize LLVM Native Target for JIT Execution
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("section-4/4-1-2.cor", "COMPLIANCE_DATA_DIR");
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
        auto module = std::make_unique<llvm::Module>("fouronetwo", context);
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

        // f1351
        auto fixedAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("f1351"));
        REQUIRE(fixedAddress != nullptr);
        // Dereference to get the actual value
        int32_t fixed = *static_cast<int32_t*>(fixedAddress);
        /// 8.75 * 32 = 280
        CHECK(fixed == 280);

        // f1352
        fixedAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("f1352"));
        REQUIRE(fixedAddress != nullptr);
        // Dereference to get the actual value
        fixed = *static_cast<int32_t*>(fixedAddress);
        /// 87.5 * 32 = 2800
        CHECK(fixed == 2800);

        // f1353
        fixedAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("f1353"));
        REQUIRE(fixedAddress != nullptr);
        // Dereference to get the actual value
        fixed = *static_cast<int32_t*>(fixedAddress);
        /// 875 * 32 = 28000
        CHECK(fixed == 28000);

        // f1354 - same as f1353 but RHS is an integer not a float
        fixedAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("f1354"));
        REQUIRE(fixedAddress != nullptr);
        // Dereference to get the actual value
        fixed = *static_cast<int32_t*>(fixedAddress);
        /// 875 * 32 = 28000
        CHECK(fixed == 28000);
        // Clean up
        delete ee;
    }

    /// An array is restricted to a one or two dimensional set of numbers all of the
    /// same type (including scale for fixed-point).
    SECTION("4-3-1")
    {
        // Initialize LLVM Native Target for JIT Execution
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("section-4/4-3-1.cor", "COMPLIANCE_DATA_DIR");
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
        CHECK(errors);
        
        CHECK(reporterString.find("Type mismatch in expression") != std::string::npos);
        CHECK(reporterString.find("b[0] :=") == std::string::npos);
        CHECK(reporterString.find("b[3] :=") != std::string::npos);
    }
}
