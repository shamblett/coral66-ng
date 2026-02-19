/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "lib/frontend/lexer/lexer.hpp"

#include <llvm/Support/Format.h>

#include <array>

#include "lib/frontend/common/messages/errors.hpp"
#include "lib/frontend/common/source_object.hpp"
#include "test/common/test_config.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66::Errors::Lexer;
using namespace Catch::Matchers;

namespace
{

coral66::CSourceObject lex(const std::string& code)
{
    std::string buffer;
    llvm::raw_string_ostream ss(buffer);
    bool errors;
    auto result = coral66::Lexer::tokenize(code, &ss, &errors);
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
    auto result = lex("ABSOLUTE AND ANSWER ARRAY BEGIN BIT BITS CODE COMMENT "
                        "COMMON DEFINE DELETE DIFFER DO ELSE END EXTERNAL FIXED FLOATING "
                        "FOR GOTO IF INTEGER LABEL LIBRARY LITERAL LOCATION MASK OCTAL OR "
                        "OVERLAY PRESET PROCEDURE PROGRAM RECURSIVE SEGMENT STEP SWITCH TABLE "
                        "THEN UNION UNSIGNED UNTIL VALUE WHILE WITH");

    const std::vector correct = {
        TokenType::AbsoluteKeyword,  TokenType::AndKeyword,      TokenType::AnswerKeyword,    TokenType::ArrayKeyword,
        TokenType::BeginKeyword,     TokenType::BitKeyword,      TokenType::BitsKeyword,      TokenType::CodeKeyword,
        TokenType::CommentKeyword,   TokenType::CommonKeyword,   TokenType::DefineKeyword,    TokenType::DeleteKeyword,
        TokenType::DifferKeyword,    TokenType::DoKeyword,       TokenType::ElseKeyword,      TokenType::EndKeyword,
        TokenType::ExternalKeyword,  TokenType::FixedKeyword,    TokenType::FloatingKeyword,  TokenType::ForKeyword,
        TokenType::GotoKeyword,      TokenType::IfKeyword,       TokenType::IntegerKeyword,   TokenType::LabelKeyword,
        TokenType::LibraryKeyword,   TokenType::LiteralKeyword,  TokenType::LocationKeyword,  TokenType::MaskKeyword,
        TokenType::OctalKeyword,     TokenType::OrKeyword,       TokenType::OverlayKeyword,   TokenType::PresetKeyword,
        TokenType::ProcedureKeyword, TokenType::ProgramKeyword,  TokenType::RecursiveKeyword, TokenType::SegmentKeyword,
        TokenType::StepKeyword,      TokenType::SwitchKeyword,   TokenType::TableKeyword,     TokenType::ThenKeyword,
        TokenType::UnionKeyword,     TokenType::UnsignedKeyword, TokenType::UntilKeyword,     TokenType::ValueKeyword,
        TokenType::WhileKeyword,     TokenType::WithKeyword};

    std::vector<TokenType> tokens;
    tokens.reserve(result.data().size());
    std::ranges::transform(result.data(), std::back_inserter(tokens),
                           [](const CToken& token) { return token.getTokenType(); });
    CHECK_THAT(tokens, Catch::Matchers::Equals(correct));
}

TEST_CASE("Coral Lexing Identifiers", "[lexer]")
{
    auto result = lex("MyIdentifier var123 _start");
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
        auto result = lex("123 0 9999");
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
        auto result = lex("3.14 0.0 .5 1.2e3");
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
        auto result = lex("'Hello World' 'It''s me' ''");
        REQUIRE(result.data().size() == 3);
        CHECK(result.data()[0].getTokenType() == coral66::Lexer::TokenType::StringLiteral);
        CHECK(std::get<std::string>(result.data()[0].getValue()) == "Hello World");

        CHECK(result.data()[1].getTokenType() == coral66::Lexer::TokenType::StringLiteral);
        CHECK(std::get<std::string>(result.data()[1].getValue()) == "It's me");

        CHECK(result.data()[2].getTokenType() == coral66::Lexer::TokenType::StringLiteral);
        CHECK(std::get<std::string>(result.data()[2].getValue()).empty());
    }
}

TEST_CASE("Coral Lexing Operators and Punctuation", "[lexer]")
{
    using namespace coral66::Lexer;
    auto result = lex("+ - * / = ( ) [ ] . : , ; <= >= <> := <-");

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
