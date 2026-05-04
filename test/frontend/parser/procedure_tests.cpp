/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 07/03/2026
 */

#include "common.hpp"
#include "lib/frontend/syntax/syntax.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66;
using namespace coral66::syntax;

/*
 * Tests cover section 8 of the specification
 */

TEST_CASE("Procedures - Basic Declarations", "[Parser][Procedure]")
{
    SECTION("Untyped Procedure With No Parameters")
    {
        ParsedProcedure p("PROCEDURE p; BEGIN END");
        CHECK(p.procedure != nullptr);
        CHECK(p.procedure->getName()->getText() == "p");
        CHECK(!p.procedure->getTypeToken().has_value());
        CHECK(!p.procedure->isRecursive());
        CHECK(p.procedure->getParameters().empty());
    }

    SECTION("Recursive Procedure")
    {
        ParsedProcedure p("RECURSIVE PROCEDURE p; BEGIN END");
        CHECK(p.procedure != nullptr);
        CHECK(p.procedure->getName()->getText() == "p");
        CHECK(!p.procedure->getTypeToken().has_value());
        CHECK(p.procedure->isRecursive());
        CHECK(p.procedure->getParameters().empty());
    }

    SECTION("Typed Procedure With No Parameters - Integer")
    {
        ParsedProcedure p("INTEGER PROCEDURE p; ANSWER 0");
        CHECK(p.procedure != nullptr);
        CHECK(p.procedure->getName()->getText() == "p");
        CHECK(p.procedure->getTypeToken().has_value());
        CHECK(p.procedure->getTypeToken().value()->getTokenType() == lexer::TokenType::IntegerKeyword);
        CHECK(!p.procedure->isRecursive());
        CHECK(p.procedure->getParameters().empty());
    }

    SECTION("Typed Recursive Procedure")
    {
        ParsedProcedure p("INTEGER RECURSIVE PROCEDURE p; ANSWER 0");
        CHECK(p.procedure != nullptr);
        CHECK(p.procedure->getName()->getText() == "p");
        CHECK(p.procedure->getTypeToken().has_value());
        CHECK(p.procedure->getTypeToken().value()->getTokenType() == lexer::TokenType::IntegerKeyword);
        CHECK(p.procedure->isRecursive());
        CHECK(p.procedure->getParameters().empty());
    }

    SECTION("Typed Procedure With No Parameters - Floating")
    {
        ParsedProcedure p("FLOATING PROCEDURE p; ANSWER 0.0");
        CHECK(p.procedure != nullptr);
        CHECK(p.procedure->getName()->getText() == "p");
        CHECK(p.procedure->getTypeToken().has_value());
        CHECK(p.procedure->getTypeToken().value()->getTokenType() == lexer::TokenType::FloatingKeyword);
        CHECK(p.procedure->getParameters().empty());
    }

    SECTION("Typed Procedure With No Parameters - Fixed")
    {
        ParsedProcedure p("FIXED(10,5) PROCEDURE p; ANSWER 0");
        CHECK(p.procedure != nullptr);
        CHECK(p.procedure->getName()->getText() == "p");
        CHECK(p.procedure->getTypeToken().has_value());
        CHECK(p.procedure->getTypeToken().value()->getTokenType() == lexer::TokenType::FixedKeyword);
        CHECK(p.procedure->getParameters().empty());
    }
}

TEST_CASE("Procedures - Value/Location Parameters", "[Parser][Procedure]")
{
    SECTION("Single VALUE Parameter")
    {
        ParsedProcedure p("PROCEDURE p(VALUE INTEGER i); BEGIN END");
        CHECK(p.procedure != nullptr);
        CHECK(p.procedure->getParameters().size() == 1);
        auto* param = dynamic_cast<const ParameterDeclaration*>(p.procedure->getParameters()[0].get());
        CHECK(param != nullptr);
        CHECK(param->getMode() == ParameterDeclaration::ParameterMode::Value);
        CHECK(param->getType().has_value());
        CHECK(param->getType().value()->getTokenType() == lexer::TokenType::IntegerKeyword);
        CHECK(param->getIdentifiers().size() == 1);
        CHECK(param->getIdentifiers()[0]->getText() == "i");
    }

    SECTION("Multiple VALUE Parameters")
    {
        ParsedProcedure p("PROCEDURE p(VALUE FLOATING x, y); BEGIN END");
        CHECK(p.procedure != nullptr);
        CHECK(p.procedure->getParameters().size() == 1);
        auto* param = dynamic_cast<const ParameterDeclaration*>(p.procedure->getParameters()[0].get());
        CHECK(param != nullptr);
        CHECK(param->getMode() == ParameterDeclaration::ParameterMode::Value);
        CHECK(param->getType().has_value());
        CHECK(param->getType().value()->getTokenType() == lexer::TokenType::FloatingKeyword);
        CHECK(param->getIdentifiers().size() == 2);
        CHECK(param->getIdentifiers()[0]->getText() == "x");
        CHECK(param->getIdentifiers()[1]->getText() == "y");
    }

    SECTION("LOCATION Parameter")
    {
        ParsedProcedure p("PROCEDURE p(LOCATION FIXED(10,5) x); BEGIN END");
        CHECK(p.procedure != nullptr);
        CHECK(p.procedure->getParameters().size() == 1);
        auto* param = dynamic_cast<const ParameterDeclaration*>(p.procedure->getParameters()[0].get());
        CHECK(param != nullptr);
        CHECK(param->getMode() == ParameterDeclaration::ParameterMode::Location);
        CHECK(param->getType().has_value());
        CHECK(param->getType().value()->getTokenType() == lexer::TokenType::FixedKeyword);
        CHECK(param->getIdentifiers().size() == 1);
        CHECK(param->getIdentifiers()[0]->getText() == "x");
    }

    SECTION("Multiple Parameters Mixed")
    {
        ParsedProcedure p("PROCEDURE p(VALUE INTEGER i, j; LOCATION FLOATING x); BEGIN END");
        CHECK(p.procedure != nullptr);
        CHECK(p.procedure->getParameters().size() == 2);
        auto* param1 = dynamic_cast<const ParameterDeclaration*>(p.procedure->getParameters()[0].get());
        CHECK(param1 != nullptr);
        CHECK(param1->getMode() == ParameterDeclaration::ParameterMode::Value);
        CHECK(param1->getType().value()->getTokenType() == lexer::TokenType::IntegerKeyword);
        CHECK(param1->getIdentifiers().size() == 2);

        auto* param2 = dynamic_cast<const ParameterDeclaration*>(p.procedure->getParameters()[1].get());
        CHECK(param2 != nullptr);
        CHECK(param2->getMode() == ParameterDeclaration::ParameterMode::Location);
        CHECK(param2->getType().value()->getTokenType() == lexer::TokenType::FloatingKeyword);
        CHECK(param2->getIdentifiers().size() == 1);
        CHECK(param2->getIdentifiers()[0]->getText() == "x");
    }

    SECTION("Untyped VALUE Parameter (Non-Standard Extension)")
    {
        ParsedProcedure p("PROCEDURE p(VALUE u:v); BEGIN END");
        CHECK(p.procedure != nullptr);
        CHECK(p.procedure->getParameters().size() == 1);
        auto* param = dynamic_cast<const ParameterDeclaration*>(p.procedure->getParameters()[0].get());
        CHECK(param != nullptr);
        CHECK(param->getMode() == ParameterDeclaration::ParameterMode::UntypedValue);
        CHECK(!param->getType().has_value());
        CHECK(param->getIdentifiers().size() == 2);
        CHECK(param->getIdentifiers()[0]->getText() == "u");
        CHECK(param->getIdentifiers()[1]->getText() == "v");
    }
}

TEST_CASE("Procedures - Other Parameters", "[Parser][Procedure]")
{
    SECTION("LABEL and SWITCH Parameters")
    {
        ParsedProcedure p("PROCEDURE p(LABEL l1, l2; SWITCH s1); BEGIN END");
        CHECK(p.procedure != nullptr);
        CHECK(p.procedure->getParameters().size() == 2);

        auto* param1 = dynamic_cast<const ParameterDeclaration*>(p.procedure->getParameters()[0].get());
        CHECK(param1 != nullptr);
        CHECK(param1->getMode() == ParameterDeclaration::ParameterMode::Label);
        CHECK(param1->getIdentifiers().size() == 2);
        CHECK(param1->getIdentifiers()[0]->getText() == "l1");
        CHECK(param1->getIdentifiers()[1]->getText() == "l2");

        auto* param2 = dynamic_cast<const ParameterDeclaration*>(p.procedure->getParameters()[1].get());
        CHECK(param2 != nullptr);
        CHECK(param2->getMode() == ParameterDeclaration::ParameterMode::Switch);
        CHECK(param2->getIdentifiers().size() == 1);
        CHECK(param2->getIdentifiers()[0]->getText() == "s1");
    }

    SECTION("ARRAY Parameters")
    {
        ParsedProcedure p("PROCEDURE p(INTEGER ARRAY a, b); BEGIN END");
        CHECK(p.procedure != nullptr);
        CHECK(p.procedure->getParameters().size() == 1);

        auto* param1 = dynamic_cast<const ArrayDeclaration*>(p.procedure->getParameters()[0].get());
        CHECK(param1 != nullptr);
        CHECK(param1->getType()->getTokenType() == lexer::TokenType::IntegerKeyword);
        CHECK(param1->getIdentifiers().size() == 2);
        CHECK(param1->getIdentifiers()[0]->getText() == "a");
        CHECK(param1->getIdentifiers()[1]->getText() == "b");
        CHECK(param1->getDimensions().empty());
    }

    SECTION("TABLE Parameters")
    {
        ParsedProcedure p("PROCEDURE p(TABLE t[10,10] [ f1 INTEGER 0 ] ); BEGIN END");
        CHECK(p.procedure != nullptr);
        CHECK(p.procedure->getParameters().size() == 1);

        auto* param1 = dynamic_cast<const TableDeclaration*>(p.procedure->getParameters()[0].get());
        CHECK(param1 != nullptr);
        CHECK(param1->getIdentifier()->getText() == "t");
        CHECK(param1->getWidth() != nullptr);
        CHECK(param1->getLength() != nullptr);
    }

    SECTION("Untyped PROCEDURE Parameters")
    {
        ParsedProcedure p("PROCEDURE p(PROCEDURE q); BEGIN END");
        CHECK(p.procedure != nullptr);
        CHECK(p.procedure->getParameters().size() == 1);

        auto* param1 = dynamic_cast<const ParameterDeclaration*>(p.procedure->getParameters()[0].get());
        CHECK(param1 != nullptr);
        CHECK(param1->getMode() == ParameterDeclaration::ParameterMode::Procedure);
        CHECK(param1->getIdentifiers().size() == 1);
        CHECK(param1->getIdentifiers()[0]->getText() == "q");
    }

    SECTION("Untyped PROCEDURE Parameters With Type List")
    {
        ParsedProcedure p("PROCEDURE p(PROCEDURE q(VALUE INTEGER, LABEL)); BEGIN END");
        CHECK(p.procedure != nullptr);
        CHECK(p.procedure->getParameters().size() == 1);

        auto* param1 = dynamic_cast<const ParameterDeclaration*>(p.procedure->getParameters()[0].get());
        CHECK(param1 != nullptr);
        CHECK(param1->getMode() == ParameterDeclaration::ParameterMode::Procedure);
        CHECK(param1->getIdentifiers().size() == 1);
        CHECK(param1->getIdentifiers()[0]->getText() == "q");
    }
}
