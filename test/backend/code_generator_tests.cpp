/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Verifier.h>

#include "common.hpp"
#include "lib/backend//code_generation.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace Catch::Matchers;

static coral66::CGLLVM::Options emitAllDecls = []
{
    coral66::CGLLVM::Options result;
    result.emitAllDecls = true;
    return result;
}();

TEST_CASE("Coral 66 Fixed Point Arithmetic", "[Coral]")
{
    llvm::LLVMContext context;
    llvm::Module module("", context);

    SECTION("Fixed Point Addition")
    {
        auto program = generateProgram("PROGRAM FixedAdd\n"
                                       "BEGIN\n"
                                       "  FIXED(16,8) a;\n"
                                       "  FIXED(16,8) b;\n"
                                       "  FIXED(16,8) c;\n"
                                       "  a := 1.5;\n"
                                       "  b := 2.5;\n"
                                       "  c := a + b;\n"
                                       "END");
        coral66::CGLLVM::generateLLVM(module, program, emitAllDecls);
        CAPTURE(module);
        REQUIRE_FALSE(llvm::verifyModule(module, &llvm::errs()));

        auto* function = module.getFunction("main");
        REQUIRE(function);

        bool foundAdd = false;
        for (auto& bb : *function)
        {
            for (auto& inst : bb)
            {
                if (inst.getOpcode() == llvm::Instruction::Add)
                {
                    foundAdd = true;
                    CHECK(inst.getType()->isIntegerTy(16));
                }
            }
        }
        CHECK(foundAdd);
    }

    SECTION("Fixed Point Multiplication")
    {
        auto program = generateProgram("PROGRAM FixedMul\n"
                                       "BEGIN\n"
                                       "  FIXED(16,8) a;\n"
                                       "  FIXED(16,8) b;\n"
                                       "  FIXED(16,8) c;\n"
                                       "  c := a * b;\n"
                                       "END");
        coral66::CGLLVM::generateLLVM(module, program, emitAllDecls);
        REQUIRE_FALSE(llvm::verifyModule(module, &llvm::errs()));

        auto* function = module.getFunction("main");
        REQUIRE(function);

        bool foundMul = false;
        bool foundShift = false;
        for (auto& bb : *function)
        {
            for (auto& inst : bb)
            {
                if (inst.getOpcode() == llvm::Instruction::Mul)
                {
                    foundMul = true;
                }
                if (inst.getOpcode() == llvm::Instruction::AShr)
                {
                    foundShift = true;
                    auto* constant = llvm::dyn_cast<llvm::ConstantInt>(inst.getOperand(1));
                    REQUIRE(constant);
                    CHECK(constant->getZExtValue() == 8);
                }
            }
        }
        CHECK(foundMul);
        CHECK(foundShift);
    }
}

TEST_CASE("Coral 66 Table", "[Coral]")
{
    llvm::LLVMContext context;
    llvm::Module module("", context);

    SECTION("Table Declaration and Access")
    {
        auto program = generateProgram("PROGRAM TableTest\n"
                                       "BEGIN\n"
                                       "  TABLE T[10]\n"
                                       "  [\n"
                                       "    INTEGER x;\n"
                                       "    FIXED(16,8) y\n"
                                       "  ];\n"
                                       "  x[0] := 10;\n"
                                       "END");
        coral66::CGLLVM::generateLLVM(module, program, emitAllDecls);
        REQUIRE_FALSE(llvm::verifyModule(module, &llvm::errs()));
    }
}

TEST_CASE("Coral 66 Loops", "[Coral]")
{
    llvm::LLVMContext context;
    llvm::Module module("", context);

    SECTION("For Loop with Step")
    {
        auto program = generateProgram("PROGRAM LoopTest\n"
                                       "BEGIN\n"
                                       "  INTEGER i;\n"
                                       "  FOR i := 0 STEP 2 UNTIL 10 DO\n"
                                       "    i := 2;\n"
                                       "END");
        coral66::CGLLVM::generateLLVM(module, program, emitAllDecls);
        REQUIRE_FALSE(llvm::verifyModule(module, &llvm::errs()));
    }
}

TEST_CASE("Coral 66 Answer Statement", "[Coral]")
{
    llvm::LLVMContext context;
    llvm::Module module("", context);

    SECTION("Answer")
    {
        auto program = generateProgram("PROGRAM AnsTest\n"
                                       "BEGIN\n"
                                       "  INTEGER PROCEDURE test; BEGIN ANSWER 42; END;\n"
                                       "END");
        coral66::CGLLVM::generateLLVM(module, program, emitAllDecls);
        REQUIRE_FALSE(llvm::verifyModule(module, &llvm::errs()));

        auto* function = module.getFunction("test");
        REQUIRE(function);
        bool foundRet = false;
        for (auto& bb : *function)
        {
            for (auto& inst : bb)
            {
                if (llvm::dyn_cast<llvm::ReturnInst>(&inst))
                {
                    foundRet = true;
                }
            }
        }
        CHECK(foundRet);
    }
}

TEST_CASE("Coral 66 Code Statement", "[Coral]")
{
    llvm::LLVMContext context;
    llvm::Module module("", context);

    SECTION("Inline Code")
    {
        auto program = generateProgram("PROGRAM CodeTest\n"
                                       "BEGIN\n"
                                       "  CODE BEGIN \"nop\" END;\n"
                                       "END");
        coral66::CGLLVM::generateLLVM(module, program, emitAllDecls);
        REQUIRE_FALSE(llvm::verifyModule(module, &llvm::errs()));

        auto* function = module.getFunction("main");
        REQUIRE(function);
        // TODO fix bool foundAsm = false;
        for (auto& bb : *function)
        {
            for (auto& inst : bb)
            {
                if (auto* call = llvm::dyn_cast<llvm::CallInst>(&inst))
                {
                    if (call->isInlineAsm())
                    {
                        // foundAsm = true;
                    }
                }
            }
        }
        // TODO Fix CHECK(foundAsm);
    }
}

TEST_CASE("Coral 66 Control Flow", "[Coral]")
{
    llvm::LLVMContext context;
    llvm::Module module("", context);

    SECTION("If Statement")
    {
        auto program = generateProgram("PROGRAM IfTest\n"
                                       "BEGIN\n"
                                       "  INTEGER a;\n"
                                       "  a := 1;\n"
                                       "  INTEGER b; b := 2; IF a > b THEN a := 2 ELSE a := 3;\n"
                                       "END");
        coral66::CGLLVM::generateLLVM(module, program, emitAllDecls);
        REQUIRE_FALSE(llvm::verifyModule(module, &llvm::errs()));

        auto* function = module.getFunction("main");
        REQUIRE(function);
        // Should have branches
        bool foundBr = false;
        for (auto& bb : *function)
        {
            if (llvm::dyn_cast<llvm::BranchInst>(bb.getTerminator()))
            {
                foundBr = true;
            }
        }
        CHECK(foundBr);
    }
}
