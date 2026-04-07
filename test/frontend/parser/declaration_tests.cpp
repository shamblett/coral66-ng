/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 23/03/2026
 */

#include "common.hpp"
#include "lib/frontend/common/messages/errors.hpp"
#include "lib/frontend/syntax/syntax.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66;
using namespace coral66::Syntax;

/*
 * Tests cover section 4 of the specification
 */

TEST_CASE("Array Declarations", "[Parser][Declarations]")
{
    SECTION("Multiple identifiers sharing bounds")
    {
        ParsedDeclaration p("INTEGER ARRAY p, q[1:3], r[0:5]");
        REQUIRE(p.declaration.size() == 2);

        auto arr1 = dynamic_cast<const ArrayDeclaration*>(p.declaration[0].get());
        REQUIRE(arr1 != nullptr);
        REQUIRE(arr1->getIdentifiers().size() == 2);
        CHECK(arr1->getIdentifiers()[0]->getText() == "p");
        CHECK(arr1->getIdentifiers()[1]->getText() == "q");
        REQUIRE(arr1->getDimensions().size() == 1);

        auto arr2 = dynamic_cast<const ArrayDeclaration*>(p.declaration[1].get());
        REQUIRE(arr2 != nullptr);
        REQUIRE(arr2->getIdentifiers().size() == 1);
        CHECK(arr2->getIdentifiers()[0]->getText() == "r");
        REQUIRE(arr2->getDimensions().size() == 1);
    }

    SECTION("Error: Missing Identifier")
    {
        ParsedDeclaration p("INTEGER ARRAY [1:2]");
        REQUIRE(p.declaration.size() == 0);
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_IDENTIFIER));
    }

    SECTION("Error: Missing Colon")
    {
        ParsedDeclaration p("INTEGER ARRAY a[1]");
        REQUIRE(p.declaration.size() == 1);
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_COLON));
    }

    SECTION("Error: Missing Close Bracket")
    {
        ParsedDeclaration p("INTEGER ARRAY a[1:2");
        REQUIRE(p.declaration.size() == 1);
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_CLOSE_SQUARE_BRACKET));
    }
}

TEST_CASE("Table Declarations", "[Parser][Declarations]")
{
    SECTION("Basic Table With Elements")
    {
        ParsedDeclaration p(
            "TABLE t[10, 20] [ x INTEGER 0 ; y UNSIGNED (4,2) 1, 4 ; z FLOATING -1 BIT 2 ] PRESET 5(1, 2)");
        REQUIRE(p.declaration.size() == 1);

        auto table = dynamic_cast<const TableDeclaration*>(p.declaration[0].get());
        REQUIRE(table != nullptr);
        REQUIRE(table->getIdentifier()->getText() == "t");
        REQUIRE(table->getElements().size() == 3);
        CHECK(parser::DiagnosticCollector::getDiagnostics().empty());
    }
    SECTION("Error: Missing Element Type")
    {
        ParsedDeclaration p("TABLE t[10] [ x 0 ]");
        REQUIRE(p.declaration.size() == 1);
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_TYPE));
    }

    SECTION("Error: Missing Element Identifier")
    {
        ParsedDeclaration p("TABLE t[10] [ INTEGER 0 ]");
        REQUIRE(p.declaration.size() == 1);
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_IDENTIFIER));
    }
}

TEST_CASE("Presetting", "[Parser][Declarations]")
{
    SECTION("Simple Assignment")
    {
        ParsedDeclaration p("INTEGER a := 10");
        REQUIRE(p.declaration.size() == 1);
        CHECK(parser::DiagnosticCollector::getDiagnostics().empty());
    }

    SECTION("Preset Keyword")
    {
        ParsedDeclaration p("FLOATING f PRESET 10.5, -2.0, +3.1");
        REQUIRE(p.declaration.size() == 1);
        CHECK(parser::DiagnosticCollector::getDiagnostics().empty());
    }

    SECTION("Octal and Grouping")
    {
        ParsedDeclaration p("INTEGER ARRAY a[1:10] := 5(1, 2(OCTAL(77)))");
        REQUIRE(p.declaration.size() == 1);
        CHECK(parser::DiagnosticCollector::getDiagnostics().empty());
    }

    SECTION("String Literals")
    {
        ParsedDeclaration p("INTEGER ARRAY a[1:10] := \"HELLO\", \"WORLD\"");
        REQUIRE(p.declaration.size() == 1);
        CHECK(parser::DiagnosticCollector::getDiagnostics().empty());
    }

    SECTION("Error: Missing Expression")
    {
        ParsedDeclaration p("INTEGER a :=");
        REQUIRE(p.declaration.size() == 1);
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_EXPRESSION));
    }
}

TEST_CASE("Overlay Declarations", "[Parser][Declarations]")
{
    SECTION("Base only")
    {
        ParsedDeclaration p("OVERLAY x WITH INTEGER y");
        REQUIRE(p.declaration.size() == 1);
        auto overlay = dynamic_cast<const OverlayDeclaration*>(p.declaration[0].get());
        REQUIRE(overlay != nullptr);
        CHECK(overlay->getBase()->getText() == "x");
        CHECK(overlay->getOffset()->getText() == "x");
    }

    SECTION("With positive offset")
    {
        ParsedDeclaration p("OVERLAY arr[5] WITH INTEGER y");
        REQUIRE(p.declaration.size() == 1);
        auto overlay = dynamic_cast<const OverlayDeclaration*>(p.declaration[0].get());
        REQUIRE(overlay != nullptr);
        CHECK(overlay->getBase()->getText() == "arr");
        CHECK(std::get<llvm::APSInt>(overlay->getOffset()->getValue()).getZExtValue() == 5);
    }

    SECTION("With negative offset")
    {
        ParsedDeclaration p("OVERLAY arr[-10] WITH INTEGER y");
        REQUIRE(p.declaration.size() == 1);
        auto overlay = dynamic_cast<const OverlayDeclaration*>(p.declaration[0].get());
        REQUIRE(overlay != nullptr);
        CHECK(overlay->getBase()->getText() == "arr");
        CHECK(std::get<llvm::APSInt>(overlay->getOffset()->getValue()).getZExtValue() == 10);
    }

    SECTION("Error: Expected Declaration")
    {
        ParsedDeclaration p("OVERLAY x WITH");
        REQUIRE(p.declaration.size() == 0);
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_DECLARATION));
    }
}
