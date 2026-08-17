/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 21/04/2026
 */

#include <iostream>

#include "lib/frontend/preprocessor/preprocessor.hpp"
#include "test/common/test_config.hpp"
#include "test/frontend/common/common.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"
#include "tools/parser/astdump/lib/ast_dump.hpp"

using namespace coral66;
using namespace coral66::syntax;
using namespace coral66::Tests;

/*
 * Golden tests
 */

TEST_CASE("Examples", "[Parser][Golden]")
{
    SECTION("Ackermann")
    {
        auto sourcePath = getTestDataPath("ackermann/ackerman.cor", "GOLDEN_DIR");
        auto source = readFile(sourcePath);
        CHECK(!source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);
        auto ppTokens = lexer::tokenize(source, &ss);
        ppTokens = pp::preprocess(std::move(ppTokens), &ss);
        auto cTokens = lexer::toCTokens(ppTokens, &ss);
        auto context = std::make_unique<parser::Context>(cTokens, &ss);
        parser::DiagnosticCollector::clearDiagnostics();
        auto parsed = parser::parseProgram(cTokens, &ss);

        auto sourceAst = tools::AstDump::dumpToString(parsed);
        auto astPath = getTestDataPath("ackermann/ackerman_ast.json", "GOLDEN_DIR");
        auto ast = readFile(astPath);
        CHECK(!ast.empty());

        CHECK(sourceAst == ast);
    }

    SECTION("Sieve")
    {
        auto sourcePath = getTestDataPath("sieve/sieve.cor", "GOLDEN_DIR");
        auto source = readFile(sourcePath);
        CHECK(!source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);
        auto ppTokens = lexer::tokenize(source, &ss);
        ppTokens = pp::preprocess(std::move(ppTokens), &ss);
        auto cTokens = lexer::toCTokens(ppTokens, &ss);
        auto context = std::make_unique<parser::Context>(cTokens, &ss);
        parser::DiagnosticCollector::clearDiagnostics();
        auto parsed = parser::parseProgram(cTokens, &ss);

        auto sourceAst = tools::AstDump::dumpToString(parsed);
        auto astPath = getTestDataPath("sieve/sieve_ast.json", "GOLDEN_DIR");
        auto ast = readFile(astPath);
        CHECK(!ast.empty());

        CHECK(sourceAst == ast);
    }

    SECTION("Whetstone")
    {
        auto sourcePath = getTestDataPath("whetstone/whetstone.cor", "GOLDEN_DIR");
        auto source = readFile(sourcePath);
        CHECK(!source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);
        auto ppTokens = lexer::tokenize(source, &ss);
        ppTokens = pp::preprocess(std::move(ppTokens), &ss);
        auto cTokens = lexer::toCTokens(ppTokens, &ss);
        auto context = std::make_unique<parser::Context>(cTokens, &ss);
        parser::DiagnosticCollector::clearDiagnostics();
        auto parsed = parser::parseProgram(cTokens, &ss);

        auto sourceAst = tools::AstDump::dumpToString(parsed);
        auto astPath = getTestDataPath("whetstone/whetstone_ast.json", "GOLDEN_DIR");
        auto ast = readFile(astPath);
        CHECK(!ast.empty());

        CHECK(sourceAst == ast);
    }
}

TEST_CASE("Generated", "[Parser][Golden]")
{
    SECTION("G1")
    {
        auto sourcePath = getTestDataPath("generated/g1.cor", "GOLDEN_DIR");
        auto source = readFile(sourcePath);
        CHECK(!source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);
        auto ppTokens = lexer::tokenize(source, &ss);
        ppTokens = pp::preprocess(std::move(ppTokens), &ss);
        auto cTokens = lexer::toCTokens(ppTokens, &ss);
        auto context = std::make_unique<parser::Context>(cTokens, &ss);
        parser::DiagnosticCollector::clearDiagnostics();
        auto parsed = parser::parseProgram(cTokens, &ss);

        auto sourceAst = tools::AstDump::dumpToString(parsed);
        auto astPath = getTestDataPath("generated/g1_ast.json", "GOLDEN_DIR");
        auto ast = readFile(astPath);
        CHECK(!ast.empty());

        CHECK(sourceAst == ast);
    }

    SECTION("G2")
    {
        auto sourcePath = getTestDataPath("generated/g2.cor", "GOLDEN_DIR");
        auto source = readFile(sourcePath);
        CHECK(!source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);
        auto ppTokens = lexer::tokenize(source, &ss);
        ppTokens = pp::preprocess(std::move(ppTokens), &ss);
        auto cTokens = lexer::toCTokens(ppTokens, &ss);
        auto context = std::make_unique<parser::Context>(cTokens, &ss);
        parser::DiagnosticCollector::clearDiagnostics();
        auto parsed = parser::parseProgram(cTokens, &ss);

        auto sourceAst = tools::AstDump::dumpToString(parsed);
        auto astPath = getTestDataPath("generated/g2_ast.json", "GOLDEN_DIR");
        auto ast = readFile(astPath);
        CHECK(!ast.empty());

        CHECK(sourceAst == ast);
    }

    SECTION("G3")
    {
        auto sourcePath = getTestDataPath("generated/g3.cor", "GOLDEN_DIR");
        auto source = readFile(sourcePath);
        CHECK(!source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);
        auto ppTokens = lexer::tokenize(source, &ss);
        ppTokens = pp::preprocess(std::move(ppTokens), &ss);
        auto cTokens = lexer::toCTokens(ppTokens, &ss);
        auto context = std::make_unique<parser::Context>(cTokens, &ss);
        parser::DiagnosticCollector::clearDiagnostics();
        auto parsed = parser::parseProgram(cTokens, &ss);

        auto sourceAst = tools::AstDump::dumpToString(parsed);
        auto astPath = getTestDataPath("generated/g3_ast.json", "GOLDEN_DIR");
        auto ast = readFile(astPath);
        CHECK(!ast.empty());

        CHECK(sourceAst == ast);
    }

    SECTION("G4")
    {
        auto sourcePath = getTestDataPath("generated/g4.cor", "GOLDEN_DIR");
        auto source = readFile(sourcePath);
        CHECK(!source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);
        auto ppTokens = lexer::tokenize(source, &ss);
        ppTokens = pp::preprocess(std::move(ppTokens), &ss);
        auto cTokens = lexer::toCTokens(ppTokens, &ss);
        auto context = std::make_unique<parser::Context>(cTokens, &ss);
        parser::DiagnosticCollector::clearDiagnostics();
        auto parsed = parser::parseProgram(cTokens, &ss);

        auto sourceAst = tools::AstDump::dumpToString(parsed);
        auto astPath = getTestDataPath("generated/g4_ast.json", "GOLDEN_DIR");
        auto ast = readFile(astPath);
        CHECK(!ast.empty());

        CHECK(sourceAst == ast);
    }

    SECTION("G5")
    {
        auto sourcePath = getTestDataPath("generated/g5.cor", "GOLDEN_DIR");
        auto source = readFile(sourcePath);
        CHECK(!source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);
        auto ppTokens = lexer::tokenize(source, &ss);
        ppTokens = pp::preprocess(std::move(ppTokens), &ss);
        auto cTokens = lexer::toCTokens(ppTokens, &ss);
        auto context = std::make_unique<parser::Context>(cTokens, &ss);
        parser::DiagnosticCollector::clearDiagnostics();
        auto parsed = parser::parseProgram(cTokens, &ss);

        auto sourceAst = tools::AstDump::dumpToString(parsed);
        auto astPath = getTestDataPath("generated/g5_ast.json", "GOLDEN_DIR");
        auto ast = readFile(astPath);
        CHECK(!ast.empty());

        CHECK(sourceAst == ast);
    }

    SECTION("G6")
    {
        auto sourcePath = getTestDataPath("generated/g6.cor", "GOLDEN_DIR");
        auto source = readFile(sourcePath);
        CHECK(!source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);
        auto ppTokens = lexer::tokenize(source, &ss);
        ppTokens = pp::preprocess(std::move(ppTokens), &ss);
        auto cTokens = lexer::toCTokens(ppTokens, &ss);
        auto context = std::make_unique<parser::Context>(cTokens, &ss);
        parser::DiagnosticCollector::clearDiagnostics();
        auto parsed = parser::parseProgram(cTokens, &ss);

        auto sourceAst = tools::AstDump::dumpToString(parsed);
        auto astPath = getTestDataPath("generated/g6_ast.json", "GOLDEN_DIR");
        auto ast = readFile(astPath);
        CHECK(!ast.empty());

        CHECK(sourceAst == ast);
    }

    SECTION("G7")
    {
        auto sourcePath = getTestDataPath("generated/g7.cor", "GOLDEN_DIR");
        auto source = readFile(sourcePath);
        CHECK(!source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);
        auto ppTokens = lexer::tokenize(source, &ss);
        ppTokens = pp::preprocess(std::move(ppTokens), &ss);
        auto cTokens = lexer::toCTokens(ppTokens, &ss);
        auto context = std::make_unique<parser::Context>(cTokens, &ss);
        parser::DiagnosticCollector::clearDiagnostics();
        auto parsed = parser::parseProgram(cTokens, &ss);

        auto sourceAst = tools::AstDump::dumpToString(parsed);
        auto astPath = getTestDataPath("generated/g67ast.json", "GOLDEN_DIR");
        auto ast = readFile(astPath);
        CHECK(!ast.empty());

        CHECK(sourceAst == ast);
    }
}