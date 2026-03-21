/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 07/03/2026
 */

#include "common.hpp"
#include "lib/frontend/syntax/syntax.hpp"
#include "llvm/ADT/APSInt.h"
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
        CHECK(std::get<llvm::APSInt>(assignmentStatement->getExpression().begin()->getValue()) == 2);
    }

    SECTION("Expression")
    {
        const ParsedStatement s("A := IF 2 > 1 THEN 3 ELSE 4;");
        auto assignmentStatement = dynamic_cast<AssignmentStatement*>(s.statement.get());
        REQUIRE(assignmentStatement != nullptr);
        auto condExpr = dynamic_cast<const ConditionalExpression*>(&assignmentStatement->getExpression());
        REQUIRE(condExpr != nullptr);

        auto thenBranch = dynamic_cast<const LiteralExpression*>(&condExpr->getThenBranch());
        REQUIRE(thenBranch != nullptr);
        CHECK(std::get<llvm::APSInt>(thenBranch->getLiteral()->getValue()) == 3);

        auto elseBranch = dynamic_cast<const LiteralExpression*>(&condExpr->getElseBranch());
        REQUIRE(elseBranch != nullptr);
        CHECK(std::get<llvm::APSInt>(elseBranch->getLiteral()->getValue()) == 4);
    }

    SECTION("Location")
    {
        const ParsedStatement s("[LOCATION(i) + 1] := 3.8;");
        auto assignmentStatement = dynamic_cast<AssignmentStatement*>(s.statement.get());
        REQUIRE(assignmentStatement != nullptr);
    }

    SECTION("Bits")
    {
        const ParsedStatement s("BITS[2,6]x := 3;");
        auto assignmentStatement = dynamic_cast<AssignmentStatement*>(s.statement.get());
        REQUIRE(assignmentStatement != nullptr);
    }

    SECTION("Location And Bits")
    {
        const ParsedStatement s("BITS[1,23] [LOCATION(i) + 1] := 1;");
        auto assignmentStatement = dynamic_cast<AssignmentStatement*>(s.statement.get());
        REQUIRE(assignmentStatement != nullptr);
    }
}
