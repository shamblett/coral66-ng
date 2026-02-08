/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "lib/frontend/semantics/semantics.hpp"

#include "lib/frontend/parser/parser.hpp"
#include "lib/frontend/common/error_messages.hpp"
#include "lib/frontend/common/source_object.hpp"
#include "test/common/test_config.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

#define semanticsProduces(source, matches)                                   \
    do                                                                       \
    {                                                                        \
        std::string string;                                                  \
        llvm::raw_string_ostream ss(string);                                 \
        coral66::PPSourceObject tokens;                                      \
        auto options = coral66::LanguageOptions::native();                   \
        bool errors = false;                                                 \
        tokens = coral66::Lexer::tokenize(source, &options, &ss, &errors);   \
        REQUIRE_FALSE(errors);                                               \
        auto ctokens = coral66::Lexer::toCTokens(tokens, &ss, &errors);      \
        REQUIRE_FALSE(errors);                                               \
        auto tree = coral66::Parser::parseProgram(ctokens, &ss, &errors);    \
        REQUIRE_FALSE(errors);                                               \
        coral66::Semantics::analyse(tree, std::move(ctokens), &ss, &errors); \
        CHECK_THAT(string, matches);                                         \
    } while (0)

using namespace coral66::Errors::Semantics;

TEST_CASE("Coral Semantic Analysis - Declarations", "[semantics]")
{
    // Valid declarations should produce no errors
    semanticsProduces("PROGRAM BEGIN INTEGER a; END", Catch::Matchers::Equals(""));
    semanticsProduces("PROGRAM BEGIN FLOATING f; END", Catch::Matchers::Equals(""));
    semanticsProduces("PROGRAM BEGIN FIXED(16,8) x; END", Catch::Matchers::Equals(""));

    // Multiple variables
    semanticsProduces("PROGRAM BEGIN INTEGER a, b; FLOATING x; END", Catch::Matchers::Equals(""));
}

TEST_CASE("Coral Semantic Analysis - Scopes", "[semantics]")
{
    // Inner block
    semanticsProduces(
        R"(
        PROGRAM
        BEGIN
            INTEGER a;
            BEGIN
                FLOATING b;
            END;
        END
    )",
        Catch::Matchers::Equals(""));

    // Shadowing (if allowed in Coral, typically warnings or allowed)
    // For now assuming allowed
    semanticsProduces(
        R"(
        PROGRAM
        BEGIN
            INTEGER a;
            BEGIN
                FLOATING a; 
            END;
        END
    )",
        Catch::Matchers::Equals(""));
}

// NOTE: Since the current SemanticAnalysis.cpp implementation is a stub/skeleton,
// complex type checking (like assignment compatibility) tests might fail or produce nothing.
// These tests serve as a baseline for future implementation.

/*
TEST_CASE("Coral Semantic Analysis - Assignments", "[semantics]")
{
    // Valid assignment
    semanticsProduces("PROGRAM BEGIN INTEGER a; a := 5; END", Catch::Matchers::Equals(""));

    // Invalid assignment (Type mismatch) - Assuming Semantics implements this check
    // semanticsProduces("PROGRAM BEGIN INTEGER a; a := 1.5; END", Catch::Matchers::Contains("Type mismatch"));
}

TEST_CASE("Coral Semantic Analysis - Undeclared Identifier", "[semantics]")
{
    semanticsProduces("PROGRAM BEGIN a := 5; END", Catch::Matchers::Contains("Undeclared identifier"));
}
*/

TEST_CASE("Coral Array Declaration with Ranges", "[semantics][array]")
{
    // Single dimension with range
    semanticsProduces("PROGRAM BEGIN INTEGER ARRAY a[1:10]; END", Catch::Matchers::Equals(""));

    // Multiple dimensions with range
    semanticsProduces("PROGRAM BEGIN FLOATING ARRAY matrix[0:5, 1:4]; END", Catch::Matchers::Equals(""));

    // Multiple arrays
    semanticsProduces("PROGRAM BEGIN INTEGER ARRAY a[1:10], b[0:5]; END", Catch::Matchers::Equals(""));
}

TEST_CASE("Coral Table Declaration", "[semantics][table]")
{
    // Basic table
    semanticsProduces(
        R"(
        PROGRAM
        BEGIN
            TABLE t[10, 20]
            [
                INTEGER x;
                FIXED(16,8) y
            ];
        END
    )",
        Catch::Matchers::Equals(""));

    // Table with just width
    semanticsProduces(
        R"(
        PROGRAM
        BEGIN
            TABLE t[10]
            [
                INTEGER x
            ];
        END
    )",
        Catch::Matchers::Equals(""));
}

TEST_CASE("Coral Overlay Declaration", "[semantics][overlay]")
{
    // Overlay variable
    semanticsProduces(
        R"(
        PROGRAM
        BEGIN
            INTEGER x;
            OVERLAY x WITH INTEGER y;
        END
    )",
        Catch::Matchers::Equals(""));

    // Overlay with offset (assuming identifier for base)
    semanticsProduces(
        R"(
        PROGRAM
        BEGIN
            INTEGER ARRAY a[10];
            OVERLAY a WITH INTEGER b;
        END
    )",
        Catch::Matchers::Equals(""));
}

TEST_CASE("Coral For Loop", "[semantics][loop]")
{
    // Standard loop
    semanticsProduces(
        R"(
        PROGRAM
        BEGIN
            INTEGER i;
            INTEGER sum;
            sum := 0;
            FOR i := 1 STEP 1 UNTIL 10 DO
                sum := sum + i;
        END
    )",
        Catch::Matchers::Equals(""));

    // Loop with expressions
    semanticsProduces(
        R"(
        PROGRAM
        BEGIN
            INTEGER i, n, k;
            n := 10;
            k := 2;
            FOR i := n STEP -1 UNTIL 0 DO
                k := k * 2;
        END
    )",
        Catch::Matchers::Equals(""));
}

TEST_CASE("Coral Part Word Access", "[semantics][bits]")
{
    // BITS pseudo-function
    semanticsProduces(
        R"(
        PROGRAM
        BEGIN
            INTEGER a;
            INTEGER b;
            a := 0;
            BITS(0, 4, a) := 5;
            b := BITS(0, 4, a);
        END
    )",
        Catch::Matchers::Equals(""));
}

TEST_CASE("Coral Procedure Parameters", "[semantics][procedure]")
{
    // Procedure with parameters
    semanticsProduces(
        R"(
        PROGRAM
        BEGIN
            PROCEDURE p(INTEGER x, FLOATING y);
            BEGIN
                x := x + 1;
            END;
            
            p(1, 2.0);
        END
    )",
        Catch::Matchers::Equals(""));

    // Procedure with array parameter (assuming syntax)
    semanticsProduces(
        R"(
        PROGRAM
        BEGIN
            PROCEDURE sort(INTEGER ARRAY arr);
            BEGIN
                INTEGER temp;
            END;
        END
    )",
        Catch::Matchers::Equals(""));
}
