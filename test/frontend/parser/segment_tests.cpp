#include "test/vendor/catch2v3/catch_amalgamated.hpp"
#include "test/frontend/common/common.hpp"

using namespace coral66;
using namespace coral66::parser;

TEST_CASE("Parser Segment Keyword Compatibility", "[Parser][Segment]")
{
    SECTION("Ignores SEGMENT keyword inside block")
    {
        const ParsedStatement p("BEGIN SEGMENT S1 ; END");
        REQUIRE(DiagnosticCollector::getDiagnostics().empty());
        CHECK(dynamic_cast<syntax::CompoundStatement*>(p.statement.get()) != nullptr);
    }

    SECTION("Ignores SEGMENT keyword without identifier inside block")
    {
        const ParsedStatement p("BEGIN SEGMENT ; END");
        REQUIRE(DiagnosticCollector::getDiagnostics().empty());
        CHECK(dynamic_cast<syntax::CompoundStatement*>(p.statement.get()) != nullptr);
    }
}
