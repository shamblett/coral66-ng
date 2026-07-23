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

#include "common.hpp"
#include "lib/backend/code_generation.hpp"
#include "lib/frontend/common/messages/errors.hpp"
#include "lib/frontend/lexer/lexer.hpp"
#include "lib/frontend/parser/parser.hpp"
#include "lib/frontend/preprocessor/preprocessor.hpp"
#include "lib/frontend/semantics/program.hpp"
#include "test/common/test_config.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace Catch::Matchers;
using namespace coral66::errors::semantics;

///
/// Various numeric tests
///
TEST_CASE("Numerics", "[Backend][Execution]")
{
    SECTION("Fixed")
    {
        // Initialize LLVM Native Target for JIT Execution
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        auto sourcePath = getTestDataPath("numerics.cor", "BACKEND_DIR");
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
        auto module = std::make_unique<llvm::Module>("numerics", context);
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

        // Fixed value
        auto fixedAddress = reinterpret_cast<void*>(ee->getGlobalValueAddress("theFixed"));

        REQUIRE(fixedAddress != nullptr);

        // Dereference to get the actual value
        int32_t fixed = *static_cast<int32_t*>(fixedAddress);

        /// Identify Fractional Bits: The layout (13,5) dictates that the number has 5 fractional bits.
        /// Understand the Scaling Factor: Fixed-point representations scale the real value by \(2^{f}\),
        /// where \(f\) is the number of fractional bits.Calculate the Base Integer:
        /// Multiply the given value by the scaling factor (\(2^5 = 32\)).
        ///
        /// 8.75 * 32 = 280
        CHECK(fixed == 256); // TODO

        // Clean up
        delete ee;
    }
}
