/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "lexer.hpp"

#include <llvm/Support/Format.h>

#include <charconv>

#include "lexer_support.hpp"

namespace coral66::lexer
{
bool isKeyword(const std::string_view characters)
{
    return characters == "ABSOLUTE" || characters == "AND" || characters == "ANSWER" || characters == "ARRAY"
           || characters == "BEGIN" || characters == "BIT" || characters == "BITS" || characters == "CODE"
           || characters == "COMMENT" || characters == "COMMON" || characters == "DEFINE" || characters == "DELETE"
           || characters == "DIFFER" || characters == "DO" || characters == "ELSE" || characters == "END"
           || characters == "EXTERNAL" || characters == "FIXED" || characters == "FLOATING" || characters == "FOR"
           || characters == "GOTO" || characters == "IF" || characters == "INTEGER" || characters == "LABEL"
           || characters == "LIBRARY" || characters == "LITERAL" || characters == "LOCATION" || characters == "MASK"
           || characters == "OCTAL" || characters == "OR" || characters == "OVERLAY" || characters == "PRESET"
           || characters == "PROCEDURE" || characters == "PROGRAM" || characters == "RECURSIVE"
           || characters == "SEGMENT" || characters == "STEP" || characters == "SWITCH" || characters == "TABLE"
           || characters == "THEN" || characters == "UNION" || characters == "UNSIGNED" || characters == "UNTIL"
           || characters == "VALUE" || characters == "WHILE" || characters == "WITH";
}

TokenType charactersToKeyword(const std::string_view characters)
{
    using namespace coral66::lexer;
    if (characters == "ABSOLUTE")
        return TokenType::AbsoluteKeyword;
    if (characters == "AND")
        return TokenType::AndKeyword;
    if (characters == "ANSWER")
        return TokenType::AnswerKeyword;
    if (characters == "ARRAY")
        return TokenType::ArrayKeyword;
    if (characters == "BEGIN")
        return TokenType::BeginKeyword;
    if (characters == "BIT")
        return TokenType::BitKeyword;
    if (characters == "BITS")
        return TokenType::BitsKeyword;
    if (characters == "CODE")
        return TokenType::CodeKeyword;
    if (characters == "COMMENT")
        return TokenType::CommentKeyword;
    if (characters == "COMMON")
        return TokenType::CommonKeyword;
    if (characters == "DEFINE")
        return TokenType::DefineKeyword;
    if (characters == "DELETE")
        return TokenType::DeleteKeyword;
    if (characters == "DIFFER")
        return TokenType::DifferKeyword;
    if (characters == "DO")
        return TokenType::DoKeyword;
    if (characters == "ELSE")
        return TokenType::ElseKeyword;
    if (characters == "END")
        return TokenType::EndKeyword;
    if (characters == "EXTERNAL")
        return TokenType::ExternalKeyword;
    if (characters == "FIXED")
        return TokenType::FixedKeyword;
    if (characters == "FLOATING")
        return TokenType::FloatingKeyword;
    if (characters == "FOR")
        return TokenType::ForKeyword;
    if (characters == "GOTO")
        return TokenType::GotoKeyword;
    if (characters == "IF")
        return TokenType::IfKeyword;
    if (characters == "INTEGER")
        return TokenType::IntegerKeyword;
    if (characters == "LABEL")
        return TokenType::LabelKeyword;
    if (characters == "LIBRARY")
        return TokenType::LibraryKeyword;
    if (characters == "LITERAL")
        return TokenType::LiteralKeyword;
    if (characters == "LOCATION")
        return TokenType::LocationKeyword;
    if (characters == "MASK")
        return TokenType::MaskKeyword;
    if (characters == "OCTAL")
        return TokenType::OctalKeyword;
    if (characters == "OR")
        return TokenType::OrKeyword;
    if (characters == "OVERLAY")
        return TokenType::OverlayKeyword;
    if (characters == "PRESET")
        return TokenType::PresetKeyword;
    if (characters == "PROCEDURE")
        return TokenType::ProcedureKeyword;
    if (characters == "PROGRAM")
        return TokenType::ProgramKeyword;
    if (characters == "RECURSIVE")
        return TokenType::RecursiveKeyword;
    if (characters == "SEGMENT")
        return TokenType::SegmentKeyword;
    if (characters == "STEP")
        return TokenType::StepKeyword;
    if (characters == "SWITCH")
        return TokenType::SwitchKeyword;
    if (characters == "TABLE")
        return TokenType::TableKeyword;
    if (characters == "THEN")
        return TokenType::ThenKeyword;
    if (characters == "UNION")
        return TokenType::UnionKeyword;
    if (characters == "UNSIGNED")
        return TokenType::UnsignedKeyword;
    if (characters == "UNTIL")
        return TokenType::UntilKeyword;
    if (characters == "VALUE")
        return TokenType::ValueKeyword;
    if (characters == "WHILE")
        return TokenType::WhileKeyword;
    if (characters == "WITH")
        return TokenType::WithKeyword;

    CORAL66_UNREACHABLE;
}

std::uint8_t getNumUTF8ForUTF32(std::uint32_t c)
{
    if (c <= 0x007F)
    {
        return 1;
    }
    if (c <= 0x07FF)
    {
        return 2;
    }
    if (c <= 0xFFFF)
    {
        return 3;
    }
    if (c <= 0x10FFFF)
    {
        return 4;
    }
    CORAL66_UNREACHABLE;
}

template <class Iter>
[[maybe_unused]] Iter findUTF8StartByte(Iter begin, Iter end)
{
    return std::find_if(begin, end, [](std::uint8_t const c) { return c >= 0b1100'0000 || c < 0b1000'0000; });
}

std::uint32_t hexToValue(std::string_view const value)
{
    std::uint32_t result;
    auto errors = std::from_chars(value.data(), value.data() + value.size(), result, 16);
    CORAL66_ASSERT(errors.ec == std::errc{});
    return result;
}

StateMachine Start::advance(std::uint32_t const c, Context& context)
{
    switch (c)
    {
        case '.': return Dot{};
        case '\'': return StringLiteral{};
        case '\\': return MaybeUC{};
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        {
            return PreprocessingNumber{{static_cast<char>(c)}};
        }
        case '<': return Punctuation{static_cast<int>(TokenType::LessThan)};
        case ':': return Punctuation{static_cast<int>(TokenType::Colon)};
        case '-': return Punctuation{static_cast<int>(TokenType::Minus)};
        case '>': return Punctuation{static_cast<int>(TokenType::GreaterThan)};
        case '+': return Punctuation{static_cast<int>(TokenType::Plus)};
        case '=': context.push(context.getOffset() - 1, context.getOffset(), TokenType::Equal); return *this;
        case '*': return Punctuation{static_cast<int>(TokenType::Asterisk)};
        case '/': return Punctuation{static_cast<int>(TokenType::Division)};
        case '(': context.push(context.getOffset() - 1, context.getOffset(), TokenType::OpenParentheses); return *this;
        case ')': context.push(context.getOffset() - 1, context.getOffset(), TokenType::CloseParentheses); return *this;
        case '{': context.push(context.getOffset() - 1, context.getOffset(), TokenType::OpenBrace); return *this;
        case '}': context.push(context.getOffset() - 1, context.getOffset(), TokenType::CloseBrace); return *this;
        case '[':
            context.push(context.getOffset() - 1, context.getOffset(), TokenType::OpenSquareBracket);
            return *this;
        case ']':
            context.push(context.getOffset() - 1, context.getOffset(), TokenType::CloseSquareBracket);
            return *this;
        case ';': context.push(context.getOffset() - 1, context.getOffset(), TokenType::SemiColon); return *this;
        case ',': context.push(context.getOffset() - 1, context.getOffset(), TokenType::Comma); return *this;
        case '\n': context.push(context.getOffset() - 1, context.getOffset(), TokenType::Newline); return *this;
        default:
        {
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
            {
                std::string buffer(4, ' ');
                auto* start = buffer.data();
                llvm::ConvertCodePointToUTF8(c, start);
                buffer.resize(std::distance(buffer.data(), start));
                return Text{std::move(buffer)};
            }
            if (isWhitespace(c))
            {
                context.leadingWhitespace = true;
                return *this;
            }
            if (!llvm::sys::unicode::isPrintable(c)) // NOLINT(*-narrowing-conversions)
            {
                std::string buffer = "\\U";
                buffer.reserve(10);
                llvm::raw_string_ostream ss(buffer);
                ss << llvm::format_hex_no_prefix(c, 8);
                ss.flush();
                const auto size = getNumUTF8ForUTF32(c);
                context.report(errors::lexer::NON_PRINTABLE_CHARACTER_N, context.getOffset() - size, buffer,
                               std::pair{context.getOffset() - size, context.getOffset()});
                DiagnosticCollector::addDiagnostic(errors::lexer::NON_PRINTABLE_CHARACTER_N);
            }
            else
            {
                context.push(context.getOffset() - getNumUTF8ForUTF32(c), context.getOffset(),
                             TokenType::Miscellaneous);
            }
            return *this;
        }
    }
}

std::pair<StateMachine, bool> StringLiteral::advance(char const c, Context& context)
{
    if (sawQuote)
    {
        if (c == '\'')
        {
            characters += '\'';
            sawQuote = false;
            return {std::move(*this), true};
        }
        context.push(TokenType::StringLiteral, characters);
        return {Start{}, false};
    }

    if (c == '\'')
    {
        sawQuote = true;
        return {std::move(*this), true};
    }

    characters += c;
    if ((c == '\n' || c == '\r'))
    {
        context.push(TokenType::StringLiteral, characters);
        return {Start{}, false};
    }
    return {std::move(*this), true};
}

std::pair<StateMachine, bool> L::advance(char const c, Context&) noexcept
{
    if (c == '\'')
    {
        return {StringLiteral{true, {}}, true};
    }
    if (c == '\\')
    {
        return {MaybeUC{std::make_unique<StateMachine>(L{})}, true};
    }

    return {Text{"L"}, false};
}

std::pair<StateMachine, bool> Text::advance(std::uint32_t const c, Context& context)
{
    if (c == '\\')
    {
        return {MaybeUC{std::make_unique<StateMachine>(std::move(*this))}, true};
    }

    if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') && !(c >= '0' && c <= '9'))
    {
        context.push(getNumUTF8ForUTF32(c), TokenType::Identifier, characters);
        return {Start{}, false};
    }
    characters.resize(characters.size() + 4);
    auto* start = characters.data() + characters.size() - 4;
    llvm::ConvertCodePointToUTF8(c, start);
    characters.resize(characters.size() - std::distance(start, characters.data() + characters.size()));
    return {std::move(*this), true};
}

std::pair<StateMachine, bool> PreprocessingNumber::advance(std::uint32_t const c, Context& context)
{
    constexpr std::uint8_t toLower = 32;
    if (c == '\\')
    {
        return {MaybeUC{std::make_unique<StateMachine>(std::move(*this))}, true};
    }

    if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') && !(c >= '0' && c <= '9') && c != '.'
        && (((characters.back() | toLower) != 'e' && (characters.back() | toLower) != 'p') || (c != '+' && c != '-')))
    {
        context.push(1, TokenType::PPNumber, characters);
        return {Start{}, false};
    }
    characters.resize(characters.size() + 4);
    auto* start = characters.data() + characters.size() - 4;
    llvm::ConvertCodePointToUTF8(c, start);
    characters.resize(characters.size() - std::distance(start, characters.data() + characters.size()));
    return {std::move(*this), true};
}

std::pair<StateMachine, bool> MaybeUC::advance(std::uint32_t c,           // NOLINT(*-make-member-function-const)
                                               Context& context) noexcept // NOLINT(*-make-member-function-const)
{
    if (c == 'u' || c == 'U')
    {
        // If Universal character, it's in or starting an identifier, not in a character or string literal
        if (!prevState)
        {
            return {UniversalCharacter{c == 'U'}, true};
        }
        if (std::holds_alternative<Text>(*prevState))
        {
            return {UniversalCharacter{c == 'U', coral66::get<Text>(std::move(*prevState))}, true};
        }
        if (std::holds_alternative<L>(*prevState))
        {
            return {UniversalCharacter{c == 'U', Text{"L"}}, true};
        }
        if (std::holds_alternative<PreprocessingNumber>(*prevState))
        {
            return {UniversalCharacter{c == 'U', coral66::get<PreprocessingNumber>(std::move(*prevState))}, true};
        }
        CORAL66_UNREACHABLE;
    }

    if (prevState)
    {
        context.withOffset(context.getOffset() - getNumUTF8ForUTF32(c),
                           [&] { coral66::match(*prevState, [&](auto&& value) { value.advance(' ', context); }); });
    }
    return {Start{}, false};
}

std::pair<StateMachine, bool> UniversalCharacter::advance(char const c, Context& context)
{
    if (!(c >= '0' && c <= '9') && !(c >= 'a' && c <= 'f') && !(c >= 'A' && c <= 'F'))
    {
        std::size_t result = context.view(context.tokenStartOffset, context.getOffset() - 1).rfind('\\');
        CORAL66_ASSERT(result != std::string_view::npos);
        result += context.tokenStartOffset;
        if (suspHolder)
        {
            context.withOffset(result + 1, [&]
                               { coral66::match(*suspHolder, [&](auto&& value) { value.advance(' ', context); }); });
        }
        return {Text{(big ? "U" : "u") + characters}, false};
    }

    characters.push_back(c);
    if (characters.size() != (big ? 8 : 4))
    {
        return {std::move(*this), true};
    }

    std::size_t ucStart = context.currentView().rfind('\\');
    CORAL66_ASSERT(ucStart != std::string_view::npos);
    ucStart += context.tokenStartOffset;
    auto result = universalCharacterToValue(characters, ucStart, std::pair{ucStart, context.getOffset()}, context);
    if (!result)
    {
        return {Start{}, true};
    }
    // The Universal Character is a valid character from here on
    if (suspHolder && std::holds_alternative<PreprocessingNumber>(*suspHolder))
    {
        // In a PP Number we only go through the whole mechanism to check if the universal character is invalid.
        // Otherwise, different tokens need to be generated
        auto pp = coral66::get<PreprocessingNumber>(std::move(*suspHolder));
        pp.characters += context.currentView().substr(ucStart - context.tokenStartOffset);
        return {std::move(pp), true};
    }

    // This line is needed because a text can start with a universal character. Luckily a Preprocessing number can't
    // So we only reach this line if suspHolder is either empty or contains a Text
    auto newText = suspHolder ? coral66::get<Text>(std::move(*suspHolder)) : Text{};
    newText.characters.resize(newText.characters.size() + 4);
    auto* start = newText.characters.data() + newText.characters.size() - 4;
    llvm::ConvertCodePointToUTF8(*result, start);
    newText.characters.resize(newText.characters.size()
                              - std::distance(start, newText.characters.data() + newText.characters.size()));
    return {std::move(newText), true};
}

// ReSharper disable once CppMemberFunctionMayBeConst
std::pair<StateMachine, bool> Dot::advance(char const c, Context& context) // NOLINT(*-make-member-function-const)
{
    if (c >= '0' && c <= '9')
    {
        if (dotCount == 2)
        {
            context.push(context.tokenStartOffset, context.tokenStartOffset + 1, TokenType::Dot);
        }
        return {PreprocessingNumber{"."}, false};
    }

    context.push(context.tokenStartOffset, context.tokenStartOffset + 1, TokenType::Dot);
    return {Start{}, false};
}

// ReSharper disable once CppMemberFunctionMayBeConst
std::pair<StateMachine, bool> Punctuation::advance(char const c,     // NOLINT(*-make-member-function-const)
                                                   Context& context) // NOLINT(*-make-member-function-const)
{
    switch (first)
    {
        case static_cast<int>(TokenType::Minus): context.push(1, TokenType::Minus); return {Start{}, false};
        case static_cast<int>(TokenType::GreaterThan):
            if (c == '=')
            {
                context.push(TokenType::GreaterThanOrEqual);
                return {Start{}, true};
            }
            context.push(1, TokenType::GreaterThan);
            return {Start{}, false};
        case static_cast<int>(TokenType::LessThan):
            if (c == '=')
            {
                context.push(TokenType::LessThanOrEqual);
                return {Start{}, true};
            }
            if (c == '>')
            {
                context.push(TokenType::NotEqual);
                return {Start{}, true};
            } // Coral <> for Not Equal
            context.push(1, TokenType::LessThan);
            return {Start{}, false};
        case static_cast<int>(TokenType::Plus): context.push(1, TokenType::Plus); return {Start{}, false};
        case static_cast<int>(TokenType::Asterisk): context.push(1, TokenType::Asterisk); return {Start{}, false};
        case static_cast<int>(TokenType::Division): context.push(1, TokenType::Division); return {Start{}, false};
        case static_cast<int>(TokenType::Colon):
            if (c == '=')
            {
                context.push(TokenType::Assignment);
                return {Start{}, true};
            } // :=
            context.push(1, TokenType::Colon);
            return {Start{}, false};
        default: CORAL66_UNREACHABLE;
    }
}

} // namespace coral66::lexer
