/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 25/04/2026
 */

#include "lib/frontend/common/diagnostic_collection.hpp"
#include "lib/frontend/parser/parser.hpp"
#include "lib/frontend/semantics/semantic_context.hpp"
#include "lib/frontend/semantics/symbol_resolution_pass.hpp"
#include "lib/frontend/semantics/type_checking_pass.hpp"
#include "test/common/test_config.hpp"
#include "test/frontend/parser/common.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66;
using namespace coral66::semantics;

struct TypeCheckingResult
{
    std::string outStr;
    llvm::raw_string_ostream ss;
    bool errors = false;
    SemanticContext context;
    SymbolResolutionPass symPass;
    TypeCheckingPass typePass;

    TypeCheckingResult(const ParsedProgram& parsed, bool runTypePass = true)
        : ss(outStr), context(parsed.cTokens, &ss, &errors), symPass(context), typePass(context)
    {
        DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::clearDiagnostics();
        REQUIRE(parsed.program.hasBlock());
        if (parsed.program.hasBlock())
        {
            symPass.run(parsed.program);
            if (runTypePass)
                typePass.run(parsed.program);
        }
    }
    
    bool hasDiagnostic(std::string_view name) const
    {
        for (auto d : DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics())
        {
            if (d.getName() == name) return true;
        }
        return false;
    }
};

TEST_CASE("Type Checking - IF Condition", "[Semantics][TypeChecking]")
{
    SECTION("Valid Integer Condition")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; IF I = 0 THEN I := 1 END");
        TypeCheckingResult res(parsed);
        REQUIRE(!res.hasDiagnostic("EXPECTED_INTEGER_TYPE"));
    }

    SECTION("Invalid Floating Condition")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN FLOATING F ; IF F = 0.0 THEN F := 1.0 END");
        TypeCheckingResult res(parsed);
        REQUIRE(res.hasDiagnostic("EXPECTED_INTEGER_TYPE"));
    }
}

TEST_CASE("Type Checking - GOTO Index", "[Semantics][TypeChecking]")
{
    SECTION("Valid Integer Index")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; GOTO s[I] END");
        TypeCheckingResult res(parsed);
        REQUIRE(!res.hasDiagnostic("EXPECTED_INTEGER_TYPE"));
    }

    SECTION("Invalid Floating Index")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN FLOATING F ; GOTO s[F] END");
        TypeCheckingResult res(parsed);
        REQUIRE(res.hasDiagnostic("EXPECTED_INTEGER_TYPE"));
    }
}

TEST_CASE("Type Checking - Array Access", "[Semantics][TypeChecking]")
{
    SECTION("Valid Integer Index")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; INTEGER I ; A[I] := 1 END");
        TypeCheckingResult res(parsed);
        REQUIRE(!res.hasDiagnostic("EXPECTED_INTEGER_TYPE"));
    }

    SECTION("Invalid Floating Index")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; FLOATING F ; A[F] := 1 END");
        TypeCheckingResult res(parsed);
        REQUIRE(res.hasDiagnostic("EXPECTED_INTEGER_TYPE"));
    }
}

TEST_CASE("Type Checking - Assignment", "[Semantics][TypeChecking]")
{
    SECTION("Type Mismatch Assigning Array to Variable")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; INTEGER I ; I := A END");
        TypeCheckingResult res(parsed);
        REQUIRE(res.hasDiagnostic("TYPE_MISMATCH"));
    }

    SECTION("Type Mismatch Assigning Variable to Array")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; INTEGER I ; A := I END");
        TypeCheckingResult res(parsed);
        REQUIRE(res.hasDiagnostic("TYPE_MISMATCH"));
    }
}

TEST_CASE("Type Checking - Binary Expression", "[Semantics][TypeChecking]")
{
    SECTION("Valid Binary Expression")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := I + 1 END");
        TypeCheckingResult res(parsed);
        REQUIRE(!res.hasDiagnostic("TYPE_MISMATCH"));
    }

    SECTION("Type Mismatch Array in Binary Expression")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; INTEGER I ; I := A + 1 END");
        TypeCheckingResult res(parsed);
        REQUIRE(res.hasDiagnostic("TYPE_MISMATCH"));
    }
}

TEST_CASE("Type Checking - Procedure Calls", "[Semantics][TypeChecking]")
{
    SECTION("Argument Count Mismatch")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN PROCEDURE P(VALUE INTEGER X) ; BEGIN END ; P(1) END");
        TypeCheckingResult res(parsed);
        REQUIRE(res.hasDiagnostic("PROCEDURE_ARGUMENT_COUNT_MISMATCH"));
    }
    
    SECTION("Valid Zero Argument Call")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN PROCEDURE P ; BEGIN END ; P END");
        TypeCheckingResult res(parsed);
        REQUIRE(!res.hasDiagnostic("PROCEDURE_ARGUMENT_COUNT_MISMATCH"));
        REQUIRE(!res.hasDiagnostic("PROCEDURE_ARGUMENT_TYPE_MISMATCH"));
    }
    
    SECTION("Argument Type Mismatch")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN P(1.0) END");
        TypeCheckingResult res(parsed, false);
        
        static PrimitiveType intType(PrimitiveType::Integer);
        std::vector<ProcedureType::Parameter> params;
        params.push_back({&intType, "X", true});
        
        auto procType = IntrusiveVariantValue<Type>(ProcedureType{nullptr, std::move(params)});
        
        auto it = parsed.cTokens.data().begin();
        auto nameToken = &(*it);
        
        auto pDecl = IntrVarPtr<Usable>(new semantics::ProcedureDeclaration(nameToken, std::move(procType)));
        
        res.context.symbolTable.insert("P", pDecl.get());
        
        if (parsed.program.hasBlock()) {
            for (const auto& stmt : parsed.program.getBlock().getStatements()) {
                if (auto callStmt = dynamic_cast<const syntax::ProcedureCallStatement*>(stmt.get())) {
                    if (auto callExpr = dynamic_cast<const syntax::FunctionCallExpression*>(&callStmt->getExpression())) {
                        res.context.resolvedIdentifiers[callExpr] = pDecl.get();
                    }
                }
            }
        }
        res.context.ownedUsables.push_back(std::move(pDecl));
        
        res.typePass.run(parsed.program);
        
        REQUIRE(res.hasDiagnostic("PROCEDURE_ARGUMENT_TYPE_MISMATCH"));
    }
}
