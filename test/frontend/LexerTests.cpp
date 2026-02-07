/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "lib/frontend/compiler/lexer.hpp"

#include <llvm/Support/Format.h>

#include <array>

#include "lib/frontend/common/error_messages.hpp"
#include "lib/frontend/common/source_object.hpp"
#include "test/common/TestConfig.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66::Errors::Lexer;
using namespace Catch::Matchers;

namespace
{

coral66::CSourceObject lexes(std::string code, const coral66::LanguageOptions& options = coral66::LanguageOptions::native())
{
    std::string buffer;
    llvm::raw_string_ostream ss(buffer);
    bool errors;
    auto result = coral66::Lexer::tokenize(std::move(code), &options, &ss, &errors);
    UNSCOPED_INFO(buffer);
    REQUIRE_FALSE(errors);
    auto tokens = coral66::Lexer::toCTokens(std::move(result), &ss, &errors);
    UNSCOPED_INFO(buffer);
    REQUIRE_FALSE(errors);
    return tokens;
}

} // namespace

TEST_CASE("Coral Lexing Keywords", "[lexer]")
{
    using namespace coral66::Lexer;
    auto result = lexes(
        "BEGIN END PROCEDURE PROGRAM SEGMENT IF THEN ELSE FOR STEP UNTIL DO GOTO ANSWER INTEGER FLOATING FIXED BITS ARRAY TABLE PRESET OVERLAY VALUE LOCATION COMMON COMMENT DEFINE CODE");
    
    std::vector correct = {
        TokenType::BeginKeyword, TokenType::EndKeyword, TokenType::ProcedureKeyword, TokenType::ProgramKeyword,
        TokenType::SegmentKeyword, TokenType::IfKeyword, TokenType::ThenKeyword, TokenType::ElseKeyword,
        TokenType::ForKeyword, TokenType::StepKeyword, TokenType::UntilKeyword, TokenType::DoKeyword,
        TokenType::GotoKeyword, TokenType::AnswerKeyword, TokenType::IntegerKeyword, TokenType::FloatingKeyword,
        TokenType::FixedKeyword, TokenType::BitsKeyword, TokenType::ArrayKeyword, TokenType::TableKeyword,
        TokenType::PresetKeyword, TokenType::OverlayKeyword, TokenType::ValueKeyword, TokenType::LocationKeyword,
        TokenType::CommonKeyword, TokenType::CommentKeyword, TokenType::DefineKeyword, TokenType::CodeKeyword
    };
    
    std::vector<coral66::Lexer::TokenType> tokens;
    tokens.reserve(result.data().size());
    std::transform(result.data().begin(), result.data().end(), std::back_inserter(tokens),
                   [](const CToken& token) { return token.getTokenType(); });
    CHECK_THAT(tokens, Catch::Matchers::Equals(correct));
}

TEST_CASE("Coral Lexing Identifiers", "[lexer]")
{
    auto result = lexes("MyIdentifier var123 _start");
    REQUIRE(result.data().size() == 3);
    CHECK(result.data()[0].getTokenType() == coral66::Lexer::TokenType::Identifier);
    CHECK(std::get<std::string>(result.data()[0].getValue()) == "MyIdentifier");
    CHECK(result.data()[1].getTokenType() == coral66::Lexer::TokenType::Identifier);
    CHECK(std::get<std::string>(result.data()[1].getValue()) == "var123");
    CHECK(result.data()[2].getTokenType() == coral66::Lexer::TokenType::Identifier);
    CHECK(std::get<std::string>(result.data()[2].getValue()) == "_start");
}

TEST_CASE("Coral Lexing Literals", "[lexer]")
{
    SECTION("Integers")
    {
        auto result = lexes("123 0 9999");
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
        auto result = lexes("3.14 0.0 .5 1.2e3");
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
        auto result = lexes("'Hello World' 'It''s me' ''");
        REQUIRE(result.data().size() == 3);
        CHECK(result.data()[0].getTokenType() == coral66::Lexer::TokenType::StringLiteral);
        CHECK(std::get<std::string>(result.data()[0].getValue()) == "Hello World");
        
        CHECK(result.data()[1].getTokenType() == coral66::Lexer::TokenType::StringLiteral);
        CHECK(std::get<std::string>(result.data()[1].getValue()) == "It's me");
        
        CHECK(result.data()[2].getTokenType() == coral66::Lexer::TokenType::StringLiteral);
        CHECK(std::get<std::string>(result.data()[2].getValue()) == "");
    }
}

TEST_CASE("Coral Lexing Operators and Punctuation", "[lexer]")
{
    using namespace coral66::Lexer;
    auto result = lexes("+ - * / = ( ) [ ] . : , ; <= >= <> := <-");
    // Note: := and <- might parse as two tokens depending on lexer implementation in Punctuation::advance
    // My previous edit to Punctuation::advance handled := as Assignment.
    // <- was not explicitly handled in Punctuation::advance (case '<' checks for '=' and '>'), so it will be LessThan followed by Minus.
    
    // Let's verify standard operators
    std::vector correct = {
        TokenType::Plus, TokenType::Minus, TokenType::Asterisk, TokenType::Division, TokenType::Equal,
        TokenType::OpenParentheses, TokenType::CloseParentheses, TokenType::OpenSquareBracket, TokenType::CloseSquareBracket,
        TokenType::Dot, TokenType::Colon, TokenType::Comma, TokenType::SemiColon,
        TokenType::LessThanOrEqual, TokenType::GreaterThanOrEqual, TokenType::NotEqual, TokenType::Assignment
    };
    
    // Check first batch
    for(size_t i = 0; i < correct.size(); ++i) {
        CHECK(result.data()[i].getTokenType() == correct[i]);
    }
    
    // Check <- which should be LessThan, Minus
    size_t offset = correct.size();
    CHECK(result.data()[offset].getTokenType() == TokenType::LessThan);
    CHECK(result.data()[offset+1].getTokenType() == TokenType::Minus);
}
