/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 04/07/2026
 */

#include <llvm/IR/Verifier.h>

#include "lib/backend/code_generation.hpp"
#include "test/backend/common.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66;

TEST_CASE("Code Generator Absolute Communicator", "[CodeGenerator][Absolute]")
{
    SECTION("Generates Volatile Accesses in Baremetal Mode - Integer")
    {
        auto [syntaxTree, program] =
            generateProgramPair("PROGRAM MAIN ABSOLUTE ( INTEGER X / 1024 ) BEGIN X := 10; END");

        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("coral_module", context);

        cgllvm::Options codegenOptions;
        codegenOptions.baremetal = true;

        auto targetMachine = cgllvm::generateLLVM(*module, syntaxTree, program, codegenOptions);
        REQUIRE_FALSE(llvm::verifyModule(*module, &llvm::errs()));

        // Inspect IR using the ContainsIR matcher
        CHECK_THAT(*module, ContainsIR("store volatile i32 10, ptr inttoptr \\(i64 1024 to ptr\\)"));
        CHECK_THAT(*module, ContainsIR("inttoptr \\(i64 1024 to ptr\\)"));
    }

    SECTION("Generates Volatile Accesses in Baremetal Mode - Integer Hex Value")
    {
        auto [syntaxTree, program] =
            generateProgramPair("PROGRAM MAIN ABSOLUTE ( INTEGER X / 0x400 ) BEGIN X := 10; END");

        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("coral_module", context);

        cgllvm::Options codegenOptions;
        codegenOptions.baremetal = true;

        auto targetMachine = cgllvm::generateLLVM(*module, syntaxTree, program, codegenOptions);
        REQUIRE_FALSE(llvm::verifyModule(*module, &llvm::errs()));

        // Inspect IR using the ContainsIR matcher
        CHECK_THAT(*module, ContainsIR("store volatile i32 10, ptr inttoptr \\(i64 1024 to ptr\\)"));
        CHECK_THAT(*module, ContainsIR("inttoptr \\(i64 1024 to ptr\\)"));
    }

    SECTION("Generates Volatile Accesses in Baremetal Mode - Array")
    {
        auto [syntaxTree, program] = generateProgramPair(
            "PROGRAM MAIN ABSOLUTE ( INTEGER X / 1024; ARRAY A / 2048 [1:10]; ) BEGIN X := A[1]; END");

        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("coral_module", context);

        cgllvm::Options codegenOptions;
        codegenOptions.baremetal = true;

        auto targetMachine = cgllvm::generateLLVM(*module, syntaxTree, program, codegenOptions);
        REQUIRE_FALSE(llvm::verifyModule(*module, &llvm::errs()));

        // Inspect IR using the ContainsIR matcher
        CHECK_THAT(*module, ContainsIR("store volatile i32 %arrayload, ptr inttoptr \\(i64 1024 to ptr\\)"));
        CHECK_THAT(*module, ContainsIR("load volatile i32, ptr inttoptr \\(i64 2048 to ptr\\)"));
    }

    SECTION("Generates Volatile Accesses in Baremetal Mode - Table")
    {
        auto [syntaxTree, program] = generateProgramPair(
            "PROGRAM MAIN ABSOLUTE ( INTEGER X / 1024; TABLE T / 4096 [10, 5] [ I INTEGER 0 ]; ) BEGIN X := 10; I[0] := 2; END");

        llvm::LLVMContext context;
        auto module = std::make_unique<llvm::Module>("coral_module", context);

        cgllvm::Options codegenOptions;
        codegenOptions.baremetal = true;

        auto targetMachine = cgllvm::generateLLVM(*module, syntaxTree, program, codegenOptions);
        REQUIRE_FALSE(llvm::verifyModule(*module, &llvm::errs()));

        // Inspect IR using the ContainsIR matcher
        CHECK_THAT(*module, ContainsIR("store volatile i32 10, ptr inttoptr \\(i64 1024 to ptr\\)"));
        CHECK_THAT(*module, ContainsIR("inttoptr \\(i64 4096 to ptr\\)"));
    }
}