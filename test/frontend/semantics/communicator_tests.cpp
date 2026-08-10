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

namespace
{
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
} // namespace

static const Usable* findDeclarationComm(const SemanticContext& ctx, const std::string& name)
{
    for (const auto& u : ctx.ownedUsables)
    {
        if (u->is<semantics::VariableDeclaration>() && u->as<semantics::VariableDeclaration>().getName() == name)
            return u.get();
        if (u->is<semantics::ProcedureDeclaration>() && u->as<semantics::ProcedureDeclaration>().getName() == name)
            return u.get();
        if (u->is<semantics::ArrayDeclaration>() && u->as<semantics::ArrayDeclaration>().getName() == name)
            return u.get();
        if (u->is<semantics::TableDeclaration>() && u->as<semantics::TableDeclaration>().getName() == name)
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

        CHECK(findDeclarationComm(res.context, "A") != nullptr);
        CHECK(findDeclarationComm(res.context, "B") != nullptr);
        CHECK(findDeclarationComm(res.context, "I") != nullptr);
        CHECK(res.errors == false);
    }

    SECTION("Procedure Specification in Common")
    {
        ParsedProgram parsed(
            "PROGRAM TEST COMMON(INTEGER PROCEDURE P(VALUE INTEGER)) BEGIN INTEGER I ; I := P(10) END");
        SymbolResolutionResult res(parsed);

        auto procedureDeclaration = findDeclarationComm(res.context, "P");
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

        auto procedureDeclaration = findDeclarationComm(res.context, "P");
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
        const ParsedProgram parsed("PROGRAM TEST LIBRARY(NONEXISTENTLIB) BEGIN INTEGER I ; END");
        const SymbolResolutionResult res(parsed);

        CHECK(res.errors == true);
        const auto diagnostics = DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics();
        bool foundError = false;
        for (auto& d : diagnostics)
            if (d.getName() == "FAILED_TO_READ_CMI_FILE_N")
                foundError = true;
        CHECK(foundError);
    }
}

TEST_CASE("Symbol Resolution - Absolute Communicators", "[Semantics][SymbolResolution][Communicator]")
{
    SECTION("Valid Absolute Variables With Constant Addresses")
    {
        ParsedProgram parsed("PROGRAM TEST ABSOLUTE ( INTEGER X / 1024, Y / 2048 ) BEGIN END");
        SymbolResolutionResult res(parsed);

        CHECK(res.errors == false);

        auto xVar = findDeclarationComm(res.context, "X");
        CHECK(xVar != nullptr);
        auto varDeclX = xVar->tryAs<semantics::VariableDeclaration>();
        CHECK(varDeclX != nullptr);
        CHECK(varDeclX->getAbsoluteAddress() == 1024);

        auto yVar = findDeclarationComm(res.context, "Y");
        CHECK(yVar != nullptr);
        auto varDeclY = yVar->tryAs<semantics::VariableDeclaration>();
        CHECK(varDeclY != nullptr);
        CHECK(varDeclY->getAbsoluteAddress() == 2048);
    }

    SECTION("Absolute Arrays With Address")
    {
        ParsedProgram parsed("PROGRAM TEST ABSOLUTE ( INTEGER ARRAY A / 4096 [1:10] ) BEGIN END");
        SymbolResolutionResult res(parsed);

        CHECK(res.errors == false);

        auto aVar = findDeclarationComm(res.context, "A");
        CHECK(aVar != nullptr);
        auto arrDecl = aVar->tryAs<semantics::ArrayDeclaration>();
        CHECK(arrDecl != nullptr);
        CHECK(arrDecl->getAbsoluteAddress() == 4096);
    }

    SECTION("Absolute Tables With Address")
    {
        ParsedProgram parsed("PROGRAM TEST ABSOLUTE ( TABLE T / 8192 [10, 5] [ I INTEGER 0 ] ) BEGIN END");
        SymbolResolutionResult res(parsed);

        CHECK(res.errors == false);

        auto tVar = findDeclarationComm(res.context, "T");
        REQUIRE(tVar != nullptr);
        auto tabDecl = tVar->tryAs<semantics::TableDeclaration>();
        REQUIRE(tabDecl != nullptr);
        CHECK(tabDecl->getAbsoluteAddress() == 8192);
    }

    SECTION("Absolute Address Evaluation Failure")
    {
        ParsedProgram parsed("PROGRAM TEST ABSOLUTE ( INTEGER X / Y ) BEGIN END");
        SymbolResolutionResult res(parsed);

        CHECK(res.errors == true);
        auto diagnostics = DiagnosticCollection<coral66::detail::Diagnostic::DiagnosticBase>::getDiagnostics();
        bool foundError = false;
        for (auto& d : diagnostics)
            if (d.getName() == "EXPECTED_CONSTANT_EXPRESSION")
                foundError = true;
        CHECK(foundError);
    }
}
