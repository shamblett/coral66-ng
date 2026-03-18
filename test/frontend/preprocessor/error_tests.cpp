/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 18/03/2026
 */

#include <iostream>
#include <utility>

#include "lib/frontend/common/source_object.hpp"
#include "lib/frontend/common/messages/errors.hpp"
#include "lib/frontend/preprocessor/preprocessor.hpp"
#include "test/common/test_config.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

/*
 *
 * Error diagnostics tests
 *
 */

using namespace coral66::errors::pp;
namespace
{
coral66::PPSourceObject preprocess(std::string source,  bool &errorsOccurred)
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

TEST_CASE("Preprocess - Errors", "[PP]")
{
    SECTION("Empty DEFINE")
    {
        bool errors = false;
        auto result = preprocess("DEFINE ;", errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(PP_EXPECTED_N_AFTER_N));
    }

    SECTION("DEFINE Not Followed By Identifier")
    {
        bool errors = false;
        auto result = preprocess("DEFINE 66;", errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(PP_EXPECTED_N_INSTEAD_OF_N));
    }

    SECTION("Keyword Cannot Be A Macro Identifier")
    {
        bool errors = false;
        auto result = preprocess("DEFINE BEGIN", errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(KEYWORD_CANNOT_BE_A_MACRO_IDENTIFIER));
    }

    SECTION("Whitespace Required After Object Macro Definition")
    {
        bool errors = false;
        auto result = preprocess("DEFINE id\"5\";", errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(WHITESPACE_REQUIRED_AFTER_OBJECT_MACRO_DEFINITION));
    }
}
