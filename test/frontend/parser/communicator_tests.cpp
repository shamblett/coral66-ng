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
}