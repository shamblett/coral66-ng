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
}
