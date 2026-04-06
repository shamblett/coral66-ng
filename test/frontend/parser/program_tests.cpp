/*
* Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 06/04/2026
 */

#include "common.hpp"
#include "lib/frontend/syntax/syntax.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66;
using namespace coral66::Syntax;

TEST_CASE("Coral Parse Program with Communicator", "[Parser][Program]")
{
    SECTION("Empty Common Communicator")
    {
        const ParsedProgram p("PROGRAM TEST COMMON () BEGIN END");
        REQUIRE(p.program.getCommunicator() != nullptr);
        auto communicatorReference = dynamic_cast<const CommonCommunicator*>(p.program.getCommunicator());
        REQUIRE(communicatorReference != nullptr);
        CHECK(communicatorReference->getItems().empty());
    }

    SECTION("Common Communicator with Items")
    {
        const ParsedProgram p("PROGRAM TEST COMMON ( INTEGER A ; LABEL B ) BEGIN END");
        REQUIRE(p.program.getCommunicator() != nullptr);
        auto communicatorReference = dynamic_cast<const CommonCommunicator*>(p.program.getCommunicator());
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

    SECTION("Segment with Common Communicator")
    {
        const ParsedProgram p("SEGMENT TEST COMMON ( INTEGER A ) BEGIN END");
        REQUIRE(p.program.getCommunicator() != nullptr);
        auto communicatorReference = dynamic_cast<const CommonCommunicator*>(p.program.getCommunicator());
        REQUIRE(communicatorReference != nullptr);
    }
}
