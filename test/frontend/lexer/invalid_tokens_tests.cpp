/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include <llvm/Support/Format.h>

#include <array>

#include "lib/frontend/common/messages/errors.hpp"
#include "lib/frontend/common/source_object.hpp"
#include "lib/frontend/lexer/lexer.hpp"
#include "test/common/test_config.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

/*
 *
 * Tests cover section 10 and Appendix 2 of the specification.
 *
 */

using namespace coral66::errors::lexer;
using namespace Catch::Matchers;

using namespace coral66::lexer;

namespace
{

coral66::CSourceObject lex(const std::string& code, bool& errors)
{
    std::string buffer;
    llvm::raw_string_ostream ss(buffer);
    auto result = tokenize(code, &ss, &errors);
    UNSCOPED_INFO(buffer);
    auto tokens = toCTokens(std::move(result), &ss, &errors);
    UNSCOPED_INFO(buffer);
    return tokens;
}

} // namespace

TEST_CASE("Coral Lexing Identifiers - Invalid Identifier Strings", "[lexer]")
{
    bool errors = false;
    auto result = lex("my/identifier var1?23 st+art", errors);
    REQUIRE(result.data().size() == 8);
    CHECK(result.data()[0].getTokenType() == coral66::lexer::TokenType::Identifier);
    CHECK(std::get<std::string>(result.data()[0].getValue()) == "my");
    CHECK(result.data()[1].getTokenType() == coral66::lexer::TokenType::Division);
    CHECK(result.data()[2].getTokenType() == coral66::lexer::TokenType::Identifier);
    CHECK(std::get<std::string>(result.data()[2].getValue()) == "identifier");
    CHECK(result.data()[3].getTokenType() == coral66::lexer::TokenType::Identifier);
    CHECK(std::get<std::string>(result.data()[3].getValue()) == "var1");
    CHECK(result.data()[4].getTokenType() == coral66::lexer::TokenType::Literal);
    CHECK(result.data()[4].getType() == coral66::lexer::CToken::Integer);
    CHECK(std::get<llvm::APSInt>(result.data()[4].getValue()).getExtValue() == 23);
    CHECK(result.data()[5].getTokenType() == coral66::lexer::TokenType::Identifier);
    CHECK(std::get<std::string>(result.data()[5].getValue()) == "st");
    CHECK(result.data()[6].getTokenType() == coral66::lexer::TokenType::Plus);
    CHECK(result.data()[7].getTokenType() == coral66::lexer::TokenType::Identifier);
    CHECK(std::get<std::string>(result.data()[7].getValue()) == "art");
}

TEST_CASE("Coral Lexing Keyword = Invalid Strings", "[lexer]")
{
    SECTION("Single Keyword not found")
    {
        bool errors = false;
        auto result = lex("\t/yBEGIIIIIIIIIIIII/&&&HHHFFFFABSOLUTEhhhhhhhhhhhhhhh", errors);
        const std::vector correct = {
            TokenType::Division,
            TokenType::Identifier,
            TokenType::Division,
            TokenType::Identifier,
        };
        std::vector<TokenType> tokens;
        CHECK(result.data().size() == correct.size());
        tokens.reserve(result.data().size());
        std::ranges::transform(result.data(), std::back_inserter(tokens),
                               [](const CToken& token) { return token.getTokenType(); });
        CHECK_THAT(tokens, Catch::Matchers::Equals(correct));
        CHECK(errors);
    }

    SECTION("Single Keyword Found")
    {
        bool errors = false;
        auto result = lex("\t/yBEGIIIIIIIIIIIII/&&&HHHFFFF ABSOLUTE hhhhhhhhhhhhhhh", errors);
        const std::vector correct = {TokenType::Division,   TokenType::Identifier,      TokenType::Division,
                                     TokenType::Identifier, TokenType::AbsoluteKeyword, TokenType::Identifier};
        std::vector<TokenType> tokens;
        CHECK(result.data().size() == correct.size());
        tokens.reserve(result.data().size());
        std::ranges::transform(result.data(), std::back_inserter(tokens),
                               [](const CToken& token) { return token.getTokenType(); });
        CHECK_THAT(tokens, Catch::Matchers::Equals(correct));
        CHECK(errors);
    }

    SECTION("Multiple Invalid Keyword")
    {
        using namespace coral66::lexer;
        bool errors = false;
        auto result = lex("BEGI HHHH AND COMMM COMMENT ;", errors);
        const std::vector correct = {TokenType::Identifier, TokenType::Identifier,     TokenType::AndKeyword,
                                     TokenType::Identifier, TokenType::CommentKeyword, TokenType::SemiColon};
        std::vector<TokenType> tokens;
        CHECK(result.data().size() == correct.size());
        tokens.reserve(result.data().size());
        std::ranges::transform(result.data(), std::back_inserter(tokens),
                               [](const CToken& token) { return token.getTokenType(); });
        CHECK_THAT(tokens, Catch::Matchers::Equals(correct));
        CHECK_FALSE(errors);
    }
}

TEST_CASE("Coral Lexing Literals - Mixed Strings", "[lexer]")
{
    SECTION("Integers")
    {
        bool errors = false;
        auto result = lex("12#3 !0 9999g", errors);
        REQUIRE(result.data().size() == 5);
        CHECK(result.data()[0].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[0].getType() == coral66::lexer::CToken::Integer);
        CHECK(std::get<llvm::APSInt>(result.data()[0].getValue()) == 12);
        CHECK(result.data()[1].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[1].getType() == coral66::lexer::CToken::Integer);
        CHECK(std::get<llvm::APSInt>(result.data()[1].getValue()) == 3);
        CHECK(result.data()[2].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[2].getType() == coral66::lexer::CToken::Integer);
        CHECK(std::get<llvm::APSInt>(result.data()[2].getValue()) == 0);
        CHECK(result.data()[3].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[3].getType() == coral66::lexer::CToken::Integer);
        CHECK(std::get<llvm::APSInt>(result.data()[3].getValue()) == 9999);
        CHECK(result.data()[4].getTokenType() == coral66::lexer::TokenType::Identifier);
        CHECK(std::get<std::string>(result.data()[4].getValue()) == "g");
        CHECK(errors);
    }

    SECTION("Floating Point")
    {
        bool errors = false;
        auto result = lex("3.14h 0.gg0 rr.5 1.2eo", errors);
        REQUIRE(result.data().size() == 8);
        CHECK(result.data()[0].getType() == coral66::lexer::CToken::Floating);
        CHECK(result.data()[1].getTokenType() == coral66::lexer::TokenType::Identifier);
        CHECK(result.data()[2].getType() == coral66::lexer::CToken::Floating);
        CHECK(result.data()[3].getTokenType() == coral66::lexer::TokenType::Identifier);
        CHECK(std::get<std::string>(result.data()[3].getValue()) == "gg0");
        CHECK(result.data()[4].getTokenType() == coral66::lexer::TokenType::Identifier);
        CHECK(result.data()[5].getType() == coral66::lexer::CToken::Floating);
        CHECK(result.data()[6].getType() == coral66::lexer::CToken::Floating);
        CHECK(result.data()[7].getTokenType() == coral66::lexer::TokenType::Identifier);
    }

    SECTION("Strings")
    {
        bool errors = false;
        auto result = lex(R"("Hello World" "It"s me" "")", errors);
        REQUIRE(result.data().size() == 4);
        CHECK(result.data()[0].getTokenType() == coral66::lexer::TokenType::StringLiteral);
        CHECK(std::get<std::string>(result.data()[0].getValue()) == "Hello World");
        CHECK(result.data()[1].getTokenType() == coral66::lexer::TokenType::StringLiteral);
        CHECK(std::get<std::string>(result.data()[1].getValue()) == "It");
        CHECK(result.data()[2].getTokenType() == coral66::lexer::TokenType::Identifier);
        CHECK(std::get<std::string>(result.data()[2].getValue()) == "s");
        CHECK(result.data()[3].getTokenType() == coral66::lexer::TokenType::Identifier);
        CHECK(std::get<std::string>(result.data()[3].getValue()) == "me");
    }
}
