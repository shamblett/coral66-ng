/*
 * Project: coral66-ng
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

using namespace coral66::lexer;
using namespace coral66::errors::lexer;
using namespace Catch::Matchers;

namespace
{

coral66::CSourceObject lex(const std::string& code)
{
    std::string buffer;
    llvm::raw_string_ostream ss(buffer);
    bool errors;
    auto result = tokenize(code, &ss, &errors);
    UNSCOPED_INFO(buffer);
    REQUIRE_FALSE(errors);
    auto tokens = toCTokens(std::move(result), &ss, &errors);
    UNSCOPED_INFO(buffer);
    REQUIRE_FALSE(errors);
    return tokens;
}

} // namespace

TEST_CASE("Coral Lexing Keywords", "[lexer]")
{
    const std::vector correct = {
        TokenType::AbsoluteKeyword, TokenType::AndKeyword,       TokenType::AnswerKeyword,  TokenType::ArrayKeyword,
        TokenType::BeginKeyword,    TokenType::BitKeyword,       TokenType::BitsKeyword,    TokenType::CodeKeyword,
        TokenType::CommonKeyword,   TokenType::DefineKeyword,    TokenType::DeleteKeyword,  TokenType::DifferKeyword,
        TokenType::DoKeyword,       TokenType::ElseKeyword,      TokenType::EndKeyword,     TokenType::ExternalKeyword,
        TokenType::FixedKeyword,    TokenType::FloatingKeyword,  TokenType::ForKeyword,     TokenType::GotoKeyword,
        TokenType::IfKeyword,       TokenType::IntegerKeyword,   TokenType::LabelKeyword,   TokenType::LibraryKeyword,
        TokenType::LiteralKeyword,  TokenType::LocationKeyword,  TokenType::MaskKeyword,    TokenType::OctalKeyword,
        TokenType::OrKeyword,       TokenType::OverlayKeyword,   TokenType::PresetKeyword,  TokenType::ProcedureKeyword,
        TokenType::ProgramKeyword,  TokenType::RecursiveKeyword, TokenType::SegmentKeyword, TokenType::StepKeyword,
        TokenType::SwitchKeyword,   TokenType::TableKeyword,     TokenType::ThenKeyword,    TokenType::UnionKeyword,
        TokenType::UnsignedKeyword, TokenType::UntilKeyword,     TokenType::ValueKeyword,   TokenType::WhileKeyword,
        TokenType::WithKeyword,     TokenType::PointerKeyword};

    SECTION("Stropped")
    {
        auto result = lex("'ABSOLUTE' 'AND' 'ANSWER' 'ARRAY' 'BEGIN' 'BIT' 'BITS' 'CODE' 'COMMENT' ; "
                          "'COMMON' 'DEFINE' 'DELETE' 'DIFFER' 'DO' 'ELSE' 'END' DUMMY 'EXTERNAL' 'FIXED' 'FLOATING' "
                          "'FOR' 'GOTO' 'IF' 'INTEGER' 'LABEL' 'LIBRARY' 'LITERAL' 'LOCATION' 'MASK' 'OCTAL' 'OR' "
                          "'OVERLAY' 'PRESET' 'PROCEDURE' 'PROGRAM' 'RECURSIVE' 'SEGMENT' 'STEP' 'SWITCH' 'TABLE' "
                          "'THEN' 'UNION' 'UNSIGNED' 'UNTIL' 'VALUE' 'WHILE' 'WITH' 'POINTER' ");

        std::vector<TokenType> tokens;
        tokens.reserve(result.data().size());
        std::ranges::transform(result.data(), std::back_inserter(tokens),
                               [](const CToken& token) { return token.getTokenType(); });
        CHECK_THAT(tokens, Catch::Matchers::Equals(correct));
    }

    SECTION("Unstropped")
    {
        auto result = lex("ABSOLUTE AND ANSWER ARRAY BEGIN BIT BITS CODE COMMENT ; "
                          "COMMON DEFINE DELETE DIFFER DO ELSE END DUMMY EXTERNAL FIXED FLOATING "
                          "FOR GOTO IF INTEGER LABEL LIBRARY LITERAL LOCATION MASK OCTAL OR "
                          "OVERLAY PRESET PROCEDURE PROGRAM RECURSIVE SEGMENT STEP SWITCH TABLE "
                          "THEN UNION UNSIGNED UNTIL VALUE WHILE WITH POINTER");

        std::vector<TokenType> tokens;
        tokens.reserve(result.data().size());
        std::ranges::transform(result.data(), std::back_inserter(tokens),
                               [](const CToken& token) { return token.getTokenType(); });
        CHECK_THAT(tokens, Catch::Matchers::Equals(correct));
    }
}

TEST_CASE("Coral Lexing Identifiers", "[lexer]")
{
    // Although Coral66 does not allow identifiers that contain uppercase characters
    // these are valid at this stage of lexing
    auto result = lex("myidentifier var123 BILLY frEd");
    CHECK(result.data().size() == 4);
    CHECK(result.data()[0].getTokenType() == coral66::lexer::TokenType::Identifier);
    CHECK(std::get<std::string>(result.data()[0].getValue()) == "myidentifier");
    CHECK(result.data()[1].getTokenType() == coral66::lexer::TokenType::Identifier);
    CHECK(std::get<std::string>(result.data()[1].getValue()) == "var123");
    CHECK(result.data()[2].getTokenType() == coral66::lexer::TokenType::Identifier);
    CHECK(std::get<std::string>(result.data()[2].getValue()) == "BILLY");
    CHECK(result.data()[3].getTokenType() == coral66::lexer::TokenType::Identifier);
    CHECK(std::get<std::string>(result.data()[3].getValue()) == "frEd");
}

TEST_CASE("Coral Lexing Literals", "[lexer]")
{
    SECTION("Integers")
    {
        auto result = lex("123 0 9999");
        CHECK(result.data().size() == 3);
        CHECK(result.data()[0].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[0].getType() == coral66::lexer::CToken::Integer);
        CHECK(std::get<llvm::APSInt>(result.data()[0].getValue()) == 123);

        CHECK(result.data()[1].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[1].getType() == coral66::lexer::CToken::Integer);
        CHECK(std::get<llvm::APSInt>(result.data()[1].getValue()) == 0);

        CHECK(result.data()[2].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[2].getType() == coral66::lexer::CToken::Integer);
        CHECK(std::get<llvm::APSInt>(result.data()[2].getValue()) == 9999);
    }

    SECTION("Hexadecimal Integers")
    {
        auto result = lex("0x1000 0XABCD 0x0");
        CHECK(result.data().size() == 3);

        CHECK(result.data()[0].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[0].getType() == coral66::lexer::CToken::Integer);
        CHECK(std::get<llvm::APSInt>(result.data()[0].getValue()) == 0x1000);

        CHECK(result.data()[1].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[1].getType() == coral66::lexer::CToken::Integer);
        CHECK(std::get<llvm::APSInt>(result.data()[1].getValue()) == 0xABCD);

        CHECK(result.data()[2].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[2].getType() == coral66::lexer::CToken::Integer);
        CHECK(std::get<llvm::APSInt>(result.data()[2].getValue()) == 0);
    }

    SECTION("Floating Point")
    {
        auto result = lex("3.14 0.0 .5 1.2");
        CHECK(result.data().size() == 4);
        CHECK(result.data()[0].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[0].getType() == coral66::lexer::CToken::Floating);
        CHECK(std::get<llvm::APFloat>(result.data()[0].getValue()) == llvm::APFloat(3.14));
        CHECK(result.data()[1].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[1].getType() == coral66::lexer::CToken::Floating);
        CHECK(std::get<llvm::APFloat>(result.data()[1].getValue()) == llvm::APFloat(0.0));
        CHECK(result.data()[2].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[2].getType() == coral66::lexer::CToken::Floating);
        CHECK(std::get<llvm::APFloat>(result.data()[2].getValue()) == llvm::APFloat(0.5));
        CHECK(result.data()[3].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[3].getType() == coral66::lexer::CToken::Floating);
        CHECK(std::get<llvm::APFloat>(result.data()[3].getValue()) == llvm::APFloat(1.2));
    }

    SECTION("Exponents")
    {
        auto result = lex("2.5'3 ' 3 10 '-2");
        CHECK(result.data().size() == 3);
        CHECK(result.data()[0].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[0].getType() == coral66::lexer::CToken::Floating);
        CHECK(std::get<llvm::APFloat>(result.data()[0].getValue()) == llvm::APFloat(2500.0));

        CHECK(result.data()[1].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[1].getType() == coral66::lexer::CToken::Floating);
        CHECK(std::get<llvm::APFloat>(result.data()[1].getValue()) == llvm::APFloat(1000.0));

        CHECK(result.data()[2].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[2].getType() == coral66::lexer::CToken::Floating);
        CHECK(std::get<llvm::APFloat>(result.data()[2].getValue()) == llvm::APFloat(0.1));
    }

    SECTION("Standard Exponents")
    {
        auto result = lex("2.5e3 1e3 10e-2 1.5E2");
        CHECK(result.data().size() == 4);

        CHECK(result.data()[0].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[0].getType() == coral66::lexer::CToken::Floating);
        CHECK(std::get<llvm::APFloat>(result.data()[0].getValue()) == llvm::APFloat(2500.0));

        CHECK(result.data()[1].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[1].getType() == coral66::lexer::CToken::Floating);
        CHECK(std::get<llvm::APFloat>(result.data()[1].getValue()) == llvm::APFloat(1000.0));

        CHECK(result.data()[2].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[2].getType() == coral66::lexer::CToken::Floating);
        CHECK(std::get<llvm::APFloat>(result.data()[2].getValue()) == llvm::APFloat(0.1));

        CHECK(result.data()[3].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[3].getType() == coral66::lexer::CToken::Floating);
        CHECK(std::get<llvm::APFloat>(result.data()[3].getValue()) == llvm::APFloat(150.0));
    }

    SECTION("Strings")
    {
        auto result = lex(R"("Hello World" "It's me" "")");
        CHECK(result.data().size() == 3);
        CHECK(result.data()[0].getTokenType() == coral66::lexer::TokenType::StringLiteral);
        CHECK(std::get<std::string>(result.data()[0].getValue()) == "Hello World");

        CHECK(result.data()[1].getTokenType() == coral66::lexer::TokenType::StringLiteral);
        CHECK(std::get<std::string>(result.data()[1].getValue()) == "It's me");

        CHECK(result.data()[2].getTokenType() == coral66::lexer::TokenType::StringLiteral);
        CHECK(std::get<std::string>(result.data()[2].getValue()).empty());
    }

    SECTION("Octal Integers")
    {
        auto result = lex("OCTAL(1234) OCTAL(0) OCTAL(77) 'OCTAL'(17)");
        CHECK(result.data().size() == 16);

        CHECK(result.data()[0].getTokenType() == coral66::lexer::TokenType::OctalKeyword);
        CHECK(result.data()[1].getTokenType() == coral66::lexer::TokenType::OpenParentheses);
        CHECK(result.data()[2].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[2].getType() == coral66::lexer::CToken::Integer);
        CHECK(std::get<llvm::APSInt>(result.data()[2].getValue()) == 01234);
        CHECK(result.data()[3].getTokenType() == coral66::lexer::TokenType::CloseParentheses);

        CHECK(result.data()[6].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[6].getType() == coral66::lexer::CToken::Integer);
        CHECK(std::get<llvm::APSInt>(result.data()[6].getValue()) == 0);

        CHECK(result.data()[10].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[10].getType() == coral66::lexer::CToken::Integer);
        CHECK(std::get<llvm::APSInt>(result.data()[10].getValue()) == 077);

        CHECK(result.data()[14].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[14].getType() == coral66::lexer::CToken::Integer);
        CHECK(std::get<llvm::APSInt>(result.data()[14].getValue()) == 017);
    }

    SECTION("Octal Points")
    {
        auto result = lex("OCTAL(1.4) OCTAL(0.1) OCTAL(12.34) OCTAL(12 . 34) 'OCTAL'(0.2)");
        CHECK(result.data().size() == 20);

        CHECK(result.data()[2].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[2].getType() == coral66::lexer::CToken::Floating);
        CHECK(std::get<llvm::APFloat>(result.data()[2].getValue()) == llvm::APFloat(1.5));

        CHECK(result.data()[6].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[6].getType() == coral66::lexer::CToken::Floating);
        CHECK(std::get<llvm::APFloat>(result.data()[6].getValue()) == llvm::APFloat(0.125));

        CHECK(result.data()[10].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[10].getType() == coral66::lexer::CToken::Floating);
        CHECK(std::get<llvm::APFloat>(result.data()[10].getValue()) == llvm::APFloat(10.4375));

        CHECK(result.data()[14].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[14].getType() == coral66::lexer::CToken::Floating);
        CHECK(std::get<llvm::APFloat>(result.data()[14].getValue()) == llvm::APFloat(10.4375));

        CHECK(result.data()[18].getTokenType() == coral66::lexer::TokenType::Literal);
        CHECK(result.data()[18].getType() == coral66::lexer::CToken::Floating);
        CHECK(std::get<llvm::APFloat>(result.data()[18].getValue()) == llvm::APFloat(0.25));
    }
}

TEST_CASE("Nested Quotes in String Literals", "[lexer]")
{
    SECTION("String with nested quotes")
    {
        auto result = lex(" \"a \"\" b\" ");
        CHECK(result.data().size() == 1);
        CHECK(result.data()[0].getTokenType() == TokenType::StringLiteral);
        CHECK(result.data()[0].getText() == "a \" b");
    }

    SECTION("Empty string")
    {
        auto result = lex(" \"\" ");
        CHECK(result.data().size() == 1);
        CHECK(result.data()[0].getTokenType() == TokenType::StringLiteral);
        CHECK(result.data()[0].getText() == "");
    }

    SECTION("String with quotes at the ends")
    {
        auto result = lex(" \"\"\"hello\"\"\" ");
        CHECK(result.data().size() == 1);
        CHECK(result.data()[0].getTokenType() == TokenType::StringLiteral);
        CHECK(result.data()[0].getText() == "\"hello\"");
    }
}

TEST_CASE("Coral Lexing Operators And Punctuation", "[lexer]")
{
    using namespace coral66::lexer;
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

TEST_CASE("Coral Lexing No Token", "[lexer]")
{
    using namespace coral66::lexer;
    auto result = lex("None");

    const std::vector correct = {TokenType::Identifier};

    for (size_t i = 0; i < correct.size(); ++i)
    {
        CHECK(result.data()[i].getTokenType() == correct[i]);
    }
}

TEST_CASE("Coral Lexing Code Strings ", "[lexer]")
{
    SECTION("Code fragment")
    {
        using namespace coral66::lexer;
        auto result = lex("PROGRAM The Coral Program; "
                          "BEGIN "
                          "  DEFINE M1 1 + 2;"
                          "  INTEGER A;"
                          "  INTEGER B; "
                          "END;");

        const std::vector correct = {
            TokenType::ProgramKeyword, TokenType::Identifier,   TokenType::Identifier,    TokenType::Identifier,
            TokenType::SemiColon,      TokenType::BeginKeyword, TokenType::DefineKeyword, TokenType::Identifier,
            TokenType::Literal,        TokenType::Plus,         TokenType::Literal,       TokenType::SemiColon,
            TokenType::IntegerKeyword, TokenType::Identifier,   TokenType::SemiColon,     TokenType::IntegerKeyword,
            TokenType::Identifier,     TokenType::SemiColon,    TokenType::EndKeyword,    TokenType::SemiColon};

        CHECK(result.data().size() == correct.size());
        for (size_t i = 0; i < correct.size(); ++i)
        {
            CHECK(result.data()[i].getTokenType() == correct[i]);
        }
    }

    SECTION("Maths Sequence")
    {
        using namespace coral66::lexer;
        auto result = lex("1 + 2 + 3 := 6");

        const std::vector correct = {TokenType::Literal, TokenType::Plus,       TokenType::Literal, TokenType::Plus,
                                     TokenType::Literal, TokenType::Assignment, TokenType::Literal};

        CHECK(result.data().size() == correct.size());
        for (size_t i = 0; i < correct.size(); ++i)
        {
            CHECK(result.data()[i].getTokenType() == correct[i]);
        }
    }

    SECTION("Maths Sequence With Identifiers")
    {
        using namespace coral66::lexer;
        auto result = lex("INTEGER A;"
                          "INTEGER B;"
                          "INTEGER C;"
                          "A := 1; B := 2; C := 3 + A + B;");

        const std::vector correct = {
            TokenType::IntegerKeyword, TokenType::Identifier, TokenType::SemiColon,      TokenType::IntegerKeyword,
            TokenType::Identifier,     TokenType::SemiColon,  TokenType::IntegerKeyword, TokenType::Identifier,
            TokenType::SemiColon,      TokenType::Identifier, TokenType::Assignment,     TokenType::Literal,
            TokenType::SemiColon,      TokenType::Identifier, TokenType::Assignment,     TokenType::Literal,
            TokenType::SemiColon,      TokenType::Identifier, TokenType::Assignment,     TokenType::Literal,
            TokenType::Plus,           TokenType::Identifier, TokenType::Plus,           TokenType::Identifier,
            TokenType::SemiColon};

        CHECK(result.data().size() == correct.size());
        for (size_t i = 0; i < correct.size(); ++i)
        {
            CHECK(result.data()[i].getTokenType() == correct[i]);
        }
    }
}

TEST_CASE("Comments", "[lexer]")
{
    SECTION("Comment Keyword")
    {
        auto result = lex("COMMENT This is a comment;");

        CHECK(result.data().empty());
    }

    SECTION("End Comment")
    {
        auto result = lex("BEGIN INTEGER a; END a");

        const std::vector correct = {TokenType::BeginKeyword, TokenType::IntegerKeyword, TokenType::Identifier,
                                     TokenType::SemiColon, TokenType::EndKeyword};

        CHECK(result.data().size() == correct.size());
        for (size_t i = 0; i < correct.size(); ++i)
        {
            CHECK(result.data()[i].getTokenType() == correct[i]);
        }
    }

    SECTION("Bracketed Comment")
    {
        auto result = lex("INTEGER a; (EOL Comment)");

        const std::vector correct = {TokenType::IntegerKeyword, TokenType::Identifier, TokenType::SemiColon};

        CHECK(result.data().size() == correct.size());
        for (size_t i = 0; i < correct.size(); ++i)
        {
            CHECK(result.data()[i].getTokenType() == correct[i]);
        }
    }

    SECTION("Nested Bracketed Comment")
    {
        auto result = lex("INTEGER a; (Outer (inner (deeply nested) comment) text) INTEGER b;");

        const std::vector correct = {TokenType::IntegerKeyword, TokenType::Identifier, TokenType::SemiColon,
                                     TokenType::IntegerKeyword, TokenType::Identifier, TokenType::SemiColon};

        CHECK(result.data().size() == correct.size());
        for (size_t i = 0; i < correct.size(); ++i)
        {
            CHECK(result.data()[i].getTokenType() == correct[i]);
        }
    }

    SECTION("Multi-Line Bracketed Comment")
    {
        auto result = lex("INTEGER a;\n"
                          "(This is a\n"
                          " multi-line (with nested\n"
                          " brackets) comment)\n"
                          "INTEGER b;");

        const std::vector correct = {TokenType::IntegerKeyword, TokenType::Identifier, TokenType::SemiColon,
                                     TokenType::IntegerKeyword, TokenType::Identifier, TokenType::SemiColon};

        CHECK(result.data().size() == correct.size());
        for (size_t i = 0; i < correct.size(); ++i)
        {
            CHECK(result.data()[i].getTokenType() == correct[i]);
        }
    }

    SECTION("Non-Printable Character")
    {
        std::string source = "\u200B";
        auto result = lex(source);
        CHECK(result.data().size() == 0);
    }
}
