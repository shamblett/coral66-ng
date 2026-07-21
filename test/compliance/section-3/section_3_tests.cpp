/*
 * Project: coral66
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
#include "test/common/test_config.hpp"
#include "test/compliance/common/common.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace Catch::Matchers;
using namespace coral66::errors::semantics;

///
/// Section 3
///
TEST_CASE("Section 3", "[Compliance][Section-3]")
{
    /// If two objects have the same name and their scopes overlap, the clash of
    /// definitions could give rise to ambiguity. Typically, a clash occurs when an
    /// inner block is opened and a local object is declared to have the same name
    /// as a non-local object which already exists. In this situation, the non-local
    /// object continues to exist through the inner block (e.g. a variable maintains its
    /// value), but it becomes temporarily inaccessible. The local meaning of the
    /// identifier always takes precedence.
    SECTION("3-2")
    {
        // Initialize LLVM Native Target for JIT Execution
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("3-2.cor", "COMPLIANCE_SECTION_3_DIR");
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
        auto module = std::make_unique<llvm::Module>("twoone", context);
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

        // The ExecutionEngine took ownership of the module, but we can access globals by name

        // Outer answer
        auto outerAnswerAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("outerAnswer"));

        REQUIRE(outerAnswerAddress != nullptr);

        // Dereference to get the actual value
        int32_t outerAnswer = *static_cast<int32_t*>(outerAnswerAddress);

        REQUIRE(outerAnswer == 10);

        // Inner answer
        auto innerAnswerAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("innerAnswerValue"));

        REQUIRE(innerAnswerAddress != nullptr);

        // Dereference to get the actual value
        int32_t innerAnswer = *static_cast<int32_t*>(innerAnswerAddress);

        REQUIRE(innerAnswer == 21);
        // Clean up
        delete ee;
    }
}
