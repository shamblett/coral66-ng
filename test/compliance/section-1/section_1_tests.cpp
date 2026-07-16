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

#include <sys/wait.h>
#include <unistd.h>

#include "lib/backend/code_generation.hpp"
#include "lib/frontend/common/messages/errors.hpp"
#include "lib/frontend/lexer/lexer.hpp"
#include "lib/frontend/parser/parser.hpp"
#include "lib/frontend/preprocessor/preprocessor.hpp"
#include "lib/frontend/semantics/program.hpp"
#include "lib/frontend/semantics/semantic_support.hpp"
#include "test/common/test_config.hpp"
#include "test/compliance/common/common.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace Catch::Matchers;
using namespace coral66::errors::semantics;

///
/// Section 1.4
///
TEST_CASE("Section 1-4", "[Compliance][Section-1-4]")
{
    /// The insistence that identifiers are fully declared or specified before
    /// use simplifies the compiler by ensuring that all relevant information is
    /// available when required.
    SECTION("1-4-1")
    {
        auto sourcePath = getTestDataPath("1-4-1.cor", "COMPLIANCE_SECTION_1_DIR");
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
        CHECK(coral66::semantics::DiagnosticCollector::getDiagnostics().size() == 2);
        CHECK(coral66::semantics::DiagnosticCollector::findDiagnostic(UNDECLARED_IDENTIFIER_N));
        CHECK(coral66::semantics::DiagnosticCollector::findDiagnostic(CANNOT_ASSIGN_TO_CONST));
    }

    /// Unless run in a special
    /// diagnostic mode, a Coral 66 compiler is not expected to generate run-time
    /// checks on subscript bounds.
    ///
    /// If the access is outside the sllocated memory space of the program a sigsev is generated.
    SECTION("1-4-2")
    {
        pid_t pid = fork();

        REQUIRE(pid >= 0); // Ensure fork succeeded

        if (pid == 0)
        {
            // Child process: execute the crashing code

            // Initialize LLVM Native Target for JIT Execution
            llvm::InitializeNativeTarget();
            llvm::InitializeNativeTargetAsmPrinter();

            auto sourcePath = getTestDataPath("1-4-2.cor", "COMPLIANCE_SECTION_1_DIR");
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
            auto module = std::make_unique<llvm::Module>("onefourtwo", context);
            REQUIRE(module.get() != nullptr); // Needs to be REQUIRE

            coral66::cgllvm::Options codegenOptions;
            auto targetMachine = coral66::cgllvm::generateLLVM(*module, parsed, program, codegenOptions);

            CHECK_FALSE(llvm::verifyModule(*module, &llvm::errs()));

            // JIT Execution
            std::string errStr;
            llvm::ExecutionEngine* ee = llvm::EngineBuilder(std::move(module))
                                            .setErrorStr(&errStr)
                                            .setEngineKind(llvm::EngineKind::JIT)
                                            .create();
            REQUIRE(ee != nullptr); // Needs to be REQUIRE

            // Run the `main` function

            // Ensure all globals are allocated and JIT compilation is done before running
            ee->finalizeObject();

            llvm::Function* mainFunc = ee->FindFunctionNamed("main");
            REQUIRE(mainFunc != nullptr); // Needs to be REQUIRE

            std::vector<llvm::GenericValue> noArgs;
            ee->runFunction(mainFunc, noArgs);
        }
        else
        {
            // Parent process: monitor child behavior
            int status;
            waitpid(pid, &status, 0);

            // Assert that the child process was terminated by a signal
            CHECK(WIFSIGNALED(status));
            // Assert specifically that the signal was SIGSEGV (11)
            CHECK(WTERMSIG(status) == SIGSEGV);
        }
    }

    /// Unless run in a special
    /// diagnostic mode, a Coral 66 compiler is not expected to generate run-time
    /// checks on subscript bounds.
    ///
    /// If the access is inside the sllocated memory space of the program UB is triggered.
    SECTION("1-4-3")
    {
        // Initialize LLVM Native Target for JIT Execution
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("1-4-3.cor", "COMPLIANCE_SECTION_1_DIR");
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
        auto module = std::make_unique<llvm::Module>("onefourthree", context);
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
        auto resultAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("result"));

        REQUIRE(resultAddress != nullptr);

        // Dereference to get the actual value
        int32_t result = *static_cast<int32_t*>(resultAddress);

        // Happens to be 0 in a newly created module
        REQUIRE(result == 0);

        // Clean up
        delete ee;
    }
}
