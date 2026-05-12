/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 07/05/2026
 */

#include "lib/frontend/semantics/semantics.hpp"

#include <iostream>

#include "lib/frontend/preprocessor/preprocessor.hpp"
#include "lib/frontend/semantics/semantic_support.hpp"
#include "test/common/test_config.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66;
using namespace coral66::syntax;
using namespace coral66::Tests;

/*
 * Generated code tests
 */

TEST_CASE("Generated", "[Semantics][Generated]")
{
    SECTION("G1")
    {
        auto sourcePath = getTestDataPath("g1.cor", "GENERATED_DIR");
        auto source = readFile(sourcePath);
        CHECK(!source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);
        auto ppTokens = lexer::tokenize(source, &ss);
        ppTokens = pp::preprocess(std::move(ppTokens), &ss);
        auto cTokens = lexer::toCTokens(ppTokens, &ss);
        auto context = std::make_unique<parser::Context>(cTokens, &ss);
        auto parsed = parser::parseProgram(cTokens, &ss);

        bool errors = false;
        auto program = semantics::analyse(parsed, std::move(cTokens), &ss, &errors);
        auto diagnostics = semantics::DiagnosticCollector::getDiagnostics();
        CHECK(ss.str().empty());
        CHECK(diagnostics.empty());
    }

    SECTION("G2")
    {
        auto sourcePath = getTestDataPath("g2.cor", "GENERATED_DIR");
        auto source = readFile(sourcePath);
        CHECK(!source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);
        auto ppTokens = lexer::tokenize(source, &ss);
        ppTokens = pp::preprocess(std::move(ppTokens), &ss);
        auto cTokens = lexer::toCTokens(ppTokens, &ss);
        auto context = std::make_unique<parser::Context>(cTokens, &ss);
        auto parsed = parser::parseProgram(cTokens, &ss);

        bool errors = false;
        auto program = semantics::analyse(parsed, std::move(cTokens), &ss, &errors);
        auto diagnostics = semantics::DiagnosticCollector::getDiagnostics();
        CHECK(ss.str().empty());
        CHECK(diagnostics.empty());
    }

    SECTION("G3")
    {
        auto sourcePath = getTestDataPath("g3.cor", "GENERATED_DIR");
        auto source = readFile(sourcePath);
        CHECK(!source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);
        auto ppTokens = lexer::tokenize(source, &ss);
        ppTokens = pp::preprocess(std::move(ppTokens), &ss);
        auto cTokens = lexer::toCTokens(ppTokens, &ss);
        auto context = std::make_unique<parser::Context>(cTokens, &ss);
        auto parsed = parser::parseProgram(cTokens, &ss);

        bool errors = false;
        auto program = semantics::analyse(parsed, std::move(cTokens), &ss, &errors);
        auto diagnostics = semantics::DiagnosticCollector::getDiagnostics();
        CHECK(ss.str().empty());
        CHECK(diagnostics.empty());
    }

    SECTION("G4")
    {
        auto sourcePath = getTestDataPath("g4.cor", "GENERATED_DIR");
        auto source = readFile(sourcePath);
        CHECK(!source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);
        auto ppTokens = lexer::tokenize(source, &ss);
        ppTokens = pp::preprocess(std::move(ppTokens), &ss);
        auto cTokens = lexer::toCTokens(ppTokens, &ss);
        auto context = std::make_unique<parser::Context>(cTokens, &ss);
        auto parsed = parser::parseProgram(cTokens, &ss);

        bool errors = false;
        auto program = semantics::analyse(parsed, std::move(cTokens), &ss, &errors);
        auto diagnostics = semantics::DiagnosticCollector::getDiagnostics();
        CHECK(ss.str().empty());
        CHECK(diagnostics.empty());
    }

    SECTION("G5")
    {
        auto sourcePath = getTestDataPath("g5.cor", "GENERATED_DIR");
        auto source = readFile(sourcePath);
        CHECK(!source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);
        auto ppTokens = lexer::tokenize(source, &ss);
        ppTokens = pp::preprocess(std::move(ppTokens), &ss);
        auto cTokens = lexer::toCTokens(ppTokens, &ss);
        auto context = std::make_unique<parser::Context>(cTokens, &ss);
        auto parsed = parser::parseProgram(cTokens, &ss);

        bool errors = false;
        auto program = semantics::analyse(parsed, std::move(cTokens), &ss, &errors);
        auto diagnostics = semantics::DiagnosticCollector::getDiagnostics();
        CHECK(ss.str().empty());
        CHECK(diagnostics.empty());
    }

    SECTION("G6")
    {
        auto sourcePath = getTestDataPath("g6.cor", "GENERATED_DIR");
        auto source = readFile(sourcePath);
        CHECK(!source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);
        auto ppTokens = lexer::tokenize(source, &ss);
        ppTokens = pp::preprocess(std::move(ppTokens), &ss);
        auto cTokens = lexer::toCTokens(ppTokens, &ss);
        auto context = std::make_unique<parser::Context>(cTokens, &ss);
        auto parsed = parser::parseProgram(cTokens, &ss);

        bool errors = false;
        auto program = semantics::analyse(parsed, std::move(cTokens), &ss, &errors);
        auto diagnostics = semantics::DiagnosticCollector::getDiagnostics();
        CHECK(ss.str().empty());
        CHECK(diagnostics.empty());
    }

    SECTION("G7")
    {
        auto sourcePath = getTestDataPath("g7.cor", "GENERATED_DIR");
        auto source = readFile(sourcePath);
        CHECK(!source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);
        auto ppTokens = lexer::tokenize(source, &ss);
        ppTokens = pp::preprocess(std::move(ppTokens), &ss);
        auto cTokens = lexer::toCTokens(ppTokens, &ss);
        auto context = std::make_unique<parser::Context>(cTokens, &ss);
        auto parsed = parser::parseProgram(cTokens, &ss);

        bool errors = false;
        auto program = semantics::analyse(parsed, std::move(cTokens), &ss, &errors);
        auto diagnostics = semantics::DiagnosticCollector::getDiagnostics();
        CHECK(ss.str().empty());
        CHECK(diagnostics.empty());
    }

    SECTION("G8")
    {
        auto sourcePath = getTestDataPath("g8.cor", "GENERATED_DIR");
        auto source = readFile(sourcePath);
        CHECK(!source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);
        auto ppTokens = lexer::tokenize(source, &ss);
        ppTokens = pp::preprocess(std::move(ppTokens), &ss);
        auto cTokens = lexer::toCTokens(ppTokens, &ss);
        auto context = std::make_unique<parser::Context>(cTokens, &ss);
        auto parsed = parser::parseProgram(cTokens, &ss);

        bool errors = false;
        auto program = semantics::analyse(parsed, std::move(cTokens), &ss, &errors);
        auto diagnostics = semantics::DiagnosticCollector::getDiagnostics();
        //TODO awaiting common communicator CHECK(ss.str().empty());
        //TODO awaiting common communicator CHECK(diagnostics.empty());
    }

    SECTION("G9")
    {
        auto sourcePath = getTestDataPath("g9.cor", "GENERATED_DIR");
        auto source = readFile(sourcePath);
        CHECK(!source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);
        auto ppTokens = lexer::tokenize(source, &ss);
        ppTokens = pp::preprocess(std::move(ppTokens), &ss);
        auto cTokens = lexer::toCTokens(ppTokens, &ss);
        auto context = std::make_unique<parser::Context>(cTokens, &ss);
        auto parsed = parser::parseProgram(cTokens, &ss);

        bool errors = false;
        auto program = semantics::analyse(parsed, std::move(cTokens), &ss, &errors);
        auto diagnostics = semantics::DiagnosticCollector::getDiagnostics();
        CHECK(ss.str().empty());
        CHECK(diagnostics.empty());
    }

    SECTION("G10")
    {
        auto sourcePath = getTestDataPath("g10.cor", "GENERATED_DIR");
        auto source = readFile(sourcePath);
        CHECK(!source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);
        auto ppTokens = lexer::tokenize(source, &ss);
        ppTokens = pp::preprocess(std::move(ppTokens), &ss);
        auto cTokens = lexer::toCTokens(ppTokens, &ss);
        auto context = std::make_unique<parser::Context>(cTokens, &ss);
        auto parsed = parser::parseProgram(cTokens, &ss);

        bool errors = false;
        auto program = semantics::analyse(parsed, std::move(cTokens), &ss, &errors);
        auto diagnostics = semantics::DiagnosticCollector::getDiagnostics();
        CHECK(ss.str().empty());
        CHECK(diagnostics.empty());
    }
}
