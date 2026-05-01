#include "lib/frontend/common/diagnostic_collection.hpp"
#include "lib/frontend/parser/parser.hpp"
#include "lib/frontend/semantics/control_flow_pass.hpp"
#include "lib/frontend/semantics/semantic_context.hpp"
#include "lib/frontend/semantics/symbol_resolution_pass.hpp"
#include "lib/frontend/semantics/type_checking_pass.hpp"
#include "test/common/test_config.hpp"
#include "test/frontend/parser/common.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66;
using namespace coral66::semantics;

struct ControlFlowResult
{
    std::string outStr;
    llvm::raw_string_ostream ss;
    bool errors = false;
    SemanticContext context;
    SymbolResolutionPass symPass;
    TypeCheckingPass typePass;
    ControlFlowPass cfPass;

    ControlFlowResult(const ParsedProgram& parsed)
        : ss(outStr), context(parsed.cTokens, &ss, &errors), symPass(context), typePass(context), cfPass(context)
    {
        DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::clearDiagnostics();
        REQUIRE(parsed.program.hasBlock());
        if (parsed.program.hasBlock())
        {
            symPass.run(parsed.program);
            typePass.run(parsed.program);
            cfPass.run(parsed.program);
        }
    }

    bool hasDiagnostic(std::string_view name) const
    {
        return std::ranges::any_of(DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics(),
                                   [name](const auto& d) { return d.getName() == name; });
    }
};

TEST_CASE("Control Flow - Constant Bounds", "[Semantics][ControlFlow]")
{
    SECTION("Valid Array Access")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; INTEGER I ; I := A[5] END");
        ControlFlowResult res(parsed);
        REQUIRE(!res.hasDiagnostic("ARRAY_INDEX_OUT_OF_BOUNDS"));
    }

    SECTION("Array Access Out of Bounds (Lower)")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; INTEGER I ; I := A[0] END");
        ControlFlowResult res(parsed);
        REQUIRE(res.hasDiagnostic("ARRAY_INDEX_OUT_OF_BOUNDS"));
    }

    SECTION("Array Access Out of Bounds (Upper)")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; INTEGER I ; I := A[11] END");
        ControlFlowResult res(parsed);
        REQUIRE(res.hasDiagnostic("ARRAY_INDEX_OUT_OF_BOUNDS"));
    }

    SECTION("Array Access Evaluated Bound Valid")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; INTEGER I ; I := A[1 + 4] END");
        ControlFlowResult res(parsed);
        REQUIRE(!res.hasDiagnostic("ARRAY_INDEX_OUT_OF_BOUNDS"));
    }

    SECTION("Array Access Evaluated Bound Invalid")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; INTEGER I ; I := A[5 + 6] END");
        ControlFlowResult res(parsed);
        REQUIRE(res.hasDiagnostic("ARRAY_INDEX_OUT_OF_BOUNDS"));
    }

    SECTION("Array Declaration Evaluated Bounds Valid")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1+1 : 3*3] ; INTEGER I ; I := A[9] END");
        ControlFlowResult res(parsed);
        REQUIRE(!res.hasDiagnostic("ARRAY_INDEX_OUT_OF_BOUNDS"));
    }

    SECTION("Array Declaration Evaluated Bounds Invalid")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1+1 : 3*3] ; INTEGER I ; I := A[10] END");
        ControlFlowResult res(parsed);
        REQUIRE(res.hasDiagnostic("ARRAY_INDEX_OUT_OF_BOUNDS"));
    }

    SECTION("Multi-Dimensional Array Access Valid")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:5, -5:5] ; INTEGER I ; I := A[3, 0] END");
        ControlFlowResult res(parsed);
        REQUIRE(!res.hasDiagnostic("ARRAY_INDEX_OUT_OF_BOUNDS"));
    }

    SECTION("Multi-Dimensional Array Access Invalid")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:5, -5:5] ; INTEGER I ; I := A[3, -6] END");
        ControlFlowResult res(parsed);
        REQUIRE(res.hasDiagnostic("ARRAY_INDEX_OUT_OF_BOUNDS"));
    }

    SECTION("Valid Part Word Bounds")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := BITS [ 10 , 5 ] I END");
        ControlFlowResult res(parsed);
        REQUIRE(!res.hasDiagnostic("INVALID_BIT_RANGE"));
    }

    SECTION("Valid Part Word Bounds Evaluated")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := BITS [ 5 * 2 , 10 / 2 ] I END");
        ControlFlowResult res(parsed);
        REQUIRE(!res.hasDiagnostic("INVALID_BIT_RANGE"));
    }

    SECTION("Invalid Part Word Range (Too large)")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := BITS [ 20 , 10 ] I END");
        ControlFlowResult res(parsed);
        REQUIRE(res.hasDiagnostic("INVALID_BIT_RANGE"));
    }

    SECTION("Invalid Part Word Range (Negative Start)")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := BITS [ -1 , 5 ] I END");
        ControlFlowResult res(parsed);
        REQUIRE(res.hasDiagnostic("INVALID_BIT_RANGE"));
    }

    SECTION("Invalid Part Word Range (Negative Width)")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := BITS [ 5 , -1 ] I END");
        ControlFlowResult res(parsed);
        REQUIRE(res.hasDiagnostic("INVALID_BIT_RANGE"));
    }

    SECTION("Valid Fixed Scaling")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := FIXED ( 16 , 8 ) ( I ) END");
        ControlFlowResult res(parsed);
        REQUIRE(!res.hasDiagnostic("INVALID_SCALE_FOR_FIXED"));
    }

    SECTION("Invalid Fixed Scaling (Fraction > Total)")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := FIXED ( 16 , 20 ) ( I ) END");
        ControlFlowResult res(parsed);
        REQUIRE(res.hasDiagnostic("INVALID_SCALE_FOR_FIXED"));
    }

    SECTION("Invalid Fixed Scaling (Negative Total)")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := FIXED ( -10 , 5 ) ( I ) END");
        ControlFlowResult res(parsed);
        REQUIRE(res.hasDiagnostic("INVALID_SCALE_FOR_FIXED"));
    }

    SECTION("Invalid Fixed Scaling (Zero Total)")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := FIXED ( 0 , 0 ) ( I ) END");
        ControlFlowResult res(parsed);
        REQUIRE(res.hasDiagnostic("INVALID_SCALE_FOR_FIXED"));
    }

    SECTION("Invalid Fixed Scaling (Negative Fraction)")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := FIXED ( 16 , -2 ) ( I ) END");
        ControlFlowResult res(parsed);
        REQUIRE(res.hasDiagnostic("INVALID_SCALE_FOR_FIXED"));
    }
}

TEST_CASE("Control Flow - Answer Statement", "[Semantics][ControlFlow]")
{
    SECTION("Typed Procedure with Answer")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER PROCEDURE P ; BEGIN ANSWER 1 END ; P END");
        ControlFlowResult res(parsed);
        REQUIRE(!res.hasDiagnostic("MISSING_ANSWER_STATEMENT"));
    }

    SECTION("Typed Procedure with deeply nested Answer")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER PROCEDURE P ; BEGIN BEGIN ANSWER 1 END ; END ; P END");
        ControlFlowResult res(parsed);
        REQUIRE(!res.hasDiagnostic("MISSING_ANSWER_STATEMENT"));
    }

    SECTION("Typed Procedure missing Answer")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER PROCEDURE P ; BEGIN END ; P END");
        ControlFlowResult res(parsed);
        REQUIRE(res.hasDiagnostic("MISSING_ANSWER_STATEMENT"));
    }

    SECTION("Untyped Procedure without Answer")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN PROCEDURE P ; BEGIN END ; P END");
        ControlFlowResult res(parsed);
        REQUIRE(!res.hasDiagnostic("MISSING_ANSWER_STATEMENT"));
    }
}
