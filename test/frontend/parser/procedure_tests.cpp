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
 * Tests cover section 8 of the specification
 */

TEST_CASE("Procedures - Basic Declarations", "[Parser][Procedure]")
{
    SECTION("Untyped Procedure With No Parameters")
    {
        ParsedProcedure p("PROCEDURE p; BEGIN END");
        REQUIRE(p.procedure != nullptr);
        REQUIRE(p.procedure->getName()->getText() == "p");
        REQUIRE(!p.procedure->getTypeToken().has_value());
        REQUIRE(!p.procedure->isRecursive());
        REQUIRE(p.procedure->getParameters().empty());
    }

    SECTION("Recursive Procedure")
    {
        ParsedProcedure p("RECURSIVE PROCEDURE p; BEGIN END");
        REQUIRE(p.procedure != nullptr);
        REQUIRE(p.procedure->getName()->getText() == "p");
        REQUIRE(!p.procedure->getTypeToken().has_value());
        REQUIRE(p.procedure->isRecursive());
        REQUIRE(p.procedure->getParameters().empty());
    }

    SECTION("Typed Procedure With No Parameters - Integer")
    {
        ParsedProcedure p("INTEGER PROCEDURE p; ANSWER 0");
        REQUIRE(p.procedure != nullptr);
        REQUIRE(p.procedure->getName()->getText() == "p");
        REQUIRE(p.procedure->getTypeToken().has_value());
        REQUIRE(p.procedure->getTypeToken().value()->getTokenType() == lexer::TokenType::IntegerKeyword);
        REQUIRE(!p.procedure->isRecursive());
        REQUIRE(p.procedure->getParameters().empty());
    }

    SECTION("Typed Recursive Procedure")
    {
        ParsedProcedure p("INTEGER RECURSIVE PROCEDURE p; ANSWER 0");
        REQUIRE(p.procedure != nullptr);
        REQUIRE(p.procedure->getName()->getText() == "p");
        REQUIRE(p.procedure->getTypeToken().has_value());
        REQUIRE(p.procedure->getTypeToken().value()->getTokenType() == lexer::TokenType::IntegerKeyword);
        REQUIRE(p.procedure->isRecursive());
        REQUIRE(p.procedure->getParameters().empty());
    }

    SECTION("Typed Procedure With No Parameters - Floating")
    {
        ParsedProcedure p("FLOATING PROCEDURE p; ANSWER 0.0");
        REQUIRE(p.procedure != nullptr);
        REQUIRE(p.procedure->getName()->getText() == "p");
        REQUIRE(p.procedure->getTypeToken().has_value());
        REQUIRE(p.procedure->getTypeToken().value()->getTokenType() == lexer::TokenType::FloatingKeyword);
        REQUIRE(p.procedure->getParameters().empty());
    }

    SECTION("Typed Procedure With No Parameters - Fixed")
    {
        ParsedProcedure p("FIXED(10,5) PROCEDURE p; ANSWER 0");
        REQUIRE(p.procedure != nullptr);
        REQUIRE(p.procedure->getName()->getText() == "p");
        REQUIRE(p.procedure->getTypeToken().has_value());
        REQUIRE(p.procedure->getTypeToken().value()->getTokenType() == lexer::TokenType::FixedKeyword);
        REQUIRE(p.procedure->getParameters().empty());
    }
}

TEST_CASE("Procedures - Value/Location Parameters", "[Parser][Procedure]")
{
    SECTION("Single VALUE Parameter")
    {
        ParsedProcedure p("PROCEDURE p(VALUE INTEGER i); BEGIN END");
        REQUIRE(p.procedure != nullptr);
        REQUIRE(p.procedure->getParameters().size() == 1);
        auto* param = dynamic_cast<const ParameterDeclaration*>(p.procedure->getParameters()[0].get());
        REQUIRE(param != nullptr);
        REQUIRE(param->getMode() == ParameterDeclaration::ParameterMode::Value);
        REQUIRE(param->getType().has_value());
        REQUIRE(param->getType().value()->getTokenType() == lexer::TokenType::IntegerKeyword);
        REQUIRE(param->getIdentifiers().size() == 1);
        REQUIRE(param->getIdentifiers()[0]->getText() == "i");
    }

    SECTION("Multiple VALUE Parameters")
    {
        ParsedProcedure p("PROCEDURE p(VALUE FLOATING x, y); BEGIN END");
        REQUIRE(p.procedure != nullptr);
        REQUIRE(p.procedure->getParameters().size() == 1);
        auto* param = dynamic_cast<const ParameterDeclaration*>(p.procedure->getParameters()[0].get());
        REQUIRE(param != nullptr);
        REQUIRE(param->getMode() == ParameterDeclaration::ParameterMode::Value);
        REQUIRE(param->getType().has_value());
        REQUIRE(param->getType().value()->getTokenType() == lexer::TokenType::FloatingKeyword);
        REQUIRE(param->getIdentifiers().size() == 2);
        REQUIRE(param->getIdentifiers()[0]->getText() == "x");
        REQUIRE(param->getIdentifiers()[1]->getText() == "y");
    }

    SECTION("LOCATION Parameter")
    {
        ParsedProcedure p("PROCEDURE p(LOCATION FIXED(10,5) x); BEGIN END");
        REQUIRE(p.procedure != nullptr);
        REQUIRE(p.procedure->getParameters().size() == 1);
        auto* param = dynamic_cast<const ParameterDeclaration*>(p.procedure->getParameters()[0].get());
        REQUIRE(param != nullptr);
        REQUIRE(param->getMode() == ParameterDeclaration::ParameterMode::Location);
        REQUIRE(param->getType().has_value());
        REQUIRE(param->getType().value()->getTokenType() == lexer::TokenType::FixedKeyword);
        REQUIRE(param->getIdentifiers().size() == 1);
        REQUIRE(param->getIdentifiers()[0]->getText() == "x");
    }

    SECTION("Multiple Parameters Mixed")
    {
        ParsedProcedure p("PROCEDURE p(VALUE INTEGER i, j; LOCATION FLOATING x); BEGIN END");
        REQUIRE(p.procedure != nullptr);
        REQUIRE(p.procedure->getParameters().size() == 2);
        auto* param1 = dynamic_cast<const ParameterDeclaration*>(p.procedure->getParameters()[0].get());
        REQUIRE(param1 != nullptr);
        REQUIRE(param1->getMode() == ParameterDeclaration::ParameterMode::Value);
        REQUIRE(param1->getType().value()->getTokenType() == lexer::TokenType::IntegerKeyword);
        REQUIRE(param1->getIdentifiers().size() == 2);

        auto* param2 = dynamic_cast<const ParameterDeclaration*>(p.procedure->getParameters()[1].get());
        REQUIRE(param2 != nullptr);
        REQUIRE(param2->getMode() == ParameterDeclaration::ParameterMode::Location);
        REQUIRE(param2->getType().value()->getTokenType() == lexer::TokenType::FloatingKeyword);
        REQUIRE(param2->getIdentifiers().size() == 1);
        REQUIRE(param2->getIdentifiers()[0]->getText() == "x");
    }

    SECTION("Untyped VALUE Parameter (Non-Standard Extension)")
    {
        ParsedProcedure p("PROCEDURE p(VALUE u:v); BEGIN END");
        REQUIRE(p.procedure != nullptr);
        REQUIRE(p.procedure->getParameters().size() == 1);
        auto* param = dynamic_cast<const ParameterDeclaration*>(p.procedure->getParameters()[0].get());
        REQUIRE(param != nullptr);
        REQUIRE(param->getMode() == ParameterDeclaration::ParameterMode::UntypedValue);
        REQUIRE(!param->getType().has_value());
        REQUIRE(param->getIdentifiers().size() == 2);
        REQUIRE(param->getIdentifiers()[0]->getText() == "u");
        REQUIRE(param->getIdentifiers()[1]->getText() == "v");
    }
}

TEST_CASE("Procedures - Other Parameters", "[Parser][Procedure]")
{
    SECTION("LABEL and SWITCH Parameters")
    {
        ParsedProcedure p("PROCEDURE p(LABEL l1, l2; SWITCH s1); BEGIN END");
        REQUIRE(p.procedure != nullptr);
        REQUIRE(p.procedure->getParameters().size() == 2);

        auto* param1 = dynamic_cast<const ParameterDeclaration*>(p.procedure->getParameters()[0].get());
        REQUIRE(param1 != nullptr);
        REQUIRE(param1->getMode() == ParameterDeclaration::ParameterMode::Label);
        REQUIRE(param1->getIdentifiers().size() == 2);
        REQUIRE(param1->getIdentifiers()[0]->getText() == "l1");
        REQUIRE(param1->getIdentifiers()[1]->getText() == "l2");

        auto* param2 = dynamic_cast<const ParameterDeclaration*>(p.procedure->getParameters()[1].get());
        REQUIRE(param2 != nullptr);
        REQUIRE(param2->getMode() == ParameterDeclaration::ParameterMode::Switch);
        REQUIRE(param2->getIdentifiers().size() == 1);
        REQUIRE(param2->getIdentifiers()[0]->getText() == "s1");
    }

    SECTION("ARRAY Parameters")
    {
        ParsedProcedure p("PROCEDURE p(INTEGER ARRAY a, b); BEGIN END");
        REQUIRE(p.procedure != nullptr);
        REQUIRE(p.procedure->getParameters().size() == 1);

        auto* param1 = dynamic_cast<const ArrayDeclaration*>(p.procedure->getParameters()[0].get());
        REQUIRE(param1 != nullptr);
        REQUIRE(param1->getType()->getTokenType() == lexer::TokenType::IntegerKeyword);
        REQUIRE(param1->getIdentifiers().size() == 2);
        REQUIRE(param1->getIdentifiers()[0]->getText() == "a");
        REQUIRE(param1->getIdentifiers()[1]->getText() == "b");
        REQUIRE(param1->getDimensions().empty());
    }

    SECTION("TABLE Parameters")
    {
        ParsedProcedure p("PROCEDURE p(TABLE t[10,10] [ f1 INTEGER 0 ] ); BEGIN END");
        REQUIRE(p.procedure != nullptr);
        REQUIRE(p.procedure->getParameters().size() == 1);

        auto* param1 = dynamic_cast<const TableDeclaration*>(p.procedure->getParameters()[0].get());
        REQUIRE(param1 != nullptr);
        REQUIRE(param1->getIdentifier()->getText() == "t");
        REQUIRE(param1->getWidth() != nullptr);
        REQUIRE(param1->getLength() != nullptr);
    }

    SECTION("Untyped PROCEDURE Parameters")
    {
        ParsedProcedure p("PROCEDURE p(PROCEDURE q); BEGIN END");
        REQUIRE(p.procedure != nullptr);
        REQUIRE(p.procedure->getParameters().size() == 1);

        auto* param1 = dynamic_cast<const ParameterDeclaration*>(p.procedure->getParameters()[0].get());
        REQUIRE(param1 != nullptr);
        REQUIRE(param1->getMode() == ParameterDeclaration::ParameterMode::Procedure);
        REQUIRE(param1->getIdentifiers().size() == 1);
        REQUIRE(param1->getIdentifiers()[0]->getText() == "q");
    }

    SECTION("Untyped PROCEDURE Parameters With Type List")
    {
        ParsedProcedure p("PROCEDURE p(PROCEDURE q(VALUE INTEGER, LABEL)); BEGIN END");
        REQUIRE(p.procedure != nullptr);
        REQUIRE(p.procedure->getParameters().size() == 1);

        auto* param1 = dynamic_cast<const ParameterDeclaration*>(p.procedure->getParameters()[0].get());
        REQUIRE(param1 != nullptr);
        REQUIRE(param1->getMode() == ParameterDeclaration::ParameterMode::Procedure);
        REQUIRE(param1->getIdentifiers().size() == 1);
        REQUIRE(param1->getIdentifiers()[0]->getText() == "q");
    }
}
