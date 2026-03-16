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
        std::string source = "\"Hello World";
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

    SECTION("Unexpected Character")
    {
        bool errors = false;
        std::string source = "#";
        auto result = lexToCTokens(source, errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(UNEXPECTED_CHARACTER));
    }

    SECTION("Non-Printable Character")
    {
        bool errors = false;
        std::string source = "\u200B";
        auto result = lexToCTokens(source, errors);
        CHECK(result.findDiagnostic(NON_PRINTABLE_CHARACTER_N));
    }

    SECTION("Unterminated Stropped Keyword")
    {
        bool errors = false;
        std::string source = "'INTEGE hhhh";
        auto result = lexToCTokens(source, errors);
        CHECK(result.findDiagnostic(UNTERMINATED_STROPPED_KEYWORD));
    }

    SECTION("Invalid Stropped Keyword")
    {
        bool errors = false;
        std::string source = "'BEGIZ'";
        auto result = lexToCTokens(source, errors);
        CHECK(result.findDiagnostic(INVALID_STROPPED_KEYWORD_N));
    }

    SECTION("Unterminated Comment Keyword Sequence")
    {
        bool errors = false;
        std::string source = "COMMENT Hello";
        auto result = lex(source, errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(UNTERMINATED_COMMENT_KEYWORD_SEQUENCE));
    }

    SECTION("Malformed End Keyword Sequence")
    {
        bool errors = false;
        std::string source = "END 123";
        auto result = lex(source, errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(MALFORMED_END_KEYWORD_SEQUENCE));
    }

    SECTION("Unterminated Bracketed Comment")
    {
        bool errors = false;
        std::string source = "; ( this comment never ends";
        auto result = lex(source, errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(UNTERMINATED_BRACKETED_COMMENT));
    }

    SECTION("Malformed Number Literal")
    {
        bool errors = false;
        std::string source = "1.2.3";
        auto result = lexToCTokens(source, errors);
        CHECK(errors);
        CHECK(result.findDiagnostic(MALFORMED_NUMBER_LITERAL));
    }
}
