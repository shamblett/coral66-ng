/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "lib/frontend/parser/parser.hpp"

#include "lib/frontend/common/error_messages.hpp"
#include "lib/frontend/common/source_object.hpp"
#include "test/common/test_config.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

#define treeProduces(source, matches)                             \
    do                                                            \
    {                                                             \
        std::string string;                                       \
        llvm::raw_string_ostream ss(string);                      \
        coral66::PPSourceObject tokens;                           \
        tokens = coral66::Lexer::tokenize(source, &ss); \
        CHECK_THAT(string, Catch::Matchers::Equals(""));          \
        string.clear();                                           \
        auto ctokens = coral66::Lexer::toCTokens(tokens, &ss);    \
        CHECK_THAT(string, Catch::Matchers::Equals(""));          \
        string.clear();                                           \
        auto tree = coral66::Parser::parseProgram(ctokens, &ss);  \
        CHECK_THAT(string, matches);                              \
    } while (0)

using namespace coral66::Errors::Parser;

TEST_CASE("Coral Parse Program Structure", "[parser]")
{
    treeProduces("PROGRAM BEGIN END", Catch::Matchers::Equals(""));
    // treeProduces("PROGRAM", Catch::Matchers::Contains("Expected 'BEGIN'")); // Parser expects BEGIN
}

TEST_CASE("Coral Parse Declarations", "[parser]")
{
    treeProduces("PROGRAM BEGIN INTEGER a; END", Catch::Matchers::Equals(""));
    treeProduces("PROGRAM BEGIN FLOATING x, y; END", Catch::Matchers::Equals(""));
    treeProduces("PROGRAM BEGIN FIXED f; END", Catch::Matchers::Equals(""));
    treeProduces("PROGRAM BEGIN INTEGER a, b, c; FLOATING d; END", Catch::Matchers::Equals(""));
}

TEST_CASE("Coral Parse Statements", "[parser]")
{
    SECTION("Assignment")
    {
        treeProduces("PROGRAM BEGIN a := 5; END", Catch::Matchers::Equals(""));
        treeProduces("PROGRAM BEGIN a := b; END", Catch::Matchers::Equals(""));
    }

    SECTION("Goto")
    {
        treeProduces("PROGRAM BEGIN GOTO label; END", Catch::Matchers::Equals(""));
    }

    SECTION("If")
    {
        // Simple IF THEN
        treeProduces("PROGRAM BEGIN IF a THEN b := 1; END", Catch::Matchers::Equals(""));
        // IF THEN ELSE
        treeProduces("PROGRAM BEGIN IF a THEN b := 1 ELSE b := 0; END", Catch::Matchers::Equals(""));
    }
}

TEST_CASE("Coral Parse Mixed", "[parser]")
{
    treeProduces(R"(
        PROGRAM
        BEGIN
            INTEGER i;
            FLOATING f;
            i := 0;
            f := 1.5;
            IF i THEN GOTO end;
            i := 1;
            end:;
        END
    )",
                 Catch::Matchers::Equals(""));
}

TEST_CASE("Coral Parse Fixed Type", "[parser]")
{
    treeProduces("PROGRAM BEGIN FIXED(16,8) f; END", Catch::Matchers::Equals(""));
    treeProduces("PROGRAM BEGIN FIXED(32,16) f1, f2; END", Catch::Matchers::Equals(""));
}

TEST_CASE("Coral Parse Table", "[parser]")
{
    treeProduces(R"(
        PROGRAM
        BEGIN
            TABLE T[10]
            [
                INTEGER x;
                FIXED(16,8) y
            ];
        END
    )",
                 Catch::Matchers::Equals(""));

    // Anonymous table?
    treeProduces(R"(
        PROGRAM
        BEGIN
            TABLE [10]
            [
                INTEGER x
            ] T;
        END
    )",
                 Catch::Matchers::Equals(""));
}

TEST_CASE("Coral Parse Overlay", "[parser]")
{
    treeProduces(R"(
        PROGRAM
        BEGIN
            OVERLAY T
            [
                INTEGER x;
                FIXED(16,8) y
            ];
        END
    )",
                 Catch::Matchers::Equals(""));
}

TEST_CASE("Coral Parse Answer Statement", "[parser]")
{
    treeProduces("PROGRAM BEGIN ANSWER 42; END", Catch::Matchers::Equals(""));
    treeProduces("PROGRAM BEGIN ANSWER x + 1; END", Catch::Matchers::Equals(""));
}

TEST_CASE("Coral Parse Code Statement", "[parser]")
{
    treeProduces("PROGRAM BEGIN CODE 'nop'; END", Catch::Matchers::Equals(""));
}

TEST_CASE("Coral Parse Procedures", "[parser]")
{
    treeProduces("PROGRAM BEGIN PROCEDURE p; BEGIN END; END", Catch::Matchers::Equals(""));
    treeProduces("PROGRAM BEGIN PROCEDURE p(x); BEGIN END; END", Catch::Matchers::Equals(""));
}

TEST_CASE("Coral Parse Loops", "[parser]")
{
    treeProduces("PROGRAM BEGIN FOR i := 0 STEP 1 UNTIL 10 DO x := x + 1; END", Catch::Matchers::Equals(""));
    // While loop (existing?)
    treeProduces("PROGRAM BEGIN WHILE i < 10 DO i := i + 1; END", Catch::Matchers::Equals(""));
}
