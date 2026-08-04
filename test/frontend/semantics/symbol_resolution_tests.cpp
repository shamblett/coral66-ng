/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 02/05/2026
 */

#include "lib/frontend/common/diagnostic_collection.hpp"
#include "lib/frontend/semantics/semantic_context.hpp"
#include "lib/frontend/semantics/symbol_resolution_pass.hpp"
#include "test/common/test_config.hpp"
#include "test/frontend/common/common.hpp"
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

    explicit SymbolResolutionResult(const ParsedProgram& parsed)
        : ss(outStr), context(parsed.cTokens, &ss, &errors), pass(context)
    {
        DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::clearDiagnostics();
        if (parsed.program.hasBlock())
            pass.run(parsed.program);
        for (auto d : DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics())
        {
            llvm::errs() << "DIAGNOSTIC: " << d.getName() << "\n";
        }
    }
};

const Usable* findDeclaration(const SemanticContext& ctx, const std::string& name)
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

        auto usable = findDeclaration(res.context, "I");
        CHECK(usable != nullptr);
        CHECK(usable->is<semantics::VariableDeclaration>());
        auto& variableDeclaration = usable->as<semantics::VariableDeclaration>();
        CHECK(variableDeclaration.getType().is<PrimitiveType>());
        CHECK(variableDeclaration.getType().as<PrimitiveType>().getKind() == PrimitiveType::Integer);

        CHECK(res.errors == false);
        CHECK(coral66::DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics().empty());
    }

    SECTION("Multiple Variable Declarations")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I, J, K ; END");
        SymbolResolutionResult res(parsed);

        CHECK(findDeclaration(res.context, "I") != nullptr);
        CHECK(findDeclaration(res.context, "J") != nullptr);
        CHECK(findDeclaration(res.context, "K") != nullptr);
        CHECK(res.errors == false);
    }

    SECTION("Different Type Declarations")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; FLOATING F ; FIXED (20,12) X ; END");
        SymbolResolutionResult res(parsed);

        auto iDeclaration = findDeclaration(res.context, "I");
        CHECK(iDeclaration != nullptr);
        CHECK(iDeclaration->as<semantics::VariableDeclaration>().getType().as<PrimitiveType>().getKind()
              == PrimitiveType::Integer);

        auto fDeclaration = findDeclaration(res.context, "F");
        CHECK(fDeclaration != nullptr);
        CHECK(fDeclaration->as<semantics::VariableDeclaration>().getType().as<PrimitiveType>().getKind()
              == PrimitiveType::Floating);

        auto xDeclaration = findDeclaration(res.context, "X");
        CHECK(xDeclaration != nullptr);
        CHECK(xDeclaration->as<semantics::VariableDeclaration>().getType().is<FixedType>());
        CHECK(xDeclaration->as<semantics::VariableDeclaration>().getType().as<FixedType>().getTotalBits() == 20);
        CHECK(xDeclaration->as<semantics::VariableDeclaration>().getType().as<FixedType>().getFractionalBits() == 12);
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
        auto diagnostics = DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics();
        CHECK(diagnostics.size() == 1);
        CHECK(diagnostics[0].getName() == "REDEFINITION_OF_SYMBOL_N");
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

        auto arrayDeclaration = findDeclaration(res.context, "A");
        CHECK(arrayDeclaration != nullptr);
        CHECK(arrayDeclaration->is<semantics::ArrayDeclaration>());
        CHECK(arrayDeclaration->as<semantics::ArrayDeclaration>().getName() == "A");
        CHECK(arrayDeclaration->as<semantics::ArrayDeclaration>().getType().is<ArrayType>());
    }

    SECTION("Parameter Modes")
    {
        ParsedProgram parsed(
            "PROGRAM TEST COMMON() BEGIN "
            "PROCEDURE P(VALUE INTEGER A; LOCATION INTEGER B; INTEGER ARRAY C; LABEL D; SWITCH E; PROCEDURE F; VALUE G; LOCATION H) ; "
            "BEGIN END ; END");
        SymbolResolutionResult res(parsed);

        auto usable = findDeclaration(res.context, "P");
        CHECK(usable != nullptr);
        CHECK(usable->is<semantics::ProcedureDeclaration>());
        auto& procType = usable->as<semantics::ProcedureDeclaration>().getType().as<ProcedureType>();
        CHECK(procType.getParameters().size() == 8);

        CHECK(procType.getParameters()[0].name == "A");
        CHECK(procType.getParameters()[0].isValue == true);
        CHECK(procType.getParameters()[0].type != nullptr);
        CHECK(procType.getParameters()[0].type->is<PrimitiveType>());

        CHECK(procType.getParameters()[1].name == "B");
        CHECK(procType.getParameters()[1].isValue == false);
        CHECK(procType.getParameters()[1].type != nullptr);
        CHECK(procType.getParameters()[1].type->is<PrimitiveType>());

        CHECK(procType.getParameters()[2].name == "C");
        CHECK(procType.getParameters()[2].type != nullptr);
        CHECK(procType.getParameters()[2].type->is<ArrayType>());

        CHECK(procType.getParameters()[3].name == "D");
        CHECK(procType.getParameters()[3].type != nullptr);
        CHECK(procType.getParameters()[3].type->is<LabelType>());

        CHECK(procType.getParameters()[4].name == "E");
        CHECK(procType.getParameters()[4].type != nullptr);
        CHECK(procType.getParameters()[4].type->is<SwitchType>());

        CHECK(procType.getParameters()[5].name == "F");
        CHECK(procType.getParameters()[5].type != nullptr);
        CHECK(procType.getParameters()[5].type->is<ProcedureType>());

        CHECK(procType.getParameters()[6].name == "G");
        CHECK(procType.getParameters()[6].isValue == true);
        CHECK(procType.getParameters()[6].type != nullptr);
        CHECK(procType.getParameters()[6].type->is<UntypedType>());

        CHECK(procType.getParameters()[7].name == "H");
        CHECK(procType.getParameters()[7].isValue == false); // Location
        CHECK(procType.getParameters()[7].type != nullptr);
        CHECK(procType.getParameters()[7].type->is<UntypedType>());
    }

    SECTION("Procedure Declaration and Parameters")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN PROCEDURE P(VALUE INTEGER X) ; BEGIN END ; END");
        SymbolResolutionResult res(parsed);

        auto procedureDeclaration = findDeclaration(res.context, "P");
        CHECK(procedureDeclaration != nullptr);
        CHECK(procedureDeclaration->is<semantics::ProcedureDeclaration>());
        CHECK(procedureDeclaration->as<semantics::ProcedureDeclaration>().getName() == "P");

        CHECK(res.errors == false);
    }

    SECTION("Switch Declaration")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN SWITCH sw := l1, l2, l3 ; l1: l2: l3: END");
        SymbolResolutionResult res(parsed);

        CHECK(res.errors == false);
    }

    SECTION("Table Element Declaration")
    {
        ParsedProgram parsed(
            "PROGRAM TEST COMMON() BEGIN TABLE t[1, 10] [ x INTEGER 0; y FIXED(16,8) 1 ] ; x[1] := 0 END");
        SymbolResolutionResult res(parsed);

        CHECK(res.errors == false);
    }

    SECTION("Table Element Properties (Part-Word)")
    {
        ParsedProgram parsed(
            "PROGRAM TEST COMMON() BEGIN TABLE t[1, 10] [ x UNSIGNED (10) 0, 5 ; y FIXED(22,11) 1; z (4) 2, 10 ] ; END");
        SymbolResolutionResult res(parsed);
        CHECK(res.errors == false);

        const Usable* xUsable = nullptr;
        const Usable* yUsable = nullptr;
        const Usable* zUsable = nullptr;
        for (const auto& u : res.context.ownedUsables)
        {
            if (u->is<semantics::TableElementDeclaration>())
            {
                auto name = u->as<semantics::TableElementDeclaration>().getName();
                if (name == "x")
                    xUsable = u.get();
                if (name == "y")
                    yUsable = u.get();
                if (name == "z")
                    zUsable = u.get();
            }
        }

        CHECK(xUsable != nullptr);
        auto& xDeclaration = xUsable->as<semantics::TableElementDeclaration>();
        CHECK(xDeclaration.isUnsigned() == true);
        CHECK(xDeclaration.getWordPosition().has_value());
        CHECK(xDeclaration.getWordPosition().value() == 0);
        CHECK(xDeclaration.getBitPosition().has_value());
        CHECK(xDeclaration.getBitPosition().value() == 5);
        CHECK(xDeclaration.getType().is<PrimitiveType>());
        CHECK(xDeclaration.getType().as<PrimitiveType>().getKind() == PrimitiveType::Integer);

        CHECK(yUsable != nullptr);
        auto& yDeclaration = yUsable->as<semantics::TableElementDeclaration>();
        CHECK(yDeclaration.isUnsigned() == false);
        CHECK(yDeclaration.getWordPosition().has_value());
        CHECK(yDeclaration.getWordPosition().value() == 1);
        CHECK(yDeclaration.getBitPosition().has_value() == false);
        CHECK(yDeclaration.getType().is<FixedType>());
        CHECK(yDeclaration.getType().as<FixedType>().getTotalBits() == 22);
        CHECK(yDeclaration.getType().as<FixedType>().getFractionalBits() == 11);

        CHECK(zUsable != nullptr);
        auto& zDeclaration = zUsable->as<semantics::TableElementDeclaration>();
        CHECK(zDeclaration.isUnsigned() == false);
        CHECK(zDeclaration.getWordPosition().has_value());
        CHECK(zDeclaration.getWordPosition().value() == 2);
        CHECK(zDeclaration.getBitPosition().has_value());
        CHECK(zDeclaration.getBitPosition().value() == 10);
        CHECK(zDeclaration.getType().is<PrimitiveType>());
        CHECK(zDeclaration.getType().as<PrimitiveType>().getKind() == PrimitiveType::Integer);
    }
}

TEST_CASE("Symbol Resolution - Switch Label Target Resolution", "[Semantics][SymbolResolution]")
{
    SECTION("Valid Switch Label Targets")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN SWITCH sw := l1, l2 ; l1: l2: END");
        SymbolResolutionResult res(parsed);
        CHECK(res.errors == false);
        CHECK(coral66::DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics().empty());
    }

    SECTION("Undeclared Switch Label Target")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN SWITCH sw := l1, l2 ; l1: END");
        SymbolResolutionResult res(parsed);
        CHECK(res.errors == true);
        auto diagnostics = DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics();
        bool foundError = false;
        for (auto& d : diagnostics)
            if (d.getName() == "UNDECLARED_IDENTIFIER_N")
                foundError = true;
        CHECK(foundError);
    }

    SECTION("Invalid Switch Label Target (Not a Label)")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; SWITCH sw := I ; END");
        SymbolResolutionResult res(parsed);
        CHECK(res.errors == true);
        auto diagnostics = DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics();
        bool foundError = false;
        for (auto& d : diagnostics)
            if (d.getName() == "LABEL_NOT_FOUND_N")
                foundError = true;
        CHECK(foundError);
    }
}

TEST_CASE("Symbol Resolution - Overlay Base Verification", "[Semantics][SymbolResolution]")
{
    SECTION("Valid Overlay Base")
    {
        const ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; OVERLAY I WITH INTEGER J ; END");
        const SymbolResolutionResult res(parsed);
        CHECK(res.errors == false);
    }

    SECTION("Undeclared Overlay Base")
    {
        const ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN OVERLAY I WITH INTEGER J ; END");
        const SymbolResolutionResult res(parsed);
        CHECK(res.errors == true);
        const auto diagnostics = DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics();
        bool foundError = false;
        for (auto& d : diagnostics)
            if (d.getName() == "UNDECLARED_IDENTIFIER_N")
                foundError = true;
        CHECK(foundError);
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
            if (auto* idExpr = dynamic_cast<const IdentifierExpression*>(node))
            {
                if (std::get<std::string>(idExpr->getIdentifier()->getValue()) == "I")
                {
                    found = true;
                    CHECK(usable != nullptr);
                    CHECK(usable->is<semantics::VariableDeclaration>());
                }
            }
        }
        CHECK(found);
    }

    SECTION("Valid Variable Usage With Common Declarations")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON(INTEGER I;) BEGIN INTEGER I ; I := 1 END");
        SymbolResolutionResult res(parsed);

        CHECK(res.errors == false);
        CHECK(coral66::DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics().empty());

        bool found = false;
        for (const auto& [node, usable] : res.context.resolvedIdentifiers)
        {
            if (auto* idExpr = dynamic_cast<const IdentifierExpression*>(node))
            {
                if (std::get<std::string>(idExpr->getIdentifier()->getValue()) == "I")
                {
                    found = true;
                    CHECK(usable != nullptr);
                    CHECK(usable->is<semantics::VariableDeclaration>());
                }
            }
        }
        CHECK(found);
    }
}

TEST_CASE("Symbol Resolution - Diagnostics", "[Semantics][SymbolResolution]")
{
    SECTION("Undeclared Variable Usage")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN I := 1 END");
        SymbolResolutionResult res(parsed);

        CHECK(res.errors == true);
        auto diagnostics = DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics();
        CHECK(diagnostics.size() == 1);
        CHECK(diagnostics[0].getName() == "UNDECLARED_IDENTIFIER_N");
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
        auto diagnostic = DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics();
        CHECK(!diagnostic.empty());
        bool hasUndeclared = false;
        for (const auto& diagnostics : diagnostic)
        {
            if (diagnostics.getName() == "UNDECLARED_IDENTIFIER_N")
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
        auto diagnostic = DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics();
        CHECK(!diagnostic.empty());
        bool hasUndeclared = false;
        for (const auto& diagnostics : diagnostic)
        {
            if (diagnostics.getName() == "UNDECLARED_IDENTIFIER_N")
                hasUndeclared = true;
        }
        CHECK(hasUndeclared);
    }
}

TEST_CASE("Symbol Resolution - Labels", "[Semantics][SymbolResolution]")
{
    SECTION("Label Declaration and Pre-scanning")
    {
        const ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; l1: I := 1 END");
        const SymbolResolutionResult res(parsed);
        CHECK(res.errors == false);
        CHECK(coral66::DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics().empty());
    }

    SECTION("Label Redefinition Error")
    {
        const ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; l1: l1: I := 1 END");
        const SymbolResolutionResult res(parsed);
        CHECK(res.errors == true);
        const auto diagnostics = DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics();
        bool foundRedef = false;
        for (auto& d : diagnostics)
            if (d.getName() == "REDEFINITION_OF_SYMBOL_N")
                foundRedef = true;
        CHECK(foundRedef);
    }
}

TEST_CASE("Symbol Resolution - Extended Redefinitions", "[Semantics][SymbolResolution]")
{
    SECTION("Recursive Procedure")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN RECURSIVE PROCEDURE P ; BEGIN P END ; END");
        SymbolResolutionResult res(parsed);
        CHECK(res.errors == false);
    }
    SECTION("Procedure Redefinition Error")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN PROCEDURE P; BEGIN END; PROCEDURE P; BEGIN END; END");
        SymbolResolutionResult res(parsed);
        CHECK(res.errors == true);
    }
    SECTION("Parameter Redefinition Error")
    {
        ParsedProgram parsed(
            "PROGRAM TEST COMMON() BEGIN PROCEDURE P(VALUE INTEGER X; VALUE INTEGER X); BEGIN END; END");
        SymbolResolutionResult res(parsed);
        CHECK(res.errors == true);
    }
    SECTION("Array Redefinition Error")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10]; INTEGER ARRAY A[1:10]; END");
        SymbolResolutionResult res(parsed);
        CHECK(res.errors == true);
    }
    SECTION("Table Redefinition Error")
    {
        ParsedProgram parsed(
            "PROGRAM TEST COMMON() BEGIN TABLE t[1, 10] [ x INTEGER 0 ] ; TABLE t[1, 10] [ x INTEGER 0 ] ; END");
        SymbolResolutionResult res(parsed);
        CHECK(res.errors == true);
    }
    SECTION("Table Element Redefinition Error")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN TABLE t[1, 10] [ x INTEGER 0; x INTEGER 1 ] ; END");
        SymbolResolutionResult res(parsed);
        CHECK(res.errors == true);
    }
    SECTION("Switch Redefinition Error")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN SWITCH sw := l1; SWITCH sw := l1; l1: END");
        SymbolResolutionResult res(parsed);
        CHECK(res.errors == true);
    }
    SECTION("Overlay Declaration")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; OVERLAY I WITH INTEGER J ; END");
        SymbolResolutionResult res(parsed);
        CHECK(res.errors == false);
    }
}

TEST_CASE("Symbol Resolution - Complex Statements", "[Semantics][SymbolResolution]")
{
    SECTION("For Statement")
    {
        ParsedProgram parsed(
            "PROGRAM TEST COMMON() BEGIN INTEGER I ; FOR I := 1 STEP 1 UNTIL 10 WHILE I < 5 DO I := 1 END");
        SymbolResolutionResult res(parsed);
        CHECK(res.errors == false);
    }
    SECTION("If Statement")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; IF I = 1 THEN I := 2 ELSE I := 3 END");
        SymbolResolutionResult res(parsed);
        CHECK(res.errors == false);
    }
    SECTION("Compound Statement")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; BEGIN I := 1; I := 2 END END");
        SymbolResolutionResult res(parsed);
        CHECK(res.errors == false);
    }
    SECTION("Answer Statement")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; INTEGER PROCEDURE P; BEGIN ANSWER I END ; END");
        SymbolResolutionResult res(parsed);
        CHECK(res.errors == false);
    }
    SECTION("Goto Statement Switch")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN SWITCH sw := l1; GOTO sw[1]; l1: END");
        SymbolResolutionResult res(parsed);
        CHECK(res.errors == false);
    }
}

TEST_CASE("Symbol Resolution - Goto Targets", "[Semantics][SymbolResolution]")
{
    SECTION("Valid Goto Label Target")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN GOTO l1; l1: END");
        SymbolResolutionResult res(parsed);
        CHECK(res.errors == false);
        CHECK(coral66::DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics().empty());
    }

    SECTION("Valid Goto Switch Target")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN SWITCH sw := l1; GOTO sw[1]; l1: END");
        SymbolResolutionResult res(parsed);
        CHECK(res.errors == false);
        CHECK(coral66::DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics().empty());
    }

    SECTION("Undeclared Goto Target")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN GOTO l1; END");
        SymbolResolutionResult res(parsed);
        CHECK(res.errors == true);
        auto diagnostics = DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics();
        bool foundError = false;
        for (auto& d : diagnostics)
            if (d.getName() == "UNDECLARED_IDENTIFIER_N")
                foundError = true;
        CHECK(foundError);
    }

    SECTION("Invalid Goto Target (Variable instead of Label)")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER l1; GOTO l1; END");
        SymbolResolutionResult res(parsed);
        CHECK(res.errors == true);
        auto diagnostics = DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics();
        bool foundError = false;
        for (auto& d : diagnostics)
            if (d.getName() == "LABEL_NOT_FOUND_N")
                foundError = true;
        CHECK(foundError);
    }
}

TEST_CASE("Symbol Resolution - Procedure Signatures", "[Semantics][SymbolResolution]")
{
    SECTION("Procedure Retains Parameters and Modes")
    {
        ParsedProgram parsed(
            "PROGRAM TEST COMMON() BEGIN PROCEDURE P(VALUE INTEGER X; LOCATION FLOATING Y; INTEGER ARRAY Z) ; BEGIN END ; END");
        SymbolResolutionResult res(parsed);

        auto usable = findDeclaration(res.context, "P");
        CHECK(usable != nullptr);
        CHECK(usable->is<semantics::ProcedureDeclaration>());
        auto& procDeclaration = usable->as<semantics::ProcedureDeclaration>();

        CHECK(procDeclaration.getType().is<ProcedureType>());
        auto& procType = procDeclaration.getType().as<ProcedureType>();

        CHECK(procType.getParameters().size() == 3);

        CHECK(procType.getParameters()[0].name == "X");
        CHECK(procType.getParameters()[0].isValue == true);
        CHECK(procType.getParameters()[0].type != nullptr);
        CHECK(procType.getParameters()[0].type->is<PrimitiveType>());

        CHECK(procType.getParameters()[1].name == "Y");
        CHECK(procType.getParameters()[1].isValue == false);
        CHECK(procType.getParameters()[1].type != nullptr);
        CHECK(procType.getParameters()[1].type->is<PrimitiveType>());

        CHECK(procType.getParameters()[2].name == "Z");
        CHECK(procType.getParameters()[2].isValue == false);
        CHECK(procType.getParameters()[2].type != nullptr);
        CHECK(procType.getParameters()[2].type->is<ArrayType>());
    }

    SECTION("Procedure Return Type")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER PROCEDURE P ; BEGIN ANSWER 1 END ; END");
        SymbolResolutionResult res(parsed);

        auto usable = findDeclaration(res.context, "P");
        CHECK(usable != nullptr);
        CHECK(usable->is<semantics::ProcedureDeclaration>());

        auto& procType = usable->as<semantics::ProcedureDeclaration>().getType().as<ProcedureType>();
        CHECK(procType.getReturnType() != nullptr);
        CHECK(procType.getReturnType()->is<PrimitiveType>());
        CHECK(procType.getReturnType()->as<PrimitiveType>().getKind() == PrimitiveType::Integer);
    }
}

TEST_CASE("Symbol Resolution - Array AST Linking", "[Semantics][SymbolResolution]")
{
    SECTION("Array Declaration AST Node Tracked")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER ARRAY A[1:10] ; END");
        SymbolResolutionResult res(parsed);

        auto usable = findDeclaration(res.context, "A");
        CHECK(usable != nullptr);
        CHECK(usable->is<semantics::ArrayDeclaration>());

        auto& arrayDeclaration = usable->as<semantics::ArrayDeclaration>();
        CHECK(arrayDeclaration.getAstNode() != nullptr);

        auto* syntaxDeclaration = dynamic_cast<const syntax::ArrayDeclaration*>(arrayDeclaration.getAstNode());
        CHECK(syntaxDeclaration != nullptr);
        CHECK(syntaxDeclaration->getDimensions().size() == 1);
    }
}

TEST_CASE("Symbol Resolution - Table Memory and AST Linking", "[Semantics][SymbolResolution]")
{
    SECTION("Table Declaration Memory and AST Node Tracked")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN TABLE t[1, 10] [ x INTEGER 0 ] ; END");
        SymbolResolutionResult res(parsed);

        const Usable* tableUsable = nullptr;
        for (const auto& u : res.context.ownedUsables)
        {
            if (u->is<semantics::TableDeclaration>())
            {
                tableUsable = u.get();
                break;
            }
        }

        CHECK(tableUsable != nullptr);
        CHECK(tableUsable->is<semantics::TableDeclaration>());
        auto& tableDeclaration = tableUsable->as<semantics::TableDeclaration>();

        // AST Node Verification
        CHECK(tableDeclaration.getAstNode() != nullptr);
        auto* syntaxDeclaration = dynamic_cast<const syntax::TableDeclaration*>(tableDeclaration.getAstNode());
        CHECK(syntaxDeclaration != nullptr);
        CHECK(std::get<std::string>(syntaxDeclaration->getIdentifier()->getValue()) == "t");

        // Type Memory Verification
        CHECK(tableDeclaration.getType().is<TableType>());
        CHECK(tableDeclaration.getType().as<TableType>().getInfo().name == "t");
        CHECK(res.context.ownedTableInfos.size() == 1); // Ensure memory is tracked
    }
}

TEST_CASE("Symbol Resolution - Expressions", "[Semantics][SymbolResolution]")
{
    SECTION("Binary and Unary Expressions")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I, J, K ; I := J + -K END");
        SymbolResolutionResult res(parsed);
        CHECK(res.errors == false);
    }
    SECTION("Part Word Expression")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I, J ; I := BITS [8, 4] J END");
        SymbolResolutionResult res(parsed);
        CHECK(res.errors == false);
    }
    SECTION("Conditional Expression")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; I := IF I = 0 THEN 1 ELSE 2 END");
        SymbolResolutionResult res(parsed);
        CHECK(res.errors == false);
    }
    SECTION("Function Call Expression")
    {
        ParsedProgram parsed(
            "PROGRAM TEST COMMON() BEGIN INTEGER PROCEDURE P; BEGIN ANSWER 1 END ; INTEGER I ; I := P END");
        SymbolResolutionResult res(parsed);
        CHECK(res.errors == false);
    }
    SECTION("Location Expression")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I, J ; I := LOCATION ( J ) END");
        SymbolResolutionResult res(parsed);
        CHECK(res.errors == false);
    }
    SECTION("Type Change Expression")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON() BEGIN INTEGER I ; FLOATING F ; I := INTEGER ( F ) END");
        SymbolResolutionResult res(parsed);
        CHECK(res.errors == false);
    }
}
