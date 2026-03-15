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
        case '\'': return StroppedKeyword{};
        case '"': return StringLiteral{false, "\""};
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
        case '(':
        {
            auto& res = context.getResult();
            auto it = res.rbegin();
            while (it != res.rend() && it->getTokenType() == TokenType::Newline)
            {
                ++it;
            }
            if (it != res.rend() && it->getTokenType() == TokenType::SemiColon)
            {
                return BracketedComment{};
            }
            context.push(context.getOffset() - 1, context.getOffset(), TokenType::OpenParentheses);
            return *this;
        }
        case ')': context.push(context.getOffset() - 1, context.getOffset(), TokenType::CloseParentheses); return *this;
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
    if (c == '"')
    {
        characters += c;
        context.push(TokenType::StringLiteral, characters);
        return {Start{}, true};
    }

    characters += c;
    if ((c == '\n' || c == '\r'))
    {
        context.report(errors::lexer::UNTERMINATED_STRING_LITERAL, context.getOffset() - 1,
                       std::pair{context.tokenStartOffset, context.getOffset()});
        DiagnosticCollector::addDiagnostic(errors::lexer::UNTERMINATED_STRING_LITERAL);
        context.push(TokenType::StringLiteral, characters);
        return {Start{}, false};
    }
    return {std::move(*this), true};
}

std::pair<StateMachine, bool> Text::advance(std::uint32_t const c, Context& context)
{
    if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') && !(c >= '0' && c <= '9'))
    {
        if (characters == "COMMENT")
        {
            return {CommentSentence{}, false};
        }
        else if (characters == "END")
        {
            context.push(getNumUTF8ForUTF32(c), TokenType::Identifier, characters);
            return {EndComment{}, false};
        }

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
    if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') && !(c >= '0' && c <= '9') && c != '.')
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

std::pair<StateMachine, bool> CommentSentence::advance(char const c, Context& context)
{
    if (c == ';')
    {
        context.push(TokenType::Identifier, "COMMENT"); // Emit comment keyword
        return {Start{}, false};                        // Let Start handle the semicolon
    }
    return {std::move(*this), true};
}

std::pair<StateMachine, bool> BracketedComment::advance(char const c, Context& context)
{
    if (c == '(')
    {
        nestCount++;
    }
    else if (c == ')')
    {
        nestCount--;
        if (nestCount == 0)
        {
            return {Start{}, true}; // Bracketed comment finished, consume ')' and discard text
        }
    }
    return {std::move(*this), true};
}

std::pair<StateMachine, bool> EndComment::advance(char const c, Context& context)
{
    if (isWhitespace(c))
    {
        if (characters.empty())
        {
            return {std::move(*this), true};
        }
        else
        {
            return {Start{}, false};
        }
    }
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
    {
        characters += c;
        return {std::move(*this), true};
    }
    if (c >= '0' && c <= '9')
    {
        if (characters.empty())
        {
            context.report(errors::lexer::MALFORMED_END_KEYWORD_SEQUENCE, context.tokenStartOffset,
                           std::pair{context.tokenStartOffset, context.getOffset()});
            DiagnosticCollector::addDiagnostic(errors::lexer::MALFORMED_END_KEYWORD_SEQUENCE);
        }
        characters += c;
        return {std::move(*this), true};
    }
    return {Start{}, false};
}

std::pair<StateMachine, bool> StroppedKeyword::advance(char const c, Context& context)
{
    if (c == '\'')
    {
        if (isKeyword(characters))
        {
            if (characters == "COMMENT")
            {
                return {CommentSentence{}, true};
            }
            if (characters == "END")
            {
                context.push(TokenType::Identifier, characters);
                return {EndComment{}, true};
            }
            context.push(TokenType::Identifier, characters); // Emit as identifier, toCTokens converts
        }
        else
        {
            context.report(errors::lexer::INVALID_STROPPED_KEYWORD_N, context.tokenStartOffset, characters,
                           std::pair{context.tokenStartOffset, context.getOffset()});
            DiagnosticCollector::addDiagnostic(errors::lexer::INVALID_STROPPED_KEYWORD_N);
            context.push(TokenType::Identifier, characters);
        }
        return {Start{}, true};
    }
    if (c == '\n' || c == '\r')
    {
        context.report(errors::lexer::UNTERMINATED_STROPPED_KEYWORD, context.tokenStartOffset,
                       std::pair{context.tokenStartOffset, context.getOffset()});
        DiagnosticCollector::addDiagnostic(errors::lexer::UNTERMINATED_STROPPED_KEYWORD);
        context.push(TokenType::Identifier, characters);
        return {Start{}, false};
    }
    characters += c;
    return {std::move(*this), true};
}

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
