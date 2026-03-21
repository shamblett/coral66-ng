/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 07/03/2026
 */

#include "common.hpp"
#include "lib/frontend/syntax/syntax.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66;
using namespace coral66::Syntax;

/*
 * Tests cover section 7 of the specification
 */

/*
 * 7.1 Assignment
 */
TEST_CASE("Coral Parse Statements - Assignment", "[Parser][Statement]")
{
    SECTION("Simple")
    {
        const ParsedStatement s("A := 2;");
        auto assignmentStatement = dynamic_cast<AssignmentStatement*>(s.statement.get());
        REQUIRE(assignmentStatement != nullptr);
    }
}
