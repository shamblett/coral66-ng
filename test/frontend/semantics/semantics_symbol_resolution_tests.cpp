/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "lib/frontend/semantics/symbol_resolution_pass.hpp"

#include "lib/frontend/common/diagnostic_collection.hpp"
#include "lib/frontend/common/messages/errors.hpp"
#include "lib/frontend/parser/parser.hpp"
#include "lib/frontend/semantics/semantic_context.hpp"
#include "test/common/test_config.hpp"
#include "test/frontend/parser/common.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66;
using namespace coral66::semantics;

// Helper to run symbol resolution pass
struct SymbolResolutionResult
{
    std::string outStr;
    llvm::raw_string_ostream ss;
    bool errors = false;
    SemanticContext context;
    SymbolResolutionPass pass;

    SymbolResolutionResult(const ParsedProgram& parsed)
        : ss(outStr), context(parsed.cTokens, &ss, &errors), pass(context)
    {
        coral66::DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::clearDiagnostics();
        if (parsed.program.hasBlock()) pass.run(parsed.program);
        for (auto d : coral66::DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics()) {
            llvm::errs() << "DIAGNOSTIC: " << d.getName() << "\n";
        }
    }
};

const Usable* findDecl(const SemanticContext& ctx, const std::string& name)
{
    for (const auto& u : ctx.ownedUsables)
    {
        if (u->is<semantics::VariableDeclaration>() && u->as<semantics::VariableDeclaration>().getName() == name)
            return u.get();
        if (u->is<semantics::ProcedureDeclaration>() && u->as<semantics::ProcedureDeclaration>().getName() == name)
            return u.get();
        if (u->is<semantics::ArrayDeclaration>() && u->as<semantics::ArrayDeclaration>().getName() == name)
            return u.get();
    }
    return nullptr;
}

TEST_CASE("Symbol Resolution - Basic Declarations", "[Semantics][SymbolResolution]")
{
    SECTION("Single Variable Declaration")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; END");
        SymbolResolutionResult res(parsed);

        auto usable = findDecl(res.context, "I");
        REQUIRE(usable != nullptr);
        REQUIRE(usable->is<semantics::VariableDeclaration>());
        auto& varDecl = usable->as<semantics::VariableDeclaration>();
        CHECK(varDecl.getType().is<PrimitiveType>());
        CHECK(varDecl.getType().as<PrimitiveType>().getKind() == PrimitiveType::Integer);
        
        CHECK(res.errors == false);
        CHECK(coral66::DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics().empty());
    }

    SECTION("Multiple Variable Declarations")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I, J, K ; END");
        SymbolResolutionResult res(parsed);

        REQUIRE(findDecl(res.context, "I") != nullptr);
        REQUIRE(findDecl(res.context, "J") != nullptr);
        REQUIRE(findDecl(res.context, "K") != nullptr);
        CHECK(res.errors == false);
    }

    SECTION("Different Type Declarations")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; FLOATING F ; FIXED (16,8) X ; END");
        SymbolResolutionResult res(parsed);

        auto iDecl = findDecl(res.context, "I");
        REQUIRE(iDecl != nullptr);
        CHECK(iDecl->as<semantics::VariableDeclaration>().getType().as<PrimitiveType>().getKind() == PrimitiveType::Integer);

        auto fDecl = findDecl(res.context, "F");
        REQUIRE(fDecl != nullptr);
        CHECK(fDecl->as<semantics::VariableDeclaration>().getType().as<PrimitiveType>().getKind() == PrimitiveType::Floating);

        auto xDecl = findDecl(res.context, "X");
        REQUIRE(xDecl != nullptr);
        REQUIRE(xDecl->as<semantics::VariableDeclaration>().getType().is<FixedType>());
    }
}

TEST_CASE("Symbol Resolution - Scopes", "[Semantics][SymbolResolution]")
{
    SECTION("Nested Scopes Variables")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; BEGIN INTEGER J ; END ; END");
        SymbolResolutionResult res(parsed);
        CHECK(res.errors == false);
        CHECK(coral66::DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics().empty());
    }
    
    SECTION("Redefinition in Same Scope Error")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; INTEGER I ; END");
        SymbolResolutionResult res(parsed);

        CHECK(res.errors == true);
        auto diags = coral66::DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics();
        REQUIRE(diags.size() == 1);
        CHECK(diags[0].getName() == "REDEFINITION_OF_SYMBOL_N");
    }

    SECTION("Shadowing in Nested Scope")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; BEGIN INTEGER I ; END ; END");
        SymbolResolutionResult res(parsed);
        
        // This is valid in Coral 66 block structure
        CHECK(res.errors == false);
        CHECK(coral66::DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics().empty());
    }
}

TEST_CASE("Symbol Resolution - Complex Declarations", "[Semantics][SymbolResolution]")
{
    SECTION("Array Declaration")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; END");
        SymbolResolutionResult res(parsed);

        auto aDecl = findDecl(res.context, "A");
        REQUIRE(aDecl != nullptr);
        REQUIRE(aDecl->is<semantics::ArrayDeclaration>());
        CHECK(aDecl->as<semantics::ArrayDeclaration>().getName() == "A");
        CHECK(aDecl->as<semantics::ArrayDeclaration>().getType().is<ArrayType>());
    }

    SECTION("Procedure Declaration and Parameters")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN PROCEDURE P(VALUE INTEGER X) ; BEGIN END ; END");
        SymbolResolutionResult res(parsed);

        auto pDecl = findDecl(res.context, "P");
        REQUIRE(pDecl != nullptr);
        REQUIRE(pDecl->is<semantics::ProcedureDeclaration>());
        CHECK(pDecl->as<semantics::ProcedureDeclaration>().getName() == "P");
        
        CHECK(res.errors == false);
    }
}

TEST_CASE("Symbol Resolution - Identifier Resolution", "[Semantics][SymbolResolution]")
{
    SECTION("Valid Variable Usage")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := 1 END");
        SymbolResolutionResult res(parsed);

        CHECK(res.errors == false);
        CHECK(coral66::DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics().empty());
        
        bool found = false;
        for (const auto& [node, usable] : res.context.resolvedIdentifiers)
        {
            if (auto* idExpr = dynamic_cast<const syntax::IdentifierExpression*>(node))
            {
                if (std::get<std::string>(idExpr->getIdentifier()->getValue()) == "I")
                {
                    found = true;
                    REQUIRE(usable != nullptr);
                    CHECK(usable->is<semantics::VariableDeclaration>());
                }
            }
        }
        CHECK(found);
    }

    SECTION("Undeclared Variable Usage")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN I := 1 END");
        SymbolResolutionResult res(parsed);

        CHECK(res.errors == true);
        auto diags = coral66::DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics();
        REQUIRE(diags.size() == 1);
        CHECK(diags[0].getName() == "UNDECLARED_IDENTIFIER_N");
    }
    
    SECTION("Function Call Resolution")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN PROCEDURE P ; BEGIN END ; P END");
        SymbolResolutionResult res(parsed);

        CHECK(res.errors == false);
    }

    SECTION("Undeclared Function Call")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN P END");
        SymbolResolutionResult res(parsed);

        CHECK(res.errors == true);
        auto diags = coral66::DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics();
        REQUIRE(diags.size() > 0);
        bool hasUndeclared = false;
        for (const auto& diag : diags)
        {
            if (diag.getName() == "UNDECLARED_IDENTIFIER_N")
                hasUndeclared = true;
        }
        CHECK(hasUndeclared);
    }
    
    SECTION("Array Access Resolution")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; A[1] := 0 END");
        SymbolResolutionResult res(parsed);

        CHECK(res.errors == false);
    }

    SECTION("Undeclared Array Access")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN A[1] := 0 END");
        SymbolResolutionResult res(parsed);

        CHECK(res.errors == true);
        auto diags = coral66::DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics();
        REQUIRE(diags.size() > 0);
        bool hasUndeclared = false;
        for (const auto& diag : diags)
        {
            if (diag.getName() == "UNDECLARED_IDENTIFIER_N")
                hasUndeclared = true;
        }
        CHECK(hasUndeclared);
    }
}
