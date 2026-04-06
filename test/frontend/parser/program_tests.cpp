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
        const ParsedProgram p("PROGRAM TEST COMMON ( A ; B ) BEGIN END");
        REQUIRE(p.program.getCommunicator() != nullptr);
        auto communicatorReference = dynamic_cast<const CommonCommunicator*>(p.program.getCommunicator());
        REQUIRE(communicatorReference != nullptr);
        auto items = communicatorReference->getItems();
        REQUIRE(items.size() == 2);
        CHECK(items[0]->getTokenType() == lexer::TokenType::Identifier);
        CHECK(items[0]->getText() == "A");
        CHECK(items[1]->getTokenType() == lexer::TokenType::Identifier);
        CHECK(items[1]->getText() == "B");
    }

    SECTION("Segment with Common Communicator")
    {
        const ParsedProgram p("SEGMENT TEST COMMON ( A ; B ) BEGIN END");
        REQUIRE(p.program.getCommunicator() != nullptr);
        auto communicatorReference = dynamic_cast<const CommonCommunicator*>(p.program.getCommunicator());
        REQUIRE(communicatorReference != nullptr);
    }
}
