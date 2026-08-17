/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 18/03/2026
 */

#include <iostream>
#include <utility>

#include "lib/frontend/common/messages/errors.hpp"
#include "lib/frontend/common/source_object.hpp"
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

    SECTION("Whitespace Required After Object Macro Definition")
    {
        bool errors = false;
        auto result = preprocess("DEFINE id\"5\";", errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(WHITESPACE_REQUIRED_AFTER_OBJECT_MACRO_DEFINITION));
    }

    SECTION("No Matching Close Parenthesis")
    {
        bool errors = false;
        auto result = preprocess("DEFINE id(p1;", errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(PP_EXPECTED_N));
    }

    SECTION("Macro Parameter Name is not an Identifier")
    {
        bool errors = false;
        auto result = preprocess("DEFINE id(6);", errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(PP_EXPECTED_N_INSTEAD_OF_N));
    }

    SECTION("Expected Additional Macro Parameter")
    {
        bool errors = false;
        auto result = preprocess("DEFINE id(p1,;", errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(PP_EXPECTED_N));
    }

    SECTION("Missing Macro Parameter")
    {
        bool errors = false;
        auto result = preprocess("DEFINE id(p1,);", errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(PP_EXPECTED_N_INSTEAD_OF_N));
    }

    SECTION("Macro Parameter Redefinition")
    {
        bool errors = false;
        auto result = preprocess("DEFINE id(p1,p1);", errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(REDEFINITION_OF_MACRO_PARAMETER_N));
    }

    SECTION("Missing Close Parenthesis - Not Present")
    {
        bool errors = false;
        auto result = preprocess("DEFINE id(p1,p2;", errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(PP_EXPECTED_N));
    }

    SECTION("Missing Close Parenthesis - Not Parenthesis")
    {
        bool errors = false;
        auto result = preprocess("DEFINE id(p1,p2!;", errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(PP_EXPECTED_N_INSTEAD_OF_N));
    }

    SECTION("Incomplete DEFINE")
    {
        bool errors = false;
        auto result = preprocess("DEFINE\n", errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(PP_EXPECTED_N));
    }

    SECTION("Not Enough Arguments For Macro")
    {
        bool errors = false;
        auto result = preprocess("DEFINE mac(a, b) \"a + b\";\nmac(1);", errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(NOT_ENOUGH_ARGUMENTS_FOR_MACRO_N_EXPECTED_N_GOT_N));
    }

    SECTION("Too Many Arguments For Macro")
    {
        bool errors = false;
        auto result = preprocess("DEFINE mac(a, b) \"a + b\";\nmac(1, 2, 3);", errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(TOO_MANY_ARGUMENTS_FOR_MACRO_N_EXPECTED_N_GOT_N));
    }

    SECTION("Too Many Arguments For Zero-Arg Macro")
    {
        bool errors = false;
        auto result = preprocess("DEFINE mac() \"a + b\";\nmac(1);", errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(TOO_MANY_ARGUMENTS_FOR_MACRO_N_EXPECTED_N_GOT_N));
    }

    SECTION("Infinite Recursion Cycle Detected")
    {
        bool errors = false;
        auto result = preprocess("DEFINE A \"A\";\nA;", errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(INFINITE_MACRO_RECURSION_DETECTED_N));
    }

    SECTION("Infinite Recursion Mutual Cycle Detected")
    {
        bool errors = false;
        auto result = preprocess("DEFINE A \"B\";\nDEFINE B \"A\";\nA;", errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(INFINITE_MACRO_RECURSION_DETECTED_N));
    }
}
