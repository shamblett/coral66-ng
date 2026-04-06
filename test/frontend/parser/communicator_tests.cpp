/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 02/04/2026
 */

#include "common.hpp"
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

    SECTION("Common with items")
    {
        const ParsedCommunicator p("COMMON ( A ; B )");
        auto communicatorReference = dynamic_cast<CommonCommunicator*>(p.communicator.get());
        REQUIRE(communicatorReference != nullptr);
        auto items = communicatorReference->getItems();
        REQUIRE(items.size() == 2);
        CHECK(items[0]->getTokenType() == lexer::TokenType::Identifier);
        CHECK(items[0]->getText() == "A");
        CHECK(items[1]->getTokenType() == lexer::TokenType::Identifier);
        CHECK(items[1]->getText() == "B");
    }
}