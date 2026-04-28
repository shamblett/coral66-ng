/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 18/03/2026
 */

#include <iostream>

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
        const ParsedExpression p("IF THEN");
        CHECK(p.expression.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_CONDITION));
    }

    SECTION("Expected THEN")
    {
        const ParsedExpression p("IF a < b ELSE");
        CHECK(p.expression.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_THEN));
    }

    SECTION("Expected Statement After THEN")
    {
        const ParsedExpression p("IF a < b THEN ELSE");
        CHECK(p.expression.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_STATEMENT));
    }

    SECTION("Expected ELSE")
    {
        const ParsedExpression p("IF a < b THEN d IF");
        CHECK(p.expression.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_ELSE));
    }

    SECTION("Expected Statement After ELSE")
    {
        const ParsedExpression p("IF a < b THEN d ELSE IF;");
        CHECK(p.expression.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_STATEMENT));
    }

    SECTION("Expected Expression (Condition OR)")
    {
        const ParsedExpression p("IF a = b OR = THEN c ELSE d");
        CHECK(p.expression.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_EXPRESSION));
    }

    SECTION("Expected Comparator (Comparison)")
    {
        const ParsedExpression p("IF a THEN b ELSE c");
        CHECK(p.expression.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_COMPARATOR));
    }

    SECTION("Expected Simple Expression (Comparison Missing RHS)")
    {
        const ParsedExpression p("IF a = THEN c ELSE d");
        CHECK(p.expression.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_SIMPLE_EXPRESSION));
    }

    SECTION("Expected Term (Simple Expression)")
    {
        const ParsedExpression p("a + =");
        CHECK(p.expression.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_TERM));
    }

    SECTION("Expected Factor (Term)")
    {
        const ParsedExpression p("a * =");
        CHECK(p.expression.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_FACTOR));
    }

    SECTION("Expected Closed Parenthesis")
    {
        const ParsedExpression p("( a = ");
        CHECK(p.expression.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_CLOSE_PARENTHESIS));
    }

    SECTION("Expected Union (Bitwise Differ)")
    {
        const ParsedExpression p("a DIFFER =");
        CHECK(p.expression.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_UNION));
    }

    SECTION("Expected Mask (Bitwise Union)")
    {
        const ParsedExpression p("a UNION =");
        CHECK(p.expression.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_MASK));
    }

    SECTION("Expected Typed Primary (Bitwise Mask)")
    {
        const ParsedExpression p("a MASK =");
        CHECK(p.expression.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_TYPED_PRIMARY));
    }

    SECTION("Expected Word Reference (Typed Primary)")
    {
        const ParsedExpression p("LOCATION ( = )");
        CHECK(p.expression.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_WORD_REFERENCE));
    }

    SECTION("Expected Fraction Bits (Typed Primary)")
    {
        const ParsedExpression p("FIXED (1, ) (a)");
        CHECK(p.expression.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_FRACTION_BITS));
    }

    SECTION("Expected Open Square Bracket (Typed Primary/Word Reference)")
    {
        const ParsedExpression p("BITS = ");
        CHECK(p.expression.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_OPEN_SQUARE_BRACKET));
    }

    SECTION("Invalid Bits Format")
    {
        const ParsedExpression p("BITS [ 1, 2 ]");
        CHECK(p.expression.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_TYPED_PRIMARY));
    }

    SECTION("Invalid Bits Format missing comma")
    {
        const ParsedExpression p("BITS [ 1 ] a");
        CHECK(p.expression.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::INVALID_BITS_FORMAT));
    }

    SECTION("Expected Identifier (Word Reference)")
    {
        const ParsedExpression p("LOCATION ( = )");
        CHECK(p.expression.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_IDENTIFIER));
    }

    SECTION("Expected Open Parenthesis (Typed Primary)")
    {
        const ParsedExpression p("FIXED (1, 2) =");
        CHECK(p.expression.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_OPEN_PARENTHESIS));
    }

    SECTION("Expected Open Parenthesis (Typed Primary)")
    {
        const ParsedExpression p("FIXED (1, 2) =");
        CHECK(p.expression.get() == nullptr);
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
            const ParsedExpression p(src);
            CHECK(p.expression.get() == nullptr);
            CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        }
    }
}

TEST_CASE("Statements - Errors", "[Parser][Statement]")
{
    SECTION("Assignment - Invalid LHS - Expression")
    {
        const ParsedStatement s("A + 1 := 2;");
        CHECK(s.statement.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_VARIABLE));
    }

    SECTION("Assignment - Invalid LHS - Integer BITS Target")
    {
        const ParsedStatement s("BITS [ 2, 3 ] 4 := 5;");
        CHECK(s.statement.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_WORD_REFERENCE));
    }

    SECTION("Assignment - Invalid LHS - Location BITS Target")
    {
        const ParsedStatement s("BITS [ 2, 3 ] LOCATION(A) := 5;");
        CHECK(s.statement.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_WORD_REFERENCE));
    }

    SECTION("Assignment - No RHS")
    {
        const ParsedStatement s("A :=;");
        CHECK(s.statement.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_EXPRESSION));
    }

    SECTION("Assignment - No LHS")
    {
        const ParsedStatement s(":= 1;");
        CHECK(s.statement.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_STATEMENT));
    }

    SECTION("Invalid Procedure Call - Expression")
    {
        const ParsedStatement s("A + 1;");
        CHECK(s.statement.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_PROCEDURE_CALL));
    }

    SECTION("Invalid Procedure Call - Bits Start Without Assignment")
    {
        const ParsedStatement s("BITS [1,2] P;");
        CHECK(s.statement.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_ASSIGNMENT));
    }

    SECTION("Invalid Procedure Call - Array Start Without Assignment")
    {
        const ParsedStatement s("[LOCATION(P)];");
        CHECK(s.statement.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_ASSIGNMENT));
    }

    SECTION("Invalid Procedure Call - Literal")
    {
        const ParsedStatement s("123;");
        CHECK(s.statement.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_STATEMENT));
    }

    SECTION("Invalid Procedure Call - Array Element")
    {
        const ParsedStatement s("P[1];");
        CHECK(s.statement.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_PROCEDURE_CALL));
    }

    SECTION("Invalid Procedure Call - Add to function")
    {
        const ParsedStatement s("P(1) + 1;");
        CHECK(s.statement.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_PROCEDURE_CALL));
    }

    SECTION("Goto No Label")
    {
        const ParsedStatement s("GOTO ;");
        CHECK(s.statement.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_IDENTIFIER));
    }

    SECTION("Goto Switch No Close Square Bracket")
    {
        const ParsedStatement s("GOTO s[0");
        CHECK(s.statement.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_CLOSE_SQUARE_BRACKET));
    }

    SECTION("Answer No Expression")
    {
        const ParsedStatement s("ANSWER ;", true);
        REQUIRE(s.statement.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_EXPRESSION));
    }

    SECTION("Answer in Untyped Procedure")
    {
        const ParsedStatement s("ANSWER B;", false, true);
        REQUIRE(s.statement == nullptr);
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::ANSWER_IN_UNTYPED_PROCEDURE));
    }

    SECTION("Answer outside Procedure")
    {
        const ParsedStatement s("ANSWER B;", false, false);
        REQUIRE(s.statement == nullptr);
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::ANSWER_OUTSIDE_PROCEDURE));
    }

    SECTION("Code No Begin")
    {
        const ParsedStatement s("CODE \"mov eax\" END;");
        REQUIRE(s.statement.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_BEGIN));
    }

    SECTION("Code No Code")
    {
        const ParsedStatement s("CODE BEGIN END;");
        REQUIRE(s.statement.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_STRING_LITERAL));
    }

    SECTION("Code No End")
    {
        const ParsedStatement s("CODE BEGIN \"mov eax\" ;");
        REQUIRE(s.statement.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_END));
    }

    SECTION("Compound Statement - Missing END")
    {
        const ParsedStatement s("BEGIN A := 1;");
        CHECK(s.statement.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_END));
    }

    SECTION("Compound Statement - Missing Semicolon")
    {
        const ParsedStatement s("BEGIN A := 1 B := 2 END;");
        REQUIRE(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_SEMICOLON));
    }

    SECTION("Conditional Statement - IF No Condition")
    {
        const ParsedStatement s("IF THEN");
        CHECK(s.statement.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_CONDITION));
    }

    SECTION("Block missing semicolon between declarations")
    {
        const ParsedStatement s("BEGIN INTEGER A INTEGER B; A := 1 END;");
        REQUIRE(!parser::DiagnosticCollector::getDiagnostics().empty());
        REQUIRE(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_SEMICOLON));
    }

    SECTION("Block missing semicolon between decl and stmt")
    {
        const ParsedStatement s("BEGIN INTEGER A A := 1 END;");
        REQUIRE(!parser::DiagnosticCollector::getDiagnostics().empty());
        REQUIRE(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_SEMICOLON));
    }

    SECTION("Block with bad Switch Declaration")
    {
        const ParsedStatement s("BEGIN SWITCH S ; L1, L2; GOTO L1; END;");
        REQUIRE(!parser::DiagnosticCollector::getDiagnostics().empty());
        REQUIRE(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_ASSIGNMENT));
    }

    SECTION("Conditional Statement - IF No THEN")
    {
        const ParsedStatement s("IF A < 2");
        CHECK(s.statement.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_THEN));
    }

    SECTION("Conditional Statement - IF THEN No Consequence")
    {
        const ParsedStatement s("IF A < 2 THEN ELSE");
        CHECK(s.statement.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_STATEMENT));
    }

    SECTION("Conditional Statement - IF THEN ELSE No Alternative")
    {
        const ParsedStatement s("IF A < 2 THEN B:= 2 ELSE");
        CHECK(s.statement.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_STATEMENT));
    }

    SECTION("If Then - Labeled Nested If Forbidden")
    {
        const ParsedStatement s("IF A = 1 THEN L1: IF B = 2 THEN C := 3;");
        REQUIRE(!parser::DiagnosticCollector::getDiagnostics().empty());
        REQUIRE(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_SIMPLE_STATEMENT));
    }

    SECTION("If Then - Nested If Forbidden")
    {
        const ParsedStatement s("IF A = 1 THEN IF B = 2 THEN C := 3;");
        REQUIRE(!parser::DiagnosticCollector::getDiagnostics().empty());
        REQUIRE(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_SIMPLE_STATEMENT));
    }

    SECTION("If Then - Nested For Forbidden")
    {
        const ParsedStatement s("IF A = 1 THEN FOR I := 1 STEP 1 UNTIL 10 DO C := 3;");
        REQUIRE(!parser::DiagnosticCollector::getDiagnostics().empty());
        REQUIRE(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_SIMPLE_STATEMENT));
    }

    SECTION("For Statement - Missing Identifier")
    {
        const ParsedStatement s("FOR := 1 STEP 1 UNTIL 4 DO A :- A + 1");
        CHECK(s.statement.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_VARIABLE));
    }

    SECTION("For Statement - Missing Assignment")
    {
        const ParsedStatement s("FOR B 1 STEP 1 UNTIL 4 DO A :- A + 1");
        CHECK(s.statement.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_ASSIGNMENT));
    }

    SECTION("For Statement - Missing UNTIL")
    {
        const ParsedStatement s("FOR A := 1 STEP 1 DO B := B + 2");
        CHECK(s.statement.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_UNTIL));
    }

    SECTION("For Statement - Missing DO")
    {
        const ParsedStatement s("FOR A := 1 B := B + 2");
        CHECK(s.statement.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_DO));
    }

    SECTION("For Statement - Missing STEP")
    {
        const ParsedStatement s("FOR A := 1 UNTIL 4 DO B := B + 2");
        CHECK(s.statement.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_STEP));
    }
}

TEST_CASE("Place References - Errors", "[Parser][PlaceReference]")
{
}

TEST_CASE("Procedures - Errors", "[Parser][Procedure]")
{
    SECTION("Missing Identifier")
    {
        ParsedProcedure p("PROCEDURE ; BEGIN END");
        REQUIRE(p.procedure != nullptr);
        REQUIRE(!parser::DiagnosticCollector::getDiagnostics().empty());
        REQUIRE(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_IDENTIFIER));
    }

    SECTION("Typed Procedure Missing ANSWER statement")
    {
        ParsedProcedure p("INTEGER PROCEDURE p; BEGIN END");
        REQUIRE(p.procedure != nullptr);
        REQUIRE(!parser::DiagnosticCollector::getDiagnostics().empty());
        REQUIRE(parser::DiagnosticCollector::findDiagnostic(errors::parser::MISSING_ANSWER_STATEMENT));
    }

    SECTION("Missing Body Statement")
    {
        ParsedProcedure p("PROCEDURE p; BEGIN");
        REQUIRE(p.procedure != nullptr);
        REQUIRE(!parser::DiagnosticCollector::getDiagnostics().empty());
        REQUIRE(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_STATEMENT));
    }
}

TEST_CASE("Communicators - Errors", "[Parser][Communicator]")
{
    SECTION("Common Missing Open Parenthesis")
    {
        const ParsedCommunicator p("COMMON INTEGER A ; LABEL B )");
        REQUIRE(p.communicator == nullptr);
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_OPEN_PARENTHESIS));
    }

    SECTION("Common Missing Close Parenthesis")
    {
        const ParsedCommunicator p("COMMON ( INTEGER A ; LABEL B ");
        REQUIRE(p.communicator == nullptr);
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_CLOSE_PARENTHESIS));
    }

    SECTION("Common Missing Identifier In Procedure Specification")
    {
        const ParsedCommunicator p("COMMON ( PROCEDURE (INTEGER) )");
        REQUIRE(p.communicator == nullptr);
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_IDENTIFIER));
    }

    SECTION("Common Missing Semicolon Between Items")
    {
        const ParsedCommunicator p("COMMON ( LABEL A LABEL B )");
        REQUIRE(p.communicator == nullptr);
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_N));
    }
}

TEST_CASE("Declarations - Errors", "[Parser][Declaration]")
{
    SECTION("Missing Identifier")
    {
        ParsedDeclaration p("INTEGER ARRAY [1:2]");
        REQUIRE(p.declaration.empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_IDENTIFIER));
    }

    SECTION("Missing Colon")
    {
        ParsedDeclaration p("INTEGER ARRAY a[1]");
        REQUIRE(p.declaration.size() == 1);
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_COLON));
    }

    SECTION("Missing Close Bracket")
    {
        ParsedDeclaration p("INTEGER ARRAY a[1:2");
        REQUIRE(p.declaration.size() == 1);
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_CLOSE_SQUARE_BRACKET));
    }

    SECTION("Missing Element Type")
    {
        ParsedDeclaration p("TABLE t[10] [ x 0 ]");
        REQUIRE(p.declaration.size() == 1);
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_TYPE));
    }

    SECTION("Missing Element Identifier")
    {
        ParsedDeclaration p("TABLE t[10] [ INTEGER 0 ]");
        REQUIRE(p.declaration.size() == 1);
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_IDENTIFIER));
    }

    SECTION("Missing Expression")
    {
        ParsedDeclaration p("INTEGER a := *");
        REQUIRE(p.declaration.size() == 1);
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_EXPRESSION));
    }

    SECTION("Expected Declaration")
    {
        ParsedDeclaration p("OVERLAY x WITH");
        REQUIRE(p.declaration.empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_DECLARATION));
    }

    SECTION("Overlay Missing Declaration")
    {
        const ParsedStatement s("BEGIN OVERLAY x WITH ; END");
        REQUIRE(s.statement.get() != nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_DECLARATION));
    }

    SECTION("Procedure Parameter Missing Declaration")
    {
        const ParsedProcedure s("PROCEDURE P(INTEGER); BEGIN END");
        CHECK(s.procedure.get() == nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::EXPECTED_DECLARATION));
    }

    SECTION("Fixed Type Invalid Format")
    {
        const ParsedProcedure p("PROCEDURE P(FIXED(13) x); BEGIN END");
        CHECK(p.procedure.get() != nullptr);
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(errors::parser::INVALID_FIXED_FORMAT));
    }
}

TEST_CASE("Program structure", "[Parser][Program]")
{
    SECTION("No Program Or Segment")
    {
        const ParsedProgram p("TEST COMMON () BEGIN END");
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_PROGRAM_SEGMENT));
    }

    SECTION("Malformed Communicator")
    {
        const ParsedProgram p("SEGMENT TEST COMMON (hhhh hhh) BEGIN END");
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(MALFORMED_COMMUNICATOR));
    }

    SECTION("Misplaced Segment")
    {
        const ParsedProgram p("PROGRAM test COMMON () SEGMENT billy BEGIN END");
        CHECK(!parser::DiagnosticCollector::getDiagnostics().empty());
        CHECK(parser::DiagnosticCollector::findDiagnostic(EXPECTED_BEGIN));
    }
}