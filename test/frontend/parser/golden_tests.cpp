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
        auto sourcePath = getTestDataPath("parser", "ackermann/ackermann.cor");
        auto source = readFile(sourcePath);
        REQUIRE(!source.empty());
        auto parsed = parseSourceFile(source);
        auto sourceAst = tools::AstDump::dumpToString(parsed);
        auto astPath = getTestDataPath("parser", "ackermann/ackermann_ast.json");
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