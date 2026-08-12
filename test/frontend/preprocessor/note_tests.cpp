/*
 * Project: coral66-ng-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 19/03/2026
 */

#include <iostream>
#include <utility>

#include "lib/frontend/common/messages/notes.hpp"
#include "lib/frontend/common/source_object.hpp"
#include "lib/frontend/preprocessor/preprocessor.hpp"
#include "test/common/test_config.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

/*
 *
 * Note diagnostics tests
 *
 */

using namespace coral66::notes;
namespace
{
coral66::PPSourceObject preprocess(std::string source, bool& errorsOccurred)
{
    std::string storage;
    llvm::raw_string_ostream ss(storage);
    auto tokens = coral66::lexer::tokenize(std::move(source), &ss, &errorsOccurred);
    UNSCOPED_INFO(ss.str());
    REQUIRE_FALSE(errorsOccurred);
    auto result = coral66::pp::preprocess(std::move(tokens), &ss, &errorsOccurred);
    return result;
}
} // namespace

TEST_CASE("Preprocess - Notes", "[PP]")
{
    SECTION("Missing Close Parenthesis - Not Present")
    {
        bool errors = false;
        auto result = preprocess("DEFINE id(p1,p2;", errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(pp::TO_MATCH_N_HERE));
    }

    SECTION("Macro Parameter Redefinition")
    {
        bool errors = false;
        auto result = preprocess("DEFINE id(p1,p1);", errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(pp::PREVIOUSLY_DECLARED_HERE));
    }

    SECTION("Missing Close Parenthesis - Not Present")
    {
        bool errors = false;
        auto result = preprocess("DEFINE id(p1,p2;", errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(pp::TO_MATCH_N_HERE));
    }

    SECTION("Missing Close Parenthesis - Not Parenthesis")
    {
        bool errors = false;
        auto result = preprocess("DEFINE id(p1,p2!;", errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(pp::TO_MATCH_N_HERE));
    }
}
