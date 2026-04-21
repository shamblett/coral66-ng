/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 21/04/2026
 */

#include "common.hpp"
#include "lib/frontend/syntax/syntax.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66;
using namespace coral66::Syntax;

/*
 * Golden tests
 */

TEST_CASE("Examples", "[Parser][Golden]")
{
    SECTION("Ackermann") {}

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