/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 21/04/2026
 */


#include "common.hpp"
#include "test/common/test_config.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"
#include "tools/parser/astdump/lib/ast_dump.hpp"

using namespace coral66;
using namespace coral66::Syntax;
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
        REQUIRE(!source.empty());

        std::string reporterString;
        llvm::raw_string_ostream ss(reporterString);
        auto ppTokens = lexer::tokenize(source, &ss);
        auto cTokens = lexer::toCTokens(ppTokens, &ss);
        auto context = std::make_unique<parser::Context>(cTokens, &ss);
        parser::DiagnosticCollector::clearDiagnostics();
        auto parsed = parser::parseProgram(cTokens, &ss);

        auto sourceAst = tools::AstDump::dumpToString(parsed);
        auto astPath = getTestDataPath("golden/ackermann/ackerman_ast.json", "frontend/parser");
        auto ast = readFile(astPath);
        REQUIRE(!ast.empty());

        CHECK(sourceAst == ast);
    }

    SECTION("Sieve") {}
}

TEST_CASE("Generated", "[Parser][Golden]")
{
    SECTION("G1") {}

    SECTION("G2") {}

    SECTION("G3") {}

    SECTION("G4") {}

    SECTION("G5") {}
}