/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 21/04/2026
 */

#include <iostream>

#include "common.hpp"
#include "lib/frontend/preprocessor/preprocessor.hpp"
#include "test/common/test_config.hpp"
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
        auto sourcePath = getTestDataPath("golden/ackermann/ackerman.cor", "frontend/parser");
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
        auto astPath = getTestDataPath("golden/ackermann/ackerman_ast.json", "frontend/parser");
        auto ast = readFile(astPath);
        CHECK(!ast.empty());

        CHECK(sourceAst == ast);
    }

    SECTION("Sieve")
    {
        auto sourcePath = getTestDataPath("golden/sieve/sieve.cor", "frontend/parser");
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
        auto astPath = getTestDataPath("golden/sieve/sieve_ast.json", "frontend/parser");
        auto ast = readFile(astPath);
        CHECK(!ast.empty());

        CHECK(sourceAst == ast);
    }
}

TEST_CASE("Generated", "[Parser][Golden]")
{
    SECTION("G1")
    {
        auto sourcePath = getTestDataPath("golden/generated/g1.cor", "frontend/parser");
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
        auto astPath = getTestDataPath("golden/generated/g1_ast.json", "frontend/parser");
        auto ast = readFile(astPath);
        CHECK(!ast.empty());

        CHECK(sourceAst == ast);
    }

    SECTION("G2")
    {
        auto sourcePath = getTestDataPath("golden/generated/g2.cor", "frontend/parser");
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
        auto astPath = getTestDataPath("golden/generated/g2_ast.json", "frontend/parser");
        auto ast = readFile(astPath);
        CHECK(!ast.empty());

        CHECK(sourceAst == ast);
    }

    SECTION("G3")
    {
        auto sourcePath = getTestDataPath("golden/generated/g3.cor", "frontend/parser");
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
        auto astPath = getTestDataPath("golden/generated/g3_ast.json", "frontend/parser");
        auto ast = readFile(astPath);
        CHECK(!ast.empty());

        CHECK(sourceAst == ast);
    }

    SECTION("G4")
    {
        auto sourcePath = getTestDataPath("golden/generated/g4.cor", "frontend/parser");
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
        auto astPath = getTestDataPath("golden/generated/g4_ast.json", "frontend/parser");
        auto ast = readFile(astPath);
        CHECK(!ast.empty());

        CHECK(sourceAst == ast);
    }

    SECTION("G5")
    {
        auto sourcePath = getTestDataPath("golden/generated/g5.cor", "frontend/parser");
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
        auto astPath = getTestDataPath("golden/generated/g5_ast.json", "frontend/parser");
        auto ast = readFile(astPath);
        CHECK(!ast.empty());

        CHECK(sourceAst == ast);
    }
}