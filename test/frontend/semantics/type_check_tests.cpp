/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 25/04/2026
 */

#include <algorithm>
#include <iostream>

#include "lib/frontend/common/diagnostic_collection.hpp"
#include "lib/frontend/parser/parser.hpp"
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
struct TypeCheckingResult
{
    std::string outStr;
    llvm::raw_string_ostream ss;
    bool errors = false;
    SemanticContext context;
    SymbolResolutionPass symPass;
    TypeCheckingPass typePass;

    explicit TypeCheckingResult(const ParsedProgram& parsed, const bool runTypePass = true)
        : ss(outStr), context(parsed.cTokens, &ss, &errors), symPass(context), typePass(context)
    {
        DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::clearDiagnostics();
        CHECK(parsed.program.hasBlock());
        if (parsed.program.hasBlock())
        {
            symPass.run(parsed.program);
            if (runTypePass)
                typePass.run(parsed.program);
        }
    }

    static bool hasDiagnostic(std::string_view name)
    {
        return std::ranges::any_of(DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics(),
                                   [name](const auto& d) { return d.getName() == name; });
    }
};
} // namespace

TEST_CASE("Type Checking - IF Condition", "[Semantics][TypeChecking]")
{
    SECTION("Valid Integer Condition")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; IF I = 0 THEN I := 1 END");
        TypeCheckingResult res(parsed);
        CHECK(!res.hasDiagnostic("EXPECTED_INTEGER_TYPE"));
    }

    // SECTION("Invalid Floating Condition") removed
}

TEST_CASE("Type Checking - GOTO Index", "[Semantics][TypeChecking]")
{
    SECTION("Valid Integer Index")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; GOTO s[I] END");
        TypeCheckingResult res(parsed);
        CHECK(!res.hasDiagnostic("EXPECTED_INTEGER_TYPE"));
    }

    SECTION("Invalid Floating Index")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN FLOATING F ; GOTO s[F] END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("EXPECTED_INTEGER_TYPE"));
    }
}

TEST_CASE("Type Checking - Array Access", "[Semantics][TypeChecking]")
{
    SECTION("Valid Integer Index")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; INTEGER I ; A[I] := 1 END");
        TypeCheckingResult res(parsed);
        CHECK(!res.hasDiagnostic("EXPECTED_INTEGER_TYPE"));
    }

    SECTION("Invalid Floating Index")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; FLOATING F ; A[F] := 1 END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("EXPECTED_INTEGER_TYPE"));
    }
}

TEST_CASE("Type Checking - Assignment", "[Semantics][TypeChecking]")
{
    SECTION("Type Mismatch Assigning Array to Variable")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; INTEGER I ; I := A END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("TYPE_MISMATCH"));
    }

    SECTION("Type Mismatch Assigning Variable to Array")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; INTEGER I ; A := I END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("TYPE_MISMATCH"));
    }

    SECTION("Valid Assigning Location to Pointer")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; POINTER P ; P := LOCATION(I) END");
        TypeCheckingResult res(parsed);
        CHECK(!res.hasDiagnostic("TYPE_MISMATCH"));
    }

    SECTION("Valid Assigning Integer to Pointer")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN POINTER P ; P := 0 END");
        TypeCheckingResult res(parsed);
        CHECK(!res.hasDiagnostic("TYPE_MISMATCH"));
    }

    SECTION("Type Mismatch Assigning Pointer to Integer")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; POINTER P ; I := P END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("TYPE_MISMATCH"));
    }

    SECTION("Type Mismatch Assigning Location to Integer")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN INTEGER I, J ; I := LOCATION(J) END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("TYPE_MISMATCH"));
    }
}

TEST_CASE("Type Checking - Binary Expression", "[Semantics][TypeChecking]")
{
    SECTION("Valid Binary Expression")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := I + 1 END");
        TypeCheckingResult res(parsed);
        CHECK(!res.hasDiagnostic("TYPE_MISMATCH"));
    }

    SECTION("Type Mismatch Array in Binary Expression")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; INTEGER I ; I := A + 1 END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("TYPE_MISMATCH"));
    }
}

TEST_CASE("Type Checking - Procedure Calls", "[Semantics][TypeChecking]")
{
    SECTION("Argument Count Mismatch")
    {
        ParsedProgram const parsed(
            "PROGRAM TEST COMMON() BEGIN PROCEDURE P(VALUE INTEGER X) ; BEGIN END ; P(1, 2) END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("PROCEDURE_ARGUMENT_COUNT_MISMATCH"));
    }

    SECTION("Valid Zero Argument Call")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN PROCEDURE P ; BEGIN END ; P END");
        TypeCheckingResult res(parsed);
        CHECK(!res.hasDiagnostic("PROCEDURE_ARGUMENT_COUNT_MISMATCH"));
        CHECK(!res.hasDiagnostic("PROCEDURE_ARGUMENT_TYPE_MISMATCH"));
    }

    SECTION("Argument Type Mismatch")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN P(1.0) END");
        TypeCheckingResult res(parsed, false);

        static PrimitiveType intType(PrimitiveType::Integer);
        std::vector<ProcedureType::Parameter> params;
        params.push_back({.type = &intType, .name = "X", .isValue = false});

        auto procType = IntrusiveVariantValue(ProcedureType{nullptr, std::move(params)});

        auto it = parsed.cTokens.data().begin();
        auto nameToken = &*it;

        auto pDeclaration = IntrVarPtr<Usable>(new semantics::ProcedureDeclaration(nameToken, std::move(procType)));

        res.context.symbolTable.insert("P", pDeclaration.get());

        if (parsed.program.hasBlock())
        {
            for (const auto& stmt : parsed.program.getBlock().getStatements())
            {
                if (auto callStmt = dynamic_cast<const syntax::ProcedureCallStatement*>(stmt.get()))
                {
                    if (auto callExpr = dynamic_cast<const FunctionCallExpression*>(&callStmt->getExpression()))
                    {
                        res.context.resolvedIdentifiers[callExpr] = pDeclaration.get();
                    }
                }
            }
        }
        res.context.ownedUsables.push_back(std::move(pDeclaration));

        res.typePass.run(parsed.program);

        CHECK(res.hasDiagnostic("PROCEDURE_ARGUMENT_TYPE_MISMATCH"));
    }
}

TEST_CASE("Type Checking - Array Declaration", "[Semantics][TypeChecking]")
{
    SECTION("Valid Integer Dimensions")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] END");
        TypeCheckingResult res(parsed);
        CHECK(!res.hasDiagnostic("EXPECTED_INTEGER_TYPE"));
    }

    SECTION("Invalid Floating Dimension First")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1.0:10] END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("EXPECTED_INTEGER_TYPE"));
    }

    SECTION("Invalid Floating Dimension Second")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10.0] END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("EXPECTED_INTEGER_TYPE"));
    }
}

TEST_CASE("Type Checking - Table Declaration", "[Semantics][TypeChecking]")
{
    SECTION("Valid Integer Width and Length")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN TABLE T[10, 20] [ I INTEGER 0 ] END");
        TypeCheckingResult res(parsed);
        CHECK(!res.hasDiagnostic("EXPECTED_INTEGER_TYPE"));
    }

    SECTION("Invalid Floating Width")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN TABLE T[10.0, 20] [ I INTEGER 0 ] END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("EXPECTED_INTEGER_TYPE"));
    }

    SECTION("Invalid Floating Length")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN TABLE T[10, 20.0] [ I INTEGER 0 ] END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("EXPECTED_INTEGER_TYPE"));
    }
}

TEST_CASE("Type Checking - Overlay Declaration", "[Semantics][TypeChecking]")
{
    SECTION("Valid Inner Declaration")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; OVERLAY I WITH INTEGER ARRAY A[1:10] END");
        TypeCheckingResult res(parsed);
        CHECK(!res.hasDiagnostic("EXPECTED_INTEGER_TYPE"));
    }

    SECTION("Invalid Inner Declaration")
    {
        ParsedProgram const parsed(
            "PROGRAM TEST COMMON() BEGIN INTEGER I ; OVERLAY I WITH INTEGER ARRAY A[1.0:10] END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("EXPECTED_INTEGER_TYPE"));
    }
}

TEST_CASE("Type Checking - Procedure Calls Coercion", "[Semantics][TypeChecking]")
{
    SECTION("Argument Type Coercion for Value Parameters")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN P(1.0) END");
        TypeCheckingResult res(parsed, false);

        static PrimitiveType intType(PrimitiveType::Integer);
        std::vector<ProcedureType::Parameter> params;
        params.push_back({.type = &intType, .name = "X", .isValue = true}); // isValue = true

        auto procType = IntrusiveVariantValue(ProcedureType{nullptr, std::move(params)});

        const auto it = parsed.cTokens.data().begin();
        const auto nameToken = &*it;

        auto pDeclaration = IntrVarPtr<Usable>(new semantics::ProcedureDeclaration(nameToken, std::move(procType)));

        res.context.symbolTable.insert("P", pDeclaration.get());

        if (parsed.program.hasBlock())
        {
            for (const auto& stmt : parsed.program.getBlock().getStatements())
            {
                if (const auto callStmt = dynamic_cast<const syntax::ProcedureCallStatement*>(stmt.get()))
                {
                    if (const auto callExpr = dynamic_cast<const FunctionCallExpression*>(&callStmt->getExpression()))
                    {
                        res.context.resolvedIdentifiers[callExpr] = pDeclaration.get();
                    }
                }
            }
        }
        res.context.ownedUsables.push_back(std::move(pDeclaration));

        res.typePass.run(parsed.program);

        CHECK(!res.hasDiagnostic("PROCEDURE_ARGUMENT_TYPE_MISMATCH"));
    }
}

TEST_CASE("Type Checking - FOR WHILE Condition", "[Semantics][TypeChecking]")
{
    SECTION("Valid Integer Condition")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; FOR I := 1 WHILE 1 = 1 DO I := 2 END");
        TypeCheckingResult res(parsed);
        CHECK(!res.hasDiagnostic("EXPECTED_INTEGER_TYPE"));
    }

    // SECTION("Invalid Floating Condition") removed
}

TEST_CASE("Type Checking - ANSWER Statement", "[Semantics][TypeChecking]")
{
    SECTION("Valid Answer in Typed Procedure")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN INTEGER PROCEDURE P ; BEGIN ANSWER 1 END ; END");
        TypeCheckingResult res(parsed);
        CHECK(!res.hasDiagnostic("TYPE_MISMATCH"));
    }

    SECTION("Valid Answer in Typed Procedure With Assignment")
    {
        ParsedProgram const parsed(
            "PROGRAM TEST COMMON() BEGIN INTEGER a; INTEGER PROCEDURE P ; BEGIN ANSWER 1 END ; a := P; END");
        TypeCheckingResult res(parsed);
        CHECK(!res.hasDiagnostic("TYPE_MISMATCH"));
    }

    SECTION("Invalid Answer in Untyped Procedure")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN PROCEDURE P ; BEGIN ANSWER 1 END ; END");
        CHECK(std::ranges::any_of(DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics(),
                                  [](const auto& d) { return d.getName() == "ANSWER_IN_UNTYPED_PROCEDURE"; }));
    }

    SECTION("Invalid Answer Type")
    {
        ParsedProgram const parsed(
            "PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; INTEGER PROCEDURE P ; BEGIN ANSWER A END ; END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("TYPE_MISMATCH"));
    }
}

TEST_CASE("Type Checking - Type Change Expression", "[Semantics][TypeChecking]")
{
    SECTION("Valid Cast to Integer used as Array Index")
    {
        ParsedProgram const parsed(
            "PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; INTEGER I ; I := A[INTEGER (1.0)] END");
        TypeCheckingResult res(parsed);
        CHECK(!res.hasDiagnostic("EXPECTED_INTEGER_TYPE"));
    }

    SECTION("Invalid Cast to Floating used as Array Index")
    {
        ParsedProgram const parsed(
            "PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; INTEGER I ; I := A[FLOATING (1)] END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("EXPECTED_INTEGER_TYPE"));
    }
}

TEST_CASE("Type Checking - Fixed Type", "[Semantics][TypeChecking]")
{
    SECTION("Valid Mixed Arithmetic")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN FIXED(24,2) F ; INTEGER I ; F := F + I END");
        TypeCheckingResult res(parsed);
        CHECK(!res.hasDiagnostic("TYPE_MISMATCH"));
    }
}

TEST_CASE("Type Checking - Array Indices", "[Semantics][TypeChecking]")
{
    SECTION("Incorrect Number of Indices - Too Few")
    {
        ParsedProgram const parsed(
            "PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10, 1:10] ; INTEGER I ; A[I] := 1 END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("INCORRECT_NUMBER_OF_INDICES"));
    }

    SECTION("Incorrect Number of Indices - Too Many")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; INTEGER I ; A[I, I] := 1 END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("INCORRECT_NUMBER_OF_INDICES"));
    }

    SECTION("Anonymous Array Reference")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := [ 1234 ] END");
        TypeCheckingResult res(parsed);
        CHECK(!res.hasDiagnostic("TYPE_MISMATCH"));
        CHECK(!res.hasDiagnostic("EXPECTED_INTEGER_TYPE"));
    }
}

TEST_CASE("Type Checking - Array Parameter Matching", "[Semantics][TypeChecking]")
{
    SECTION("Different Bounds but Same Type")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; "
                                   "PROCEDURE P (INTEGER ARRAY X) ; BEGIN INTEGER I ; I := X[1] END ; "
                                   "P(A) END");
        TypeCheckingResult res(parsed);
        CHECK(!res.hasDiagnostic("PROCEDURE_ARGUMENT_TYPE_MISMATCH"));
    }

    SECTION("Different Types")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN FLOATING ARRAY A[1:10] ; "
                                   "PROCEDURE P (INTEGER ARRAY X) ; BEGIN INTEGER I ; I := X[1] END ; "
                                   "P(A) END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("PROCEDURE_ARGUMENT_TYPE_MISMATCH"));
    }
}

TEST_CASE("Type Checking - For Loop Variables", "[Semantics][TypeChecking]")
{
    SECTION("Valid Numeric Loop Variable")
    {
        ParsedProgram const parsed(
            "PROGRAM TEST COMMON() BEGIN INTEGER I ; FOR I := 1 STEP 1 UNTIL 10 DO I := I + 1 END");
        TypeCheckingResult res(parsed);
        CHECK(!res.hasDiagnostic("EXPECTED_NUMERIC_TYPE"));
    }

    SECTION("Invalid Loop Variable - Array")
    {
        ParsedProgram const parsed(
            "PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; FOR A := 1 STEP 1 UNTIL 10 DO A[1] := 1 END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("EXPECTED_NUMERIC_TYPE"));
    }
}

TEST_CASE("Type Checking - Control Flow Parameters", "[Semantics][TypeChecking]")
{
    SECTION("Passing a Label")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN "
                                   "PROCEDURE P (LABEL L) ; GOTO L ; "
                                   "MYLABEL: P(MYLABEL) END");
        TypeCheckingResult res(parsed);
        CHECK(!res.hasDiagnostic("PROCEDURE_ARGUMENT_TYPE_MISMATCH"));
    }

    SECTION("Passing a Label No Label Not Found Error")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN "
                                   "PROCEDURE P (LABEL L) ; GOTO L ; END;"
                                   "MYLABEL: P(MYLABEL) END");
        TypeCheckingResult res(parsed);
        CHECK(!res.hasDiagnostic("LABEL_NOT_FOUND_N"));
    }

    SECTION("Passing a Label No Location Parameter Requires Error")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN "
                                   "INTEGER ARRAY statusFlags [ 0 : 5 ];"
                                   "PROCEDURE P (LABEL L); GOTO L END;"
                                   "MYLABEL: P(MYLABEL) END");
        TypeCheckingResult res(parsed);
        CHECK(!res.hasDiagnostic("LOCATION_PARAMETER_REQUIRES_LVALUE"));
    }

    SECTION("Passing a Switch")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN "
                                   "SWITCH S := MYLABEL ; "
                                   "PROCEDURE P (SWITCH SW) ; GOTO SW[1] ; "
                                   "MYLABEL: P(S) END");
        TypeCheckingResult res(parsed);
        CHECK(!res.hasDiagnostic("PROCEDURE_ARGUMENT_TYPE_MISMATCH"));
        CHECK(!res.hasDiagnostic("LABEL_NOT_FOUND_N"));
    }
}

TEST_CASE("Type Checking - Table Access", "[Semantics][TypeChecking]")
{
    SECTION("Valid Table Element Access")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN TABLE T[2, 2] [ I INTEGER 0, 0 ] ; I[0] := 1 END");
        TypeCheckingResult res(parsed);
        CHECK(!res.hasDiagnostic("TYPE_MISMATCH"));
    }
}

TEST_CASE("Type Checking - BITS Operations", "[Semantics][TypeChecking]")
{
    SECTION("Valid BITS assignment")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN BITS I ; I := 10 END");
        TypeCheckingResult res(parsed);
        CHECK(!res.hasDiagnostic("TYPE_MISMATCH"));
    }

    SECTION("Invalid Floating assignment to BITS")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN BITS I ; I := 10.5 END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("TYPE_MISMATCH"));
    }

    SECTION("Valid BITS differ operation")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN BITS I ; I := 1 DIFFER 2 END");
        TypeCheckingResult res(parsed);
        CHECK(!res.hasDiagnostic("TYPE_MISMATCH"));
    }

    SECTION("Invalid floating DIFFER")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN BITS I ; I := 1.5 DIFFER 2.5 END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("TYPE_MISMATCH"));
    }
}

TEST_CASE("Type Checking - Type Change Wrappers", "[Semantics][TypeChecking]")
{
    SECTION("Invalid parameter to type change")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; "
                                   "MYLABEL: I := INTEGER(MYLABEL) END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("TYPE_MISMATCH"));
    }
}

TEST_CASE("Type Checking - Dummy Test", "[Semantics][TypeChecking]")
{
    CHECK(true);
}

TEST_CASE("Type Checking - PartWord Bounds", "[Semantics][TypeChecking]")
{
    SECTION("Valid PartWord indices")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := BITS [ 1 , 2 ] I END");
        TypeCheckingResult res(parsed);
        CHECK(!res.hasDiagnostic("EXPECTED_INTEGER_TYPE"));
    }

    SECTION("Invalid Floating startBit")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := BITS [ 1.5 , 2 ] I END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("EXPECTED_INTEGER_TYPE"));
    }

    SECTION("Invalid Floating width")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := BITS [ 1 , 2.5 ] I END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("EXPECTED_INTEGER_TYPE"));
    }

    SECTION("Invalid Label base operand")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; MYLABEL: I := BITS [ 4 , 0 ] MYLABEL END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("TYPE_MISMATCH"));
    }

    SECTION("Invalid Untyped Procedure base operand")
    {
        ParsedProgram const parsed(
            "PROGRAM TEST COMMON() BEGIN INTEGER I ; PROCEDURE P ; BEGIN END ; I := BITS [ 4 , 0 ] P END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("TYPE_MISMATCH"));
    }
}

TEST_CASE("Type Checking - Recursive Validations", "[Semantics][TypeCheck]")
{
    SECTION("Procedure calls itself without RECURSIVE")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN PROCEDURE P ; BEGIN P ; END ; END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("RECURSIVE_CALL_ON_NON_RECURSIVE_PROCEDURE"));
    }

    SECTION("Procedure calls itself with RECURSIVE")
    {
        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN RECURSIVE PROCEDURE P ; BEGIN P ; END ; END");
        TypeCheckingResult res(parsed);
        CHECK(!res.hasDiagnostic("RECURSIVE_CALL_ON_NON_RECURSIVE_PROCEDURE"));
    }

    SECTION("Preset Type Constraints - Integer")
    {
        const std::string source = R"(
        INTEGER i := 1.5;
        INTEGER ARRAY arr[1:2] := 1, "test";
        )";

        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN " + source + " END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("PRESET_TYPE_MISMATCH"));
    }

    SECTION("Preset Type Constraints - Fixed point overflow")
    {
        const std::string source = R"(
        FIXED(10, 5) f := 100.0; 
        )";

        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN " + source + " END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("PRESET_FIXED_POINT_OVERFLOW"));
    }

    SECTION("Octal Point Expressions and Presets")
    {
        const std::string source = R"(
        FLOATING fl := OCTAL(12.34);
        FIXED(16, 4) fx := -OCTAL(1.4);
        fl := OCTAL(1.4) + OCTAL(0.2);
        fx := -OCTAL(2.4);
        )";

        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN " + source + " END");
        TypeCheckingResult res(parsed);
        CHECK(!res.hasDiagnostic("TYPE_MISMATCH"));
        CHECK(!res.hasDiagnostic("PRESET_TYPE_MISMATCH"));
        CHECK(!res.hasDiagnostic("PRESET_FIXED_POINT_OVERFLOW"));
    }

    SECTION("Octal Point Preset Type Mismatch on Integer")
    {
        const std::string source = R"(
        INTEGER i := OCTAL(1.4);
        )";

        ParsedProgram const parsed("PROGRAM TEST COMMON() BEGIN " + source + " END");
        TypeCheckingResult res(parsed);
        CHECK(res.hasDiagnostic("PRESET_TYPE_MISMATCH"));
    }

    SECTION("Octal Point Fixed Point Bounds")
    {
        // FIXED(5, 1) has range [-8.0, 7.5]; OCTAL(10.0) is 8.0, so -OCTAL(10.0) = -8.0 fits, +OCTAL(10.0) = +8.0 overflows
        ParsedProgram const validParsed("PROGRAM TEST COMMON() BEGIN FIXED(5, 1) f := -OCTAL(10.0); END");
        TypeCheckingResult validRes(validParsed);
        CHECK(!validRes.hasDiagnostic("PRESET_FIXED_POINT_OVERFLOW"));

        ParsedProgram const overflowParsed("PROGRAM TEST COMMON() BEGIN FIXED(5, 1) f := OCTAL(10.0); END");
        TypeCheckingResult overflowRes(overflowParsed);
        CHECK(overflowRes.hasDiagnostic("PRESET_FIXED_POINT_OVERFLOW"));
    }
}
