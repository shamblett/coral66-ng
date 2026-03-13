/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include <llvm/Support/Format.h>

#include "lib/frontend/common/messages/errors.hpp"
#include "lib/frontend/common/source_object.hpp"
#include "lib/frontend/lexer/lexer.hpp"
#include "test/common/test_config.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

/*
 *
 * Tests cover section 10 and Appendix 2 of the specification
 * correct error generation
 *
 */

using namespace coral66::errors::lexer;
using namespace Catch::Matchers;

using namespace coral66::lexer;

namespace
{

coral66::PPSourceObject lex(const std::string& code, bool& errors)
{
    std::string buffer;
    llvm::raw_string_ostream ss(buffer);
    return tokenize(code, &ss, &errors);
}

coral66::CSourceObject lexToCTokens(const std::string& code, bool& errors)
{
    std::string buffer;
    llvm::raw_string_ostream ss(buffer);
    const auto result = tokenize(code, &ss, &errors);
    return toCTokens(result, &ss, &errors);
}

} // namespace

TEST_CASE("Tokenize - Errors", "[lexer]")
{
    SECTION("Unterminated String Literal")
    {
        bool errors = false;
        std::string source = "'Hello World";
        auto result = lex(source, errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(UNTERMINATED_STRING_LITERAL));
    }

    SECTION("Invalid UTF8 Sequence")
    {
        bool errors = false;
        std::string source = "\xFF";
        auto result = lex(source, errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(INVALID_UTF8_SEQUENCE));
    }

    SECTION("Expected Character After Backslash")
    {
        bool errors = false;
        std::string source = "'\\ '";
        auto result = lexToCTokens(source, errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(EXPECTED_CHARACTER_AFTER_BACKSLASH));
    }

    SECTION("Invalid Escape Sequence")
    {
        bool errors = false;
        std::string source = "'\\z'";
        auto result = lexToCTokens(source, errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(INVALID_ESCAPE_SEQUENCE_N));
    }

    SECTION("Newline in String Literal")
    {
        bool errors = false;
        std::string source = "'zz\nzz'";
        auto result = lexToCTokens(source, errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(NEWLINE_IN_STRING_LITERAL_USE_BACKLASH_N));
    }

    SECTION("Unexpected Character")
    {
        bool errors = false;
        std::string source = "#";
        auto result = lexToCTokens(source, errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(UNEXPECTED_CHARACTER));
    }

}
