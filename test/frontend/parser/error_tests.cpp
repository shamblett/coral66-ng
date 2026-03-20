/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 18/03/2026
 */

#include "common.hpp"
#include "lib/frontend/common/messages/errors.hpp"
#include "lib/frontend/common/source_object.hpp"
#include "lib/frontend/parser/parser_support.hpp"
#include "lib/frontend/preprocessor/preprocessor.hpp"
#include "test/common/test_config.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

/*
 *
 * Error diagnostics tests
 *
 */

using namespace coral66::errors::parser;

TEST_CASE("Expressions - Errors", "[Parser][Expression]")
{
    SECTION("IF Expected Condition")
    {
        const ParsedExpr p("IF THEN");
        CHECK(p.expr.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_CONDITION));
    }

    SECTION("Expected THEN")
    {
        const ParsedExpr p("IF a < b ELSE");
        CHECK(p.expr.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_THEN));
    }

    SECTION("Expected Statement After THEN")
    {
        const ParsedExpr p("IF a < b THEN ELSE");
        CHECK(p.expr.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_STATEMENT));
    }

    SECTION("Expected ELSE")
    {
        const ParsedExpr p("IF a < b THEN d IF");
        CHECK(p.expr.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_ELSE));
    }

    SECTION("Expected Statement After ELSE")
    {
        const ParsedExpr p("IF a < b THEN d ELSE IF;");
        CHECK(p.expr.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_STATEMENT));
    }

    SECTION("Expected Expression (Condition OR)")
    {
        const ParsedExpr p("IF a = b OR = THEN c ELSE d");
        CHECK(p.expr.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_EXPRESSION));
    }

    SECTION("Expected Comparator (Comparison)")
    {
        const ParsedExpr p("IF a THEN b ELSE c");
        CHECK(p.expr.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_COMPARATOR));
    }

    SECTION("Expected Simple Expression (Comparison Missing RHS)")
    {
        const ParsedExpr p("IF a = THEN c ELSE d");
        CHECK(p.expr.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_SIMPLE_EXPRESSION));
    }

    SECTION("Expected Term (Simple Expression)")
    {
        const ParsedExpr p("a + =");
        CHECK(p.expr.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_TERM));
    }

    SECTION("Expected Factor (Term)")
    {
        const ParsedExpr p("a * =");
        CHECK(p.expr.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_FACTOR));
    }

    SECTION("Expected Closed Parenthesis")
    {
        const ParsedExpr p("( a = ");
        CHECK(p.expr.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_CLOSED_PARENTHESIS));
    }

    SECTION("Expected Union (Bitwise Differ)")
    {
        const ParsedExpr p("a DIFFER =");
        CHECK(p.expr.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_UNION));
    }

    SECTION("Expected Mask (Bitwise Union)")
    {
        const ParsedExpr p("a UNION =");
        CHECK(p.expr.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_MASK));
    }

    SECTION("Expected Typed Primary (Bitwise Mask)")
    {
        const ParsedExpr p("a MASK =");
        CHECK(p.expr.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_TYPED_PRIMARY));
    }

    SECTION("Expected Word Reference (Typed Primary)")
    {
        const ParsedExpr p("LOCATION ( = )");
        CHECK(p.expr.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_WORD_REFERENCE));
    }

    SECTION("Expected Fraction Bits (Typed Primary)")
    {
        const ParsedExpr p("FIXED (1, ) (a)");
        CHECK(p.expr.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_FRACTION_BITS));
    }

    SECTION("Expected Open Square Bracket (Typed Primary/Word Reference)")
    {
        const ParsedExpr p("BITS = ");
        CHECK(p.expr.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_OPEN_SQUARE_BRACKET));
    }

    SECTION("Expected Identifier (Word Reference)")
    {
        const ParsedExpr p("LOCATION ( = )");
        CHECK(p.expr.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_IDENTIFIER));
    }

    SECTION("Expected Open Parenthesis (Typed Primary)")
    {
        const ParsedExpr p("FIXED (1, 2) =");
        CHECK(p.expr.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_OPEN_PARENTHESIS));
    }

    SECTION("Expected Open Parenthesis (Typed Primary)")
    {
        const ParsedExpr p("FIXED (1, 2) =");
        CHECK(p.expr.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_OPEN_PARENTHESIS));
    }

    SECTION("Graceful Failure Without Crashing on Premature EOF")
    {
        // Testing specific end-of-stream edge cases without trailing fallback tokens
        // to ensure bounds checking correctly catches 'begin == end' instead of aborting.
        const char* strings[] = {"a + ",    "a * ",          "a DIFFER ",    "a UNION ",    "a MASK ",
                                 "IF a = ", "IF a = b AND ", "IF a = b OR ", "LOCATION ( ", "FIXED (1 "};

        for (auto src : strings)
        {
            const ParsedExpr p(src);
            CHECK(p.expr.get() == nullptr);
            CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        }
    }
}
