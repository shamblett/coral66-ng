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

using namespace coral66::Errors::Lexer;
using namespace Catch::Matchers;

using namespace coral66::Lexer;

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

TEST_CASE("Coral Lexing Identifiers - invalid identifier strings", "[lexer]")
{
    bool errors = false;
    auto result = lex("my/identifier var1?23 st+art BILLY frEd", errors);
    REQUIRE(result.data().size() == 8);
    CHECK(result.data()[0].getTokenType() == coral66::Lexer::TokenType::Identifier);
    CHECK(std::get<std::string>(result.data()[0].getValue()) == "my");
    CHECK(result.data()[1].getTokenType() == coral66::Lexer::TokenType::Division);
    CHECK(result.data()[2].getTokenType() == coral66::Lexer::TokenType::Identifier);
    CHECK(std::get<std::string>(result.data()[2].getValue()) == "identifier");
    CHECK(result.data()[3].getTokenType() == coral66::Lexer::TokenType::Identifier);
    CHECK(std::get<std::string>(result.data()[3].getValue()) == "var1");
    CHECK(result.data()[4].getTokenType() == coral66::Lexer::TokenType::Literal);
    CHECK(result.data()[4].getType() == coral66::Lexer::CToken::Integer);
    CHECK(result.data()[5].getTokenType() == coral66::Lexer::TokenType::Identifier);
    CHECK(std::get<std::string>(result.data()[5].getValue()) == "st");
    CHECK(result.data()[6].getTokenType() == coral66::Lexer::TokenType::Plus);
    CHECK(result.data()[7].getTokenType() == coral66::Lexer::TokenType::Identifier);
    CHECK(std::get<std::string>(result.data()[7].getValue()) == "art");
    
}

TEST_CASE("Coral Lexing Keyword = invalid strings", "[lexer]")
{
    SECTION("Single Keyword not found")
    {
        bool errors = false;
        auto result = lex("\t/yBEGIIIIIIIIIIIII/&&&HHHFFFFABSOLUTEhhhhhhhhhhhhhhh", errors);
        const std::vector correct = {TokenType::Division, TokenType::Division,};
        std::vector<TokenType> tokens;
        CHECK(result.data().size() == correct.size());
        tokens.reserve(result.data().size());
        std::ranges::transform(result.data(), std::back_inserter(tokens),
                               [](const CToken& token) { return token.getTokenType(); });
        CHECK_THAT(tokens, Catch::Matchers::Equals(correct));
        CHECK(errors);
    }

    SECTION("Single Keyword found")
    {
        bool errors = false;
        auto result = lex("\t/yBEGIIIIIIIIIIIII/&&&HHHFFFF ABSOLUTE hhhhhhhhhhhhhhh", errors);
        const std::vector correct = {TokenType::Division, TokenType::Division,
                                     TokenType::AbsoluteKeyword, TokenType::Identifier};
        std::vector<TokenType> tokens;
        CHECK(result.data().size() == correct.size());
        tokens.reserve(result.data().size());
        std::ranges::transform(result.data(), std::back_inserter(tokens),
                               [](const CToken& token) { return token.getTokenType(); });
        CHECK_THAT(tokens, Catch::Matchers::Equals(correct));
        CHECK(errors);
    }

    SECTION("Multiple invalid Keyword")
    {
        using namespace coral66::Lexer;
        bool errors = false;
        auto result = lex("BEGI HHHH AND COMMM COMMENT", errors);
        const std::vector correct = {TokenType::AndKeyword, TokenType::CommentKeyword};
        std::vector<TokenType> tokens;
        CHECK(result.data().size() == correct.size());
        tokens.reserve(result.data().size());
        std::ranges::transform(result.data(), std::back_inserter(tokens),
                               [](const CToken& token) { return token.getTokenType(); });
        CHECK_THAT(tokens, Catch::Matchers::Equals(correct));
        CHECK_FALSE(errors);
    }
}

TEST_CASE("Coral Lexing Literals - mixed strings", "[lexer]")
{
    SECTION("Integers")
    {
        bool errors = false;
        auto result = lex("123 0 9999", errors);
        REQUIRE(result.data().size() == 3);
        CHECK(result.data()[0].getTokenType() == coral66::Lexer::TokenType::Literal);
        CHECK(result.data()[0].getType() == coral66::Lexer::CToken::Integer);
        CHECK(result.data()[1].getTokenType() == coral66::Lexer::TokenType::Literal);
        CHECK(result.data()[1].getType() == coral66::Lexer::CToken::Integer);
        CHECK(result.data()[2].getTokenType() == coral66::Lexer::TokenType::Literal);
        CHECK(result.data()[2].getType() == coral66::Lexer::CToken::Integer);
    }

    SECTION("Floating Point")
    {
        bool errors = false;
        auto result = lex("3.14 0.0 .5 1.2e3", errors);
        REQUIRE(result.data().size() == 4);
        CHECK(result.data()[0].getTokenType() == coral66::Lexer::TokenType::Literal);
        CHECK(result.data()[0].getType() == coral66::Lexer::CToken::Floating);
        CHECK(result.data()[1].getTokenType() == coral66::Lexer::TokenType::Literal);
        CHECK(result.data()[1].getType() == coral66::Lexer::CToken::Floating);
        CHECK(result.data()[2].getTokenType() == coral66::Lexer::TokenType::Literal);
        CHECK(result.data()[2].getType() == coral66::Lexer::CToken::Floating);
        CHECK(result.data()[3].getTokenType() == coral66::Lexer::TokenType::Literal);
        CHECK(result.data()[3].getType() == coral66::Lexer::CToken::Floating);
    }

    SECTION("Strings")
    {
        bool errors = false;
        auto result = lex("'Hello World' 'It''s me' ''", errors);
        REQUIRE(result.data().size() == 3);
        CHECK(result.data()[0].getTokenType() == coral66::Lexer::TokenType::StringLiteral);
        CHECK(std::get<std::string>(result.data()[0].getValue()) == "Hello World");

        CHECK(result.data()[1].getTokenType() == coral66::Lexer::TokenType::StringLiteral);
        CHECK(std::get<std::string>(result.data()[1].getValue()) == "It's me");

        CHECK(result.data()[2].getTokenType() == coral66::Lexer::TokenType::StringLiteral);
        CHECK(std::get<std::string>(result.data()[2].getValue()).empty());
    }
}

TEST_CASE("Coral Lexing Operators and Punctuation - mixed strings", "[lexer]")
{
    bool errors = false;
    using namespace coral66::Lexer;
    auto result = lex("+ - * / = ( ) [ ] . : , ; <= >= <> := <-", errors);

    // Let's verify standard operators
    const std::vector correct = {TokenType::Plus,
                                 TokenType::Minus,
                                 TokenType::Asterisk,
                                 TokenType::Division,
                                 TokenType::Equal,
                                 TokenType::OpenParentheses,
                                 TokenType::CloseParentheses,
                                 TokenType::OpenSquareBracket,
                                 TokenType::CloseSquareBracket,
                                 TokenType::Dot,
                                 TokenType::Colon,
                                 TokenType::Comma,
                                 TokenType::SemiColon,
                                 TokenType::LessThanOrEqual,
                                 TokenType::GreaterThanOrEqual,
                                 TokenType::NotEqual,
                                 TokenType::Assignment};

    // Check first batch
    for (size_t i = 0; i < correct.size(); ++i)
    {
        CHECK(result.data()[i].getTokenType() == correct[i]);
    }

    // Check <- which should be LessThan, Minus
    const size_t offset = correct.size();
    CHECK(result.data()[offset].getTokenType() == TokenType::LessThan);
    CHECK(result.data()[offset + 1].getTokenType() == TokenType::Minus);
}
