
/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 17/03/2026
 */

#include "lib/frontend/common/source_object.hpp"
#include "lib/frontend/lexer/lexer.hpp"
#include "test/common/test_config.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

/*
 * Lexer support package specific tests
 */

using namespace coral66::lexer;
using namespace coral66;

TEST_CASE("needsWhitespaceInBetween", "[lexer]")
{
    // isText(left)
    // left is Identifier, Keyword, etc.
    // right is StringLiteral, Literal, PPNumber -> true
    CHECK(needsWhitespaceInBetween(TokenType::Identifier, TokenType::StringLiteral) == true);
    CHECK(needsWhitespaceInBetween(TokenType::Identifier, TokenType::Literal) == true);
    CHECK(needsWhitespaceInBetween(TokenType::Identifier, TokenType::PPNumber) == true);
    // right is Text -> true
    CHECK(needsWhitespaceInBetween(TokenType::Identifier, TokenType::Identifier) == true);
    CHECK(needsWhitespaceInBetween(TokenType::Identifier, TokenType::BeginKeyword) == true);
    // right is not Text -> false
    CHECK(needsWhitespaceInBetween(TokenType::Identifier, TokenType::Plus) == false);

    // left is PPNumber
    // right is Text -> true
    CHECK(needsWhitespaceInBetween(TokenType::PPNumber, TokenType::Identifier) == true);
    // right is PPNumber -> true
    CHECK(needsWhitespaceInBetween(TokenType::PPNumber, TokenType::PPNumber) == true);
    // right is not Text or PPNumber -> false
    CHECK(needsWhitespaceInBetween(TokenType::PPNumber, TokenType::Plus) == false);

    // left is neither Text nor PPNumber -> false
    CHECK(needsWhitespaceInBetween(TokenType::Plus, TokenType::Identifier) == false);
    CHECK(needsWhitespaceInBetween(TokenType::Plus, TokenType::PPNumber) == false);
}

TEST_CASE("parseStringLiteral", "[lexer]")
{
    PPSourceObject sourceObject;
    std::string buffer;
    llvm::raw_string_ostream reporter(buffer);

    // Normal string
    PPToken normalToken(TokenType::StringLiteral, 0, 4, 0, 4, 0, 0, "\"AB\"");
    auto resultNormal = parseStringLiteral(normalToken, sourceObject, &reporter);
    CHECK(resultNormal.has_value());
    CHECK(resultNormal->getText() == "AB");

    // String with escaped quote ""
    PPToken escapedToken(TokenType::StringLiteral, 0, 6, 0, 6, 0, 0, R"("A""B")");
    auto resultEscaped = parseStringLiteral(escapedToken, sourceObject, &reporter);
    CHECK(resultEscaped.has_value());
    CHECK(resultEscaped->getText() == "A\"B");

    // Empty string
    PPToken emptyToken(TokenType::StringLiteral, 0, 2, 0, 2, 0, 0, "\"\"");
    auto resultEmpty = parseStringLiteral(emptyToken, sourceObject, &reporter);
    CHECK(resultEmpty.has_value());
    CHECK(resultEmpty->getText().empty());
}

TEST_CASE("toCTokens with PPSourceObject lvalue", "[lexer]")
{
    std::string buffer;
    llvm::raw_string_ostream reporter(buffer);
    bool errorsOccurred = false;

    // Create a simple token sequence
    auto ppSource = tokenize("BEGIN END", &reporter, &errorsOccurred);
    REQUIRE_FALSE(errorsOccurred);

    // Call lvalue overload
    const PPSourceObject& lvalueRef = ppSource;
    auto cSource = toCTokens(lvalueRef, &reporter, &errorsOccurred);
    REQUIRE_FALSE(errorsOccurred);
    CHECK(!cSource.data().empty());
}

TEST_CASE("toCTokens with PPTokenIterator", "[lexer]")
{
    std::string buffer;
    llvm::raw_string_ostream reporter(buffer);
    bool errorsOccurred = false;

    // Create a simple token sequence
    auto ppSource = tokenize("BEGIN END", &reporter, &errorsOccurred);
    REQUIRE_FALSE(errorsOccurred);

    // Call iterator overload
    auto cTokens = toCTokens(ppSource.data().data(), ppSource.data().data() + ppSource.data().size(), ppSource,
                             &reporter, &errorsOccurred);

    REQUIRE_FALSE(errorsOccurred);
    CHECK(!cTokens.empty());
}

TEST_CASE("CustomFormat for TokenType", "[lexer]")
{
    constexpr diagnostics::CustomFormat<U't', U'o', U'k', U'e', U'n', U'T', U'y', U'p', U'e'> formatter;
    const PPToken token(TokenType::Identifier, 0, 5, 0, 5, 0, 0, "dummy");
    const std::string formatted = formatter(token);
    CHECK(!formatted.empty());
}

TEST_CASE("StringConverter for TokenType", "[lexer]")
{
    const std::string inFormatStr =
        diagnostics::StringConverter<coral66::lexer::TokenType>::inFormat(TokenType::Identifier, nullptr);
    CHECK(!inFormatStr.empty());

    const std::string inArgStr = diagnostics::StringConverter<coral66::lexer::TokenType>::inArg(TokenType::Identifier, nullptr);
    CHECK(!inArgStr.empty());
}
