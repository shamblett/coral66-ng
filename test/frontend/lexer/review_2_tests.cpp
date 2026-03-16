#include "lib/frontend/common/source_object.hpp"
#include "lib/frontend/lexer/lexer.hpp"
#include "test/vendor/catch2v3/catch_amalgamated.hpp"

using namespace coral66::lexer;

namespace
{
coral66::CSourceObject lex(const std::string& code)
{
    std::string buffer;
    llvm::raw_string_ostream ss(buffer);
    bool errors;
    auto result = coral66::lexer::tokenize(code, &ss, &errors);
    return coral66::lexer::toCTokens(std::move(result), &ss, &errors);
}
} // namespace

TEST_CASE("Nested Quotes in String Literals", "[lexer]")
{
    SECTION("String with nested quotes")
    {
        auto result = lex(" \"a \"\" b\" ");
        REQUIRE(result.data().size() == 1);
        CHECK(result.data()[0].getTokenType() == TokenType::StringLiteral);
        CHECK(result.data()[0].getText() == "a \" b");
    }

    SECTION("Empty string")
    {
        auto result = lex(" \"\" ");
        REQUIRE(result.data().size() == 1);
        CHECK(result.data()[0].getTokenType() == TokenType::StringLiteral);
        CHECK(result.data()[0].getText() == "");
    }

    SECTION("String with quotes at the ends")
    {
        auto result = lex(" \"\"\"hello\"\"\" ");
        REQUIRE(result.data().size() == 1);
        CHECK(result.data()[0].getTokenType() == TokenType::StringLiteral);
        CHECK(result.data()[0].getText() == "\"hello\"");
    }
}

TEST_CASE("Float Exponents with 10", "[lexer]")
{
    SECTION("Integer with exponent")
    {
        auto result = lex(" 1 10 3 ");
        REQUIRE(result.data().size() == 1);
        CHECK(result.data()[0].getTokenType() == TokenType::Literal);
        CHECK(result.data()[0].getType() == CToken::Floating);
        auto val = std::get<llvm::APFloat>(result.data()[0].getValue()).convertToDouble();
        CHECK_THAT(val, Catch::Matchers::WithinRel(1000.0, 1e-5));
    }

    SECTION("Float with negative exponent")
    {
        auto result = lex(" 1.5 10 -3 ");
        REQUIRE(result.data().size() == 1);
        CHECK(result.data()[0].getTokenType() == TokenType::Literal);
        CHECK(result.data()[0].getType() == CToken::Floating);
        auto val = std::get<llvm::APFloat>(result.data()[0].getValue()).convertToDouble();
        CHECK_THAT(val, Catch::Matchers::WithinRel(0.0015, 1e-5));
    }

    SECTION("Bare exponent")
    {
        auto result = lex(" 10 -3 ");
        REQUIRE(result.data().size() == 1);
        CHECK(result.data()[0].getTokenType() == TokenType::Literal);
        CHECK(result.data()[0].getType() == CToken::Floating);
        auto val = std::get<llvm::APFloat>(result.data()[0].getValue()).convertToDouble();
        CHECK_THAT(val, Catch::Matchers::WithinRel(0.001, 1e-5));
    }

    SECTION("Non-exponent usage of 10")
    {
        // 10 followed by a non-sign and non-integer shouldn't merge
        auto result = lex(" 10 + a ");
        REQUIRE(result.data().size() == 3);
        CHECK(result.data()[0].getTokenType() == TokenType::Literal);
        CHECK(result.data()[0].getType() == CToken::Integer);
        CHECK(result.data()[1].getTokenType() == TokenType::Plus);
        CHECK(result.data()[2].getTokenType() == TokenType::Identifier);
    }

    SECTION("Multiple operations")
    {
        auto result = lex(" 1.5 10 -3 + 10 -2 ");
        REQUIRE(result.data().size() == 3);
        CHECK(result.data()[0].getTokenType() == TokenType::Literal);
        CHECK(result.data()[0].getType() == CToken::Floating);
        CHECK(result.data()[1].getTokenType() == TokenType::Plus);
        CHECK(result.data()[2].getTokenType() == TokenType::Literal);
        CHECK(result.data()[2].getType() == CToken::Floating);

        auto val1 = std::get<llvm::APFloat>(result.data()[0].getValue()).convertToDouble();
        auto val2 = std::get<llvm::APFloat>(result.data()[2].getValue()).convertToDouble();
        CHECK_THAT(val1, Catch::Matchers::WithinRel(0.0015, 1e-5));
        CHECK_THAT(val2, Catch::Matchers::WithinRel(0.01, 1e-5));
    }
}
