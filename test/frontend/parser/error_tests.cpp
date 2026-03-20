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
        CHECK(parser::DiagnosticCollector::getDiagnostics().size() == 1);
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_CONDITION));
    }

    SECTION("Expected THEN")
    {
        const ParsedExpr p("IF a < b ELSE");
        CHECK(p.expr.get() == nullptr);
        CHECK(parser::DiagnosticCollector::getDiagnostics().size() == 1);
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_THEN));
    }

    SECTION("Expected Statement After THEN")
    {
        const ParsedExpr p("IF a < b THEN ELSE");
        CHECK(p.expr.get() == nullptr);
        CHECK(parser::DiagnosticCollector::getDiagnostics().size() == 1);
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_STATEMENT));
    }

    SECTION("Expected ELSE")
    {
        const ParsedExpr p("IF a < b THEN d IF");
        CHECK(p.expr.get() == nullptr);
        CHECK(parser::DiagnosticCollector::getDiagnostics().size() == 1);
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_ELSE));
    }

    SECTION("Expected Statement After ELSE")
    {
        const ParsedExpr p("IF a < b THEN d ELSE IF;");
        CHECK(p.expr.get() == nullptr);
        CHECK(parser::DiagnosticCollector::getDiagnostics().size() == 2);
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_STATEMENT));
    }
}
