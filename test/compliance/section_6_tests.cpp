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

        // floatLiteral
        variableAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("floatLiteral"));
        REQUIRE(variableAddress != nullptr);

        variable = *static_cast<float*>(variableAddress);
        CHECK(variable == 10.0);

        // integerLiteral
        variableAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("integerLiteral"));
        REQUIRE(variableAddress != nullptr);

        variable = *static_cast<int32_t*>(variableAddress);
        CHECK(variable == 90);

        // Clean up
        delete ee;
    }

    /// Any typed primary other than FLOATING is a sequence of bits to which the
    /// logical operations DIFFER, UNION and MASK may be applied, bit by bit, to
    /// produce an expression of the form Differ, where every bit takes part in the
    /// operation, including any sign bit, and the result as a whole has the numeric
    /// type INTEGER. The binding priorities are in the increasing order: DIFFER,
    /// UNION, MASK.
    SECTION("6-1-3")
    {
        // Initialize LLVM Native Target for JIT Execution
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("section-6/6-1-3.cor", "COMPLIANCE_DATA_DIR");
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
        auto module = std::make_unique<llvm::Module>("sixonethree", context);
        REQUIRE(module.get() != nullptr); // Needs to be REQUIRE

        coral66::cgllvm::Options codegenOptions;
        auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

        CHECK_FALSE(llvm::verifyModule(*module, &llvm::errs()));

        // JIT Execution
        std::string errStr;
        llvm::ExecutionEngine* ee =
            llvm::EngineBuilder(std::move(module)).setErrorStr(&errStr).setEngineKind(llvm::EngineKind::JIT).create();
        REQUIRE(ee != nullptr); // Needs to be REQUIRE

        ee->finalizeObject();

        llvm::Function* mainFunc = ee->FindFunctionNamed("main");
        REQUIRE(mainFunc != nullptr); // Needs to be REQUIRE

        std::vector<llvm::GenericValue> noArgs;
        ee->runFunction(mainFunc, noArgs);

        auto getGlobalInt = [&](const char* name) -> int32_t {
            auto addr = reinterpret_cast<int32_t*>(ee->getGlobalValueAddress(name));
            REQUIRE(addr != nullptr);
            return *addr;
        };

        // Truth tables: 12 (1100b) and 10 (1010b)
        CHECK(getGlobalInt("resDiffer") == 6); // 0110b
        CHECK(getGlobalInt("resUnion") == 14); // 1110b
        CHECK(getGlobalInt("resMask") == 8);   // 1000b

        // Sign bit participation
        CHECK(getGlobalInt("resSignMask") == static_cast<int32_t>(0x80000000u));
        CHECK(getGlobalInt("resSignDiffer") == 0x7FFFFFFF);

        // Binding priorities: 6 DIFFER (3 UNION (5 MASK 12)) = 6 ^ (3 | 4) = 1
        CHECK(getGlobalInt("resPrecedence") == 1);

        // Grouping via INTEGER(...): 12 MASK INTEGER(3 UNION 5) = 12 & 7 = 4
        CHECK(getGlobalInt("resGrouped") == 4);

        // Typed primaries: FIXED(16,4) 1.5 (raw 24) MASK 8 = 8
        CHECK(getGlobalInt("resFixedMask") == 8);
        // BITS [4, 2] 12 (3) UNION LITERAL(A) (65) = 67
        CHECK(getGlobalInt("resPartWordUnion") == 67);
        // LOCATION(a) MASK 0 = 0
        CHECK(getGlobalInt("resLocMask") == 0);

        // INTEGER result arithmetic and constant evaluation
        CHECK(getGlobalInt("resArith") == 20);
        CHECK(getGlobalInt("resConstBits") == 1);

        // Clean up
        delete ee;
    }

    /// 6.1.3 - For example, if i and j are integer references and x is a
    /// floating-point reference, the assignment statement
    /// x <- i - j
    ///
    /// causes i and j to be converted to floating-point before subtraction, whilst
    /// x <- INTEGER ( i - j )
    ///
    /// causes subtraction of integers before conversion to floating-point.
    SECTION("6-1-4")
    {
        // Initialize LLVM Native Target for JIT Execution
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("section-6/6-1-4.cor", "COMPLIANCE_DATA_DIR");
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
        auto module = std::make_unique<llvm::Module>("sixonefour", context);
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

        // x
        auto variableAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("x"));
        REQUIRE(variableAddress != nullptr);

        auto variable = *static_cast<float*>(variableAddress);
        CHECK(variable == 20.0f);

        // y
        variableAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("y"));
        REQUIRE(variableAddress != nullptr);

        variable = *static_cast<float*>(variableAddress);
        CHECK(variable == 20.0f);

        // Clean up
        delete ee;
    }
}
