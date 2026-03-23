/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 23/03/2026
 */

#include "common.hpp"
#include "lib/frontend/syntax/syntax.hpp"
#include "llvm/ADT/APSInt.h"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66;
using namespace coral66::Syntax;

/*
 * Tests cover section 5 of the specification
 */

/*
 * SWitch
 */
TEST_CASE("Coral Parse Place References",
          "[Parser][PlaceReference]"){SECTION("One label"){const ParsedPlaceReference p("SWITCH SW LABEL1");
auto placeReference = dynamic_cast<SwitchPlaceReference*>(p.placeReference.get());
REQUIRE(placeReference != nullptr);
}
}
;