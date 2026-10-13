/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 13/10/2026
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
#include "lib/frontend/semantics/semantic_support.hpp"
#include "llvm/IR/DataLayout.h"
#include "test/common/test_config.hpp"
#include "test/compliance/common/common.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace Catch::Matchers;
using namespace coral66::errors::semantics;

///
/// Section 4
///
TEST_CASE("Section 5", "[Compliance][Section-5]")
{
    /// Place references refer to positions of program statements, and the simplest
    /// position marker is the label (section 3.4). A switch is a preset and unalterable
    /// array of labels, which must be within scope at the switch declaration. Any use
    /// of the indexed switch name refers to the corresponding label.
    SECTION("5-1")
    {
        // Initialize LLVM Native Target for JIT Execution
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("section-5/5-1.cor", "COMPLIANCE_DATA_DIR");
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
        auto module = std::make_unique<llvm::Module>("fiveone", context);
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

        bool checkInteger = false;
        bool checkFloating = false;
        bool checkFixed = false;

        // Integer
        if (const llvm::GlobalVariable* gVal = ee->FindGlobalVariableNamed("theInteger"))
        {
            const llvm::DataLayout& dl = ee->getDataLayout();
            uint64_t sizeInBytes = dl.getTypeAllocSize(gVal->getValueType()).getFixedValue();
            CHECK(sizeInBytes == 4);
            checkInteger = true;
        }

        // Floating
        if (const llvm::GlobalVariable* gVal = ee->FindGlobalVariableNamed("theFloat"))
        {
            const llvm::DataLayout& dl = ee->getDataLayout();
            uint64_t sizeInBytes = dl.getTypeAllocSize(gVal->getValueType()).getFixedValue();
            CHECK(sizeInBytes == 4);
            checkFloating = true;
        }

        // Fixed
        if (const llvm::GlobalVariable* gVal = ee->FindGlobalVariableNamed("theFixed"))
        {
            const llvm::DataLayout& dl = ee->getDataLayout();
            uint64_t sizeInBytes = dl.getTypeAllocSize(gVal->getValueType()).getFixedValue();
            CHECK(sizeInBytes == 4);
            checkFixed = true;
        }

        CHECK(checkInteger);
        CHECK(checkFloating);
        CHECK(checkFixed);

        // Clean up
        delete ee;
    }

}
