/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 02/05/2026
 */

#include "lib/frontend/common/diagnostic_collection.hpp"
#include "lib/frontend/semantics/control_flow_pass.hpp"
#include "lib/frontend/semantics/semantic_context.hpp"
#include "lib/frontend/semantics/symbol_resolution_pass.hpp"
#include "lib/frontend/semantics/type_checking_pass.hpp"
#include "test/common/test_config.hpp"
#include "test/frontend/common/common.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66;
using namespace coral66::semantics;

namespace
{
struct ControlFlowResult
{
    std::string outStr;
    llvm::raw_string_ostream ss;
    bool errors = false;
    SemanticContext context;
    SymbolResolutionPass symPass;
    TypeCheckingPass typePass;
    ControlFlowPass cfPass;

    explicit ControlFlowResult(const ParsedProgram& parsed)
        : ss(outStr), context(parsed.cTokens, &ss, &errors), symPass(context), typePass(context), cfPass(context)
    {
        DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::clearDiagnostics();
        CHECK(parsed.program.hasBlock());
        if (parsed.program.hasBlock())
        {
            symPass.run(parsed.program);
            typePass.run(parsed.program);
            cfPass.run(parsed.program);
        }
    }

    static bool hasDiagnostic(std::string_view name)
    {
        return std::ranges::any_of(DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics(),
                                   [name](const auto& d) { return d.getName() == name; });
    }
};
} // namespace

TEST_CASE("Control Flow - Constant Bounds", "[Semantics][ControlFlow]")
{
    SECTION("Valid Array Access")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; INTEGER I ; I := A[5] END");
        ControlFlowResult res(parsed);
        CHECK(!res.hasDiagnostic("ARRAY_INDEX_OUT_OF_BOUNDS"));
    }

    SECTION("Array Access Out of Bounds (Lower)")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; INTEGER I ; I := A[0] END");
        ControlFlowResult res(parsed);
        CHECK(res.hasDiagnostic("ARRAY_INDEX_OUT_OF_BOUNDS"));
    }

    SECTION("Array Access Out of Bounds (Upper)")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; INTEGER I ; I := A[11] END");
        ControlFlowResult res(parsed);
        CHECK(res.hasDiagnostic("ARRAY_INDEX_OUT_OF_BOUNDS"));
    }

    SECTION("Array Access Evaluated Bound Valid")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; INTEGER I ; I := A[1 + 4] END");
        ControlFlowResult res(parsed);
        CHECK(!res.hasDiagnostic("ARRAY_INDEX_OUT_OF_BOUNDS"));
    }

    SECTION("Array Access Evaluated Bound Invalid")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; INTEGER I ; I := A[5 + 6] END");
        ControlFlowResult res(parsed);
        CHECK(res.hasDiagnostic("ARRAY_INDEX_OUT_OF_BOUNDS"));
    }

    SECTION("Array Declaration Evaluated Bounds Valid")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1+1 : 3*3] ; INTEGER I ; I := A[9] END");
        ControlFlowResult res(parsed);
        CHECK(!res.hasDiagnostic("ARRAY_INDEX_OUT_OF_BOUNDS"));
    }

    SECTION("Array Declaration Evaluated Bounds Invalid")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1+1 : 3*3] ; INTEGER I ; I := A[10] END");
        ControlFlowResult res(parsed);
        CHECK(res.hasDiagnostic("ARRAY_INDEX_OUT_OF_BOUNDS"));
    }

    SECTION("Array Declaration Bounds Cross-Over")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[10:1] ; END");
        ControlFlowResult res(parsed);
        CHECK(res.hasDiagnostic("INVALID_ARRAY_BOUNDS"));
    }

    SECTION("Multi-Dimensional Array Access Valid")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:5, -5:5] ; INTEGER I ; I := A[3, 0] END");
        ControlFlowResult res(parsed);
        CHECK(!res.hasDiagnostic("ARRAY_INDEX_OUT_OF_BOUNDS"));
    }

    SECTION("Multi-Dimensional Array Access Invalid")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:5, -5:5] ; INTEGER I ; I := A[3, -6] END");
        ControlFlowResult res(parsed);
        CHECK(res.hasDiagnostic("ARRAY_INDEX_OUT_OF_BOUNDS"));
    }

    SECTION("Valid Part Word Bounds")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := BITS [ 10 , 5 ] I END");
        ControlFlowResult res(parsed);
        CHECK(!res.hasDiagnostic("INVALID_BIT_RANGE"));
    }

    SECTION("Valid Part Word Bounds Evaluated")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := BITS [ 5 * 2 , 10 / 2 ] I END");
        ControlFlowResult res(parsed);
        CHECK(!res.hasDiagnostic("INVALID_BIT_RANGE"));
    }

    SECTION("Valid Part Word Bounds (32-bit word)")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := BITS [ 16 , 16 ] I END");
        ControlFlowResult res(parsed);
        CHECK(!res.hasDiagnostic("INVALID_BIT_RANGE"));
    }

    SECTION("Invalid Part Word Range (Too large)")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := BITS [ 24 , 10 ] I END");
        ControlFlowResult res(parsed);
        CHECK(res.hasDiagnostic("INVALID_BIT_RANGE"));
    }

    SECTION("Invalid Part Word Range (Zero Width)")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := BITS [ 0 , 5 ] I END");
        ControlFlowResult res(parsed);
        CHECK(res.hasDiagnostic("INVALID_BIT_RANGE"));
    }

    SECTION("Invalid Part Word Range (Negative Start)")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := BITS [ -1 , 5 ] I END");
        ControlFlowResult res(parsed);
        CHECK(res.hasDiagnostic("INVALID_BIT_RANGE"));
    }

    SECTION("Invalid Part Word Range (Negative Width)")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := BITS [ 5 , -1 ] I END");
        ControlFlowResult res(parsed);
        CHECK(res.hasDiagnostic("INVALID_BIT_RANGE"));
    }

    SECTION("Invalid Part Word Range on Assignment LHS")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; BITS [ 24 , 10 ] I := 1 END");
        ControlFlowResult res(parsed);
        CHECK(res.hasDiagnostic("INVALID_BIT_RANGE"));
    }

    SECTION("Valid Fixed Scaling")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := FIXED ( 16 , 8 ) ( I ) END");
        ControlFlowResult res(parsed);
        CHECK(!res.hasDiagnostic("INVALID_SCALE_FOR_FIXED"));
    }

    SECTION("Invalid Fixed Scaling (Fraction > Total)")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := FIXED ( 16 , 20 ) ( I ) END");
        ControlFlowResult res(parsed);
        CHECK(res.hasDiagnostic("INVALID_SCALE_FOR_FIXED"));
    }

    SECTION("Invalid Fixed Scaling (Negative Total)")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := FIXED ( -10 , 5 ) ( I ) END");
        ControlFlowResult res(parsed);
        CHECK(res.hasDiagnostic("INVALID_SCALE_FOR_FIXED"));
    }

    SECTION("Invalid Fixed Scaling (Zero Total)")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := FIXED ( 0 , 0 ) ( I ) END");
        ControlFlowResult res(parsed);
        CHECK(res.hasDiagnostic("INVALID_SCALE_FOR_FIXED"));
    }

    SECTION("Invalid Fixed Scaling (Negative Fraction)")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := FIXED ( 16 , -2 ) ( I ) END");
        ControlFlowResult res(parsed);
        CHECK(res.hasDiagnostic("INVALID_SCALE_FOR_FIXED"));
    }
}

TEST_CASE("Control Flow - Answer Statement", "[Semantics][ControlFlow]")
{
    SECTION("Typed Procedure with Answer")
    {
        const ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER PROCEDURE P ; BEGIN ANSWER 1 END ; P END");
        ControlFlowResult res(parsed);
        CHECK(!res.hasDiagnostic("MISSING_ANSWER_STATEMENT"));
    }

    SECTION("Typed Procedure with deeply nested Answer")
    {
        const ParsedProgram parsed(
            "PROGRAM TEST COMMON() BEGIN INTEGER PROCEDURE P ; BEGIN BEGIN ANSWER 1 END ; END ; P END");
        ControlFlowResult res(parsed);
        CHECK(!res.hasDiagnostic("MISSING_ANSWER_STATEMENT"));
    }

    SECTION("Typed Procedure missing Answer")
    {
        const ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER PROCEDURE P ; BEGIN END ; P END");
        ControlFlowResult res(parsed);
        CHECK(res.hasDiagnostic("MISSING_ANSWER_STATEMENT"));
    }

    SECTION("Untyped Procedure without Answer")
    {
        const ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN PROCEDURE P ; BEGIN END ; P END");
        ControlFlowResult res(parsed);
        CHECK(!res.hasDiagnostic("MISSING_ANSWER_STATEMENT"));
    }
}

TEST_CASE("Control Flow - Block Jumps", "[Semantics][ControlFlow]")
{
    SECTION("Valid Jump Out of Block")
    {
        const ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN MYLABEL: BEGIN GOTO MYLABEL ; END ; END");
        ControlFlowResult res(parsed);
        CHECK(!res.hasDiagnostic("ILLEGAL_BLOCK_JUMP"));
    }

    SECTION("Invalid Jump Into Block")
    {
        const ParsedProgram parsed(
            "PROGRAM TEST COMMON() BEGIN INTEGER I ; GOTO MYLABEL ; BEGIN INTEGER J ; MYLABEL: I := 1 ; END ; END");
        ControlFlowResult res(parsed);
        CHECK(res.hasDiagnostic("UNDECLARED_IDENTIFIER_N"));
    }
}

TEST_CASE("Control Flow - Switch & Presets", "[Semantics][ControlFlow]")
{
    SECTION("Switch bounds constant evaluation valid")
    {
        const ParsedProgram parsed(
            "PROGRAM TEST COMMON() BEGIN MYLABEL: BEGIN SWITCH S := MYLABEL ; GOTO S[1] ; END ; END");
        ControlFlowResult res(parsed);
        CHECK(!res.hasDiagnostic("SWITCH_INDEX_OUT_OF_BOUNDS"));
    }

    SECTION("Switch bounds constant evaluation too small")
    {
        const ParsedProgram parsed(
            "PROGRAM TEST COMMON() BEGIN MYLABEL: BEGIN SWITCH S := MYLABEL ; GOTO S[0] ; END ; END");
        ControlFlowResult res(parsed);
        CHECK(res.hasDiagnostic("SWITCH_INDEX_OUT_OF_BOUNDS"));
    }

    SECTION("Switch bounds constant evaluation too large")
    {
        const ParsedProgram parsed(
            "PROGRAM TEST COMMON() BEGIN MYLABEL: BEGIN SWITCH S := MYLABEL ; GOTO S[2] ; END ; END");
        ControlFlowResult res(parsed);
        CHECK(res.hasDiagnostic("SWITCH_INDEX_OUT_OF_BOUNDS"));
    }
}
