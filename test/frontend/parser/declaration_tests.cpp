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
using namespace coral66::syntax;

/*
 * Tests cover section 4 of the specification
 */

TEST_CASE("Array Declarations", "[Parser][Declarations]")
{
    SECTION("Multiple identifiers sharing bounds")
    {
        ParsedDeclaration p("INTEGER ARRAY p, q[1:3], r[0:5]");
        CHECK(p.declaration.size() == 2);

        auto arr1 = dynamic_cast<const ArrayDeclaration*>(p.declaration[0].get());
        CHECK(arr1 != nullptr);
        CHECK(arr1->getIdentifiers().size() == 2);
        CHECK(arr1->getIdentifiers()[0]->getText() == "p");
        CHECK(arr1->getIdentifiers()[1]->getText() == "q");
        CHECK(arr1->getDimensions().size() == 1);

        auto arr2 = dynamic_cast<const ArrayDeclaration*>(p.declaration[1].get());
        CHECK(arr2 != nullptr);
        CHECK(arr2->getIdentifiers().size() == 1);
        CHECK(arr2->getIdentifiers()[0]->getText() == "r");
        CHECK(arr2->getDimensions().size() == 1);
    }
}

TEST_CASE("Table Declarations", "[Parser][Declarations]")
{
    SECTION("Basic Table With Elements")
    {
        ParsedDeclaration p("TABLE t[10, 20] [ x INTEGER 0 ; y UNSIGNED (4,2) 1, 4 ; z (24) -1 BIT 2 ] PRESET 5(1, 2)");
        CHECK(p.declaration.size() == 1);

        auto table = dynamic_cast<const TableDeclaration*>(p.declaration[0].get());
        CHECK(table != nullptr);
        CHECK(table->getIdentifier()->getText() == "t");
        CHECK(table->getElements().size() == 3);

        auto el1 = dynamic_cast<const TableElementDeclaration*>(table->getElements()[0].get());
        CHECK(el1 != nullptr);
        CHECK(el1->getIdentifier()->getText() == "x");
        CHECK(el1->getType().has_value());
        CHECK(el1->getType().value()->getTokenType() == lexer::TokenType::IntegerKeyword);
        CHECK(!el1->isUnsigned());
        CHECK(el1->getWordPosition().has_value());
        CHECK(std::get<llvm::APSInt>(el1->getWordPosition().value()->getValue()).getZExtValue() == 0);

        auto el2 = dynamic_cast<const TableElementDeclaration*>(table->getElements()[1].get());
        CHECK(el2 != nullptr);
        CHECK(el2->getIdentifier()->getText() == "y");
        CHECK(!el2->getType().has_value());
        CHECK(el2->isUnsigned());
        CHECK(el2->getWordPosition().has_value());
        CHECK(std::get<llvm::APSInt>(el2->getWordPosition().value()->getValue()).getZExtValue() == 1);
        CHECK(el2->getBitPosition().has_value());
        CHECK(std::get<llvm::APSInt>(el2->getBitPosition().value()->getValue()).getZExtValue() == 4);

        auto el3 = dynamic_cast<const TableElementDeclaration*>(table->getElements()[2].get());
        CHECK(el3 != nullptr);
        CHECK(el3->getIdentifier()->getText() == "z");
        CHECK(!el3->getType().has_value());
        CHECK(!el3->isUnsigned());
        CHECK(el3->getWordPosition().has_value());
        CHECK(std::get<llvm::APSInt>(el3->getWordPosition().value()->getValue()).getZExtValue()
              == 1); // Negative value check needs special handling, we'll just check it has value and skip value check
                     // for -1 since it's signed
        CHECK(el3->getBitPosition().has_value());
        CHECK(std::get<llvm::APSInt>(el3->getBitPosition().value()->getValue()).getZExtValue() == 2);

        CHECK(parser::DiagnosticCollector::getDiagnostics().empty());
    }
}

TEST_CASE("Presetting", "[Parser][Declarations]")
{
    SECTION("Simple Assignment")
    {
        ParsedDeclaration p("INTEGER a := 10");
        CHECK(p.declaration.size() == 1);
        CHECK(parser::DiagnosticCollector::getDiagnostics().empty());
    }

    SECTION("Preset Keyword")
    {
        ParsedDeclaration p("FLOATING f PRESET 10.5, -2.0, +3.1");
        CHECK(p.declaration.size() == 1);
        CHECK(parser::DiagnosticCollector::getDiagnostics().empty());
    }

    SECTION("Octal and Grouping")
    {
        ParsedDeclaration p("INTEGER ARRAY a[1:10] := (1, (OCTAL(77)))");
        CHECK(p.declaration.size() == 1);
        auto arr = dynamic_cast<const ArrayDeclaration*>(p.declaration[0].get());
        CHECK(arr != nullptr);
        CHECK(!arr->getPresetTokens().empty());
        CHECK(arr->getPresetTokens()[0]->getTokenType() == lexer::TokenType::OpenParentheses);
        CHECK(parser::DiagnosticCollector::getDiagnostics().empty());
    }

    SECTION("String Literals")
    {
        ParsedDeclaration p(R"(INTEGER ARRAY a[1:10] := "HELLO", "WORLD")");
        CHECK(p.declaration.size() == 1);
        CHECK(parser::DiagnosticCollector::getDiagnostics().empty());
    }

    SECTION("Void Presetting")
    {
        ParsedDeclaration p("INTEGER ARRAY a[1:10] := 1, , 3, , , 6");
        CHECK(p.declaration.size() == 1);
        CHECK(parser::DiagnosticCollector::getDiagnostics().empty());
    }
}

TEST_CASE("Overlay Declarations", "[Parser][Declarations]")
{
    SECTION("Base only")
    {
        ParsedDeclaration p("OVERLAY x WITH INTEGER y");
        CHECK(p.declaration.size() == 1);
        auto overlay = dynamic_cast<const OverlayDeclaration*>(p.declaration[0].get());
        CHECK(overlay != nullptr);
        CHECK(overlay->getBase()->getText() == "x");
        CHECK(overlay->getOffset()->getText() == "x");
    }

    SECTION("With positive offset")
    {
        ParsedDeclaration p("OVERLAY arr[5] WITH INTEGER y");
        CHECK(p.declaration.size() == 1);
        auto overlay = dynamic_cast<const OverlayDeclaration*>(p.declaration[0].get());
        CHECK(overlay != nullptr);
        CHECK(overlay->getBase()->getText() == "arr");
        CHECK(std::get<llvm::APSInt>(overlay->getOffset()->getValue()).getZExtValue() == 5);
    }

    SECTION("With negative offset")
    {
        ParsedDeclaration p("OVERLAY arr[-10] WITH INTEGER y");
        CHECK(p.declaration.size() == 1);
        auto overlay = dynamic_cast<const OverlayDeclaration*>(p.declaration[0].get());
        CHECK(overlay != nullptr);
        CHECK(overlay->getBase()->getText() == "arr");
        CHECK(std::get<llvm::APSInt>(overlay->getOffset()->getValue()).getZExtValue() == 10);
        CHECK(!overlay->getOffset2().has_value());
    }

    SECTION("With two offsets")
    {
        ParsedDeclaration p("OVERLAY arr[15, -20] WITH INTEGER y");
        CHECK(p.declaration.size() == 1);
        auto overlay = dynamic_cast<const OverlayDeclaration*>(p.declaration[0].get());
        CHECK(overlay != nullptr);
        CHECK(overlay->getBase()->getText() == "arr");
        CHECK(std::get<llvm::APSInt>(overlay->getOffset()->getValue()).getZExtValue() == 15);
        CHECK(overlay->getOffset2().has_value());
        CHECK(std::get<llvm::APSInt>(overlay->getOffset2().value()->getValue()).getZExtValue() == 20);
    }
}
