/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 02/04/2026
 */

#include "common.hpp"
#include "lib/frontend/common/messages/errors.hpp"
#include "lib/frontend/syntax/syntax.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66;
using namespace coral66::Syntax;

/*
 * Tests cover section 9 of the specification
 */

/*
 * Common Communicator
 */
TEST_CASE("Coral Parse Common Communicator", "[Parser][Communicator]")
{
    SECTION("Empty Common")
    {
        const ParsedCommunicator p("COMMON ()");
        auto communicatorReference = dynamic_cast<CommonCommunicator*>(p.communicator.get());
        REQUIRE(communicatorReference != nullptr);
        CHECK(communicatorReference->getItems().empty());
    }

    SECTION("Common With items")
    {
        const ParsedCommunicator p("COMMON ( INTEGER A ; LABEL B )");
        auto communicatorReference = dynamic_cast<CommonCommunicator*>(p.communicator.get());
        REQUIRE(communicatorReference != nullptr);
        const auto& items = communicatorReference->getItems();
        REQUIRE(items.size() == 2);

        auto varDecl = dynamic_cast<const VariableDeclaration*>(items[0].get());
        REQUIRE(varDecl != nullptr);
        CHECK(varDecl->getType()->getTokenType() == lexer::TokenType::IntegerKeyword);
        CHECK(varDecl->getIdentifiers().size() == 1);
        CHECK(varDecl->getIdentifiers()[0]->getText() == "A");

        auto placeSpec = dynamic_cast<const PlaceSpecification*>(items[1].get());
        REQUIRE(placeSpec != nullptr);
        CHECK(placeSpec->getIdentifiers().size() == 1);
        CHECK(placeSpec->getIdentifiers()[0]->getText() == "B");
    }

    SECTION("Common With SWITCH")
    {
        const ParsedCommunicator p("COMMON ( SWITCH S1, S2 )");
        auto communicatorReference = dynamic_cast<CommonCommunicator*>(p.communicator.get());
        REQUIRE(communicatorReference != nullptr);
        const auto& items = communicatorReference->getItems();
        REQUIRE(items.size() == 1);

        auto placeSpec = dynamic_cast<const PlaceSpecification*>(items[0].get());
        REQUIRE(placeSpec != nullptr);
        CHECK(placeSpec->getIdentifiers().size() == 2);
        CHECK(placeSpec->getIdentifiers()[0]->getText() == "S1");
        CHECK(placeSpec->getIdentifiers()[1]->getText() == "S2");
    }

    SECTION("Common With Procedure")
    {
        const ParsedCommunicator p("COMMON ( INTEGER PROCEDURE P(INTEGER, LABEL, VALUE FLOATING), Q )");
        auto communicatorReference = dynamic_cast<CommonCommunicator*>(p.communicator.get());
        REQUIRE(communicatorReference != nullptr);
        const auto& items = communicatorReference->getItems();
        REQUIRE(items.size() == 2);

        auto procSpec1 = dynamic_cast<const ProcedureSpecification*>(items[0].get());
        REQUIRE(procSpec1 != nullptr);
        CHECK(procSpec1->getName()->getText() == "P");
        REQUIRE(procSpec1->getType().has_value());
        CHECK(procSpec1->getType().value()->getTokenType() == lexer::TokenType::IntegerKeyword);
        const auto& params1 = procSpec1->getParameters();
        REQUIRE(params1.size() == 3);

        auto param1_1 = dynamic_cast<const ParameterDeclaration*>(params1[0].get());
        REQUIRE(param1_1 != nullptr);
        CHECK(param1_1->getMode() == ParameterDeclaration::ParameterMode::Value);

        auto param1_2 = dynamic_cast<const ParameterDeclaration*>(params1[1].get());
        REQUIRE(param1_2 != nullptr);
        CHECK(param1_2->getMode() == ParameterDeclaration::ParameterMode::Label);

        auto param1_3 = dynamic_cast<const ParameterDeclaration*>(params1[2].get());
        REQUIRE(param1_3 != nullptr);
        CHECK(param1_3->getMode() == ParameterDeclaration::ParameterMode::Value);
        REQUIRE(param1_3->getType().has_value());
        CHECK(param1_3->getType().value()->getTokenType() == lexer::TokenType::FloatingKeyword);

        auto procSpec2 = dynamic_cast<const ProcedureSpecification*>(items[1].get());
        REQUIRE(procSpec2 != nullptr);
        CHECK(procSpec2->getName()->getText() == "Q");
        REQUIRE(procSpec2->getType().has_value());
        CHECK(procSpec2->getType().value()->getTokenType() == lexer::TokenType::IntegerKeyword);
        CHECK(procSpec2->getParameters().empty());
    }

    SECTION("Common With Nested Procedure In Type List")
    {
        const ParsedCommunicator p("COMMON ( PROCEDURE P(PROCEDURE(INTEGER)) )");
        auto communicatorReference = dynamic_cast<CommonCommunicator*>(p.communicator.get());
        REQUIRE(communicatorReference != nullptr);
        const auto& items = communicatorReference->getItems();
        REQUIRE(items.size() == 1);

        auto procSpec1 = dynamic_cast<const ProcedureSpecification*>(items[0].get());
        REQUIRE(procSpec1 != nullptr);
        CHECK(procSpec1->getName()->getText() == "P");
        CHECK(procSpec1->getType().value() == &lexer::noneCToken);
        const auto& params1 = procSpec1->getParameters();
        REQUIRE(params1.size() == 1);

        auto param1 = dynamic_cast<const ParameterDeclaration*>(params1[0].get());
        REQUIRE(param1 != nullptr);
        CHECK(param1->getMode() == ParameterDeclaration::ParameterMode::Procedure);
    }

    SECTION("Common With Variables Initialized Via Assignment")
    {
        const ParsedCommunicator p("COMMON ( INTEGER A, B := 1, 2 )");
        auto communicatorReference = dynamic_cast<CommonCommunicator*>(p.communicator.get());
        REQUIRE(communicatorReference != nullptr);
        const auto& items = communicatorReference->getItems();
        REQUIRE(items.size() == 1);

        auto varDecl = dynamic_cast<const VariableDeclaration*>(items[0].get());
        REQUIRE(varDecl != nullptr);
        CHECK(varDecl->getIdentifiers().size() == 2);
        CHECK(varDecl->getIdentifiers()[0]->getText() == "A");
        CHECK(varDecl->getIdentifiers()[1]->getText() == "B");
    }

    SECTION("Common With Table Data Presets")
    {
        const ParsedCommunicator p("COMMON ( INTEGER ARRAY A[1:10] := 1, 2 )");
        auto communicatorReference = dynamic_cast<CommonCommunicator*>(p.communicator.get());
        REQUIRE(communicatorReference != nullptr);
        const auto& items = communicatorReference->getItems();
        REQUIRE(items.size() == 1);

        auto arrDecl = dynamic_cast<const ArrayDeclaration*>(items[0].get());
        REQUIRE(arrDecl != nullptr);
        CHECK(arrDecl->getIdentifiers()[0]->getText() == "A");
    }

    SECTION("Common With PRESET Keywords On Array")
    {
        const ParsedCommunicator p("COMMON ( INTEGER ARRAY A[1:10] PRESET 1, 2 )");
        auto communicatorReference = dynamic_cast<CommonCommunicator*>(p.communicator.get());
        REQUIRE(communicatorReference != nullptr);
        const auto& items = communicatorReference->getItems();
        REQUIRE(items.size() == 1);

        auto arrDecl = dynamic_cast<const ArrayDeclaration*>(items[0].get());
        REQUIRE(arrDecl != nullptr);
        CHECK(arrDecl->getIdentifiers()[0]->getText() == "A");
    }

    SECTION("Common With Fixed Parameter Procedures")
    {
        const ParsedCommunicator p("COMMON ( PROCEDURE P(FIXED(13,5), VALUE FIXED(24,2)) )");
        auto communicatorReference = dynamic_cast<CommonCommunicator*>(p.communicator.get());
        REQUIRE(communicatorReference != nullptr);
        const auto& items = communicatorReference->getItems();
        REQUIRE(items.size() == 1);

        auto procSpec = dynamic_cast<const ProcedureSpecification*>(items[0].get());
        REQUIRE(procSpec != nullptr);
        CHECK(procSpec->getName()->getText() == "P");
        const auto& params = procSpec->getParameters();
        REQUIRE(params.size() == 2);

        auto param1 = dynamic_cast<const ParameterDeclaration*>(params[0].get());
        REQUIRE(param1 != nullptr);
        CHECK(param1->getMode() == ParameterDeclaration::ParameterMode::Value);
        REQUIRE(param1->getType().has_value());
        CHECK(param1->getType().value()->getTokenType() == lexer::TokenType::FixedKeyword);

        auto param2 = dynamic_cast<const ParameterDeclaration*>(params[1].get());
        REQUIRE(param2 != nullptr);
        CHECK(param2->getMode() == ParameterDeclaration::ParameterMode::Value);
        REQUIRE(param2->getType().has_value());
        CHECK(param2->getType().value()->getTokenType() == lexer::TokenType::FixedKeyword);
    }
}
