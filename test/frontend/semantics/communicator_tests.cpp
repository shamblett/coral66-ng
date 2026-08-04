/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 02/05/2026
 */

#include <iostream>

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
        pass.run(parsed.program); // always call pass.run(parsed.program)
    }
};

inline const Usable* findDeclaration(const SemanticContext& ctx, const std::string& name)
{
    for (const auto& u : ctx.ownedUsables)
    {
        if (u->is<semantics::VariableDeclaration>() && u->as<semantics::VariableDeclaration>().getName() == name)
            return u.get();
        if (u->is<semantics::ProcedureDeclaration>() && u->as<semantics::ProcedureDeclaration>().getName() == name)
            return u.get();
        if (u->is<semantics::ArrayDeclaration>() && u->as<semantics::ArrayDeclaration>().getName() == name)
            return u.get();
        if (u->is<LabelDeclaration>() && u->as<LabelDeclaration>().getName() == name)
            return u.get();
    }
    return nullptr;
}

TEST_CASE("Symbol Resolution - Common Communicators", "[Semantics][SymbolResolution][Communicator]")
{
    SECTION("Variable Declarations in Common")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON(INTEGER A; FLOATING B) BEGIN INTEGER I ; END");
        SymbolResolutionResult res(parsed);

        CHECK(findDeclaration(res.context, "A") != nullptr);
        CHECK(findDeclaration(res.context, "B") != nullptr);
        CHECK(findDeclaration(res.context, "I") != nullptr);
        CHECK(res.errors == false);
    }

    SECTION("Procedure Specification in Common")
    {
        ParsedProgram parsed(
            "PROGRAM TEST COMMON(INTEGER PROCEDURE P(VALUE INTEGER)) BEGIN INTEGER I ; I := P(10) END");
        SymbolResolutionResult res(parsed);

        auto procedureDeclaration = findDeclaration(res.context, "P");
        CHECK(procedureDeclaration != nullptr);
        CHECK(procedureDeclaration->is<semantics::ProcedureDeclaration>());
        CHECK(res.errors == false);
    }

    SECTION("Place Specification in Common")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON(LABEL L1, L2) BEGIN INTEGER I ; END");
        SymbolResolutionResult res(parsed);

        CHECK(res.errors == false);
    }

    SECTION("Exported symbols captured correctly")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON(INTEGER A; FLOATING B) BEGIN INTEGER I ; END");
        SymbolResolutionResult res(parsed);

        REQUIRE(res.context.exportedSymbols.size() == 2);
        CHECK(res.context.exportedSymbols[0]->as<semantics::VariableDeclaration>().getName() == "A");
        CHECK(res.context.exportedSymbols[1]->as<semantics::VariableDeclaration>().getName() == "B");
        CHECK(res.errors == false);
    }

    SECTION("Error on duplicate symbol in COMMON communicator")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON(INTEGER A; FLOATING A) BEGIN END");
        SymbolResolutionResult res(parsed);

        CHECK(res.errors == true);
        auto diagnostics = DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics();
        bool foundRedef = false;
        for (auto& d : diagnostics)
            if (d.getName() == "REDEFINITION_OF_SYMBOL_N")
                foundRedef = true;
        CHECK(foundRedef);
    }

    SECTION("Error on mismatching common label and inner variable")
    {
        ParsedProgram parsed("PROGRAM TEST COMMON(LABEL L1) BEGIN INTEGER L1; L1: END");
        SymbolResolutionResult res(parsed);

        CHECK(res.errors == true);
        auto diagnostics = DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics();
        bool foundRedef = false;
        for (auto& d : diagnostics)
            if (d.getName() == "REDEFINITION_OF_SYMBOL_N")
                foundRedef = true;
        CHECK(foundRedef);
    }
}

TEST_CASE("Symbol Resolution - External Communicators", "[Semantics][SymbolResolution][Communicator]")
{
    SECTION("Procedure Specification in External")
    {
        const ParsedProgram parsed("PROGRAM TEST EXTERNAL(INTEGER PROCEDURE P(VALUE INTEGER))");
        const SymbolResolutionResult res(parsed);

        auto procedureDeclaration = findDeclaration(res.context, "P");
        CHECK(procedureDeclaration != nullptr);
        CHECK(procedureDeclaration->is<semantics::ProcedureDeclaration>());
        CHECK(res.errors == false);
    }

    SECTION("Error on duplicate symbol in EXTERNAL communicator")
    {
        ParsedProgram parsed(
            "PROGRAM TEST EXTERNAL(INTEGER PROCEDURE P(VALUE INTEGER); INTEGER PROCEDURE P(VALUE INTEGER))");
        SymbolResolutionResult res(parsed);

        CHECK(res.errors == true);
        auto diagnostics = DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics();
        bool foundRedef = false;
        for (auto& d : diagnostics)
            if (d.getName() == "REDEFINITION_OF_SYMBOL_N")
                foundRedef = true;
        CHECK(foundRedef);
    }
}

TEST_CASE("Symbol Resolution - Library Communicators", "[Semantics][SymbolResolution][Communicator]")
{
    SECTION("Error when library CMI file cannot be read")
    {
        ParsedProgram parsed("PROGRAM TEST LIBRARY(NONEXISTENTLIB) BEGIN INTEGER I ; END");
        SymbolResolutionResult res(parsed);

        CHECK(res.errors == true);
        auto diagnostics = DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics();
        bool foundError = false;
        for (auto& d : diagnostics)
            if (d.getName() == "FAILED_TO_READ_CMI_FILE_N")
                foundError = true;
        CHECK(foundError);
    }
}
