/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 23/03/2026
 */

#include "common.hpp"
#include "lib/frontend/syntax/syntax.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66;
using namespace coral66::Syntax;

/*
 * Tests cover section 5 of the specification
 */

/*
 * SWitch
 */
TEST_CASE("Coral Parse Place References", "[Parser][PlaceReference]")
{
    SECTION("Single label")
    {
        const ParsedPlaceReference p("SWITCH SW <- LABEL1");
        auto switchReference = dynamic_cast<SwitchPlaceReference*>(p.placeReference.get());
        REQUIRE(switchReference != nullptr);
        CHECK(std::get<std::string>(switchReference->getName()->getValue()) == "SW");
        CHECK(std::get<std::string>(switchReference->getLabels()[0]->getValue()) == "LABEL1");
    }

    SECTION("Multiple labels")
    {
        const ParsedPlaceReference p("SWITCH SW <- LABEL1, LABEL2, LABEL3");
        auto switchReference = dynamic_cast<SwitchPlaceReference*>(p.placeReference.get());
        REQUIRE(switchReference != nullptr);
        CHECK(std::get<std::string>(switchReference->getName()->getValue()) == "SW");
        CHECK(std::get<std::string>(switchReference->getLabels()[0]->getValue()) == "LABEL1");
        CHECK(std::get<std::string>(switchReference->getLabels()[1]->getValue()) == "LABEL2");
        CHECK(std::get<std::string>(switchReference->getLabels()[2]->getValue()) == "LABEL3");
    }
}