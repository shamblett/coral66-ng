/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "lexer.hpp"

#include <llvm/ADT/StringExtras.h>
#include <llvm/Support/ConvertUTF.h>
#include <llvm/Support/Error.h>

#include <algorithm>
#include <charconv>
#include <numeric>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "../common/messages/errors.hpp"
#include "lib/frontend/common/source_object.hpp"
#include "lib/support/scope_exit.hpp"
#include "lib/support/text.hpp"

using namespace coral66::Lexer;

namespace
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
    using namespace coral66::Lexer;
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

struct Start;
struct CharacterLiteral;
struct StringLiteral;
struct Text;
struct PreprocessingNumber;
struct MaybeUC;
struct UniversalCharacter;
struct Dot;
struct Punctuation;
struct LineComment;
struct BlockComment;
struct AfterInclude;
struct L;

using StateMachine = std::variant<Start, CharacterLiteral, StringLiteral, Text, PreprocessingNumber, MaybeUC,
                                  UniversalCharacter, LineComment, BlockComment, Dot, Punctuation, AfterInclude, L>;

std::pair<std::uint64_t, std::uint64_t> map(const IntervalMap& intervalMap, std::uint64_t const offset)
{
    auto result = std::lower_bound(intervalMap.begin(), intervalMap.end(), offset,
                                   [](auto&& lhs, auto&& rhs) { return std::get<0>(lhs) < rhs; });
    CORAL66_ASSERT(result != intervalMap.end());
    if (std::get<0>(*result) > offset)
    {
        --result;
    }
    const auto next = result + 1;
    if (next == intervalMap.end())
    {
        CORAL66_ASSERT(intervalMap.size() >= 2);
        return {std::get<2>(intervalMap[intervalMap.size() - 2]), std::get<2>(intervalMap[intervalMap.size() - 2])};
    }
    const auto denominator = std::get<0>(*next) - std::get<0>(*result);
    CORAL66_ASSERT(denominator != 0);
    const auto nominatorRange = std::get<2>(*result) - std::get<1>(*result);
    const auto lowBound = (offset - std::get<0>(*result)) * nominatorRange / denominator + std::get<1>(*result);
    const auto highBound = (offset + 1 - std::get<0>(*result)) * nominatorRange / denominator + std::get<1>(*result);
    return {lowBound, highBound};
}

template <class Arg>
decltype(auto) mapArgument(const IntervalMap& intervalMap, Arg&& arg)
{
    using T = std::decay_t<Arg>;
    if constexpr (std::is_integral_v<T>)
    {
        return map(intervalMap, arg).first;
    }
    else if constexpr (coral66::IsTupleLike<T>{})
    {
        static_assert(std::tuple_size_v<T> == 3 || std::tuple_size_v<T> == 2);
        if constexpr (std::tuple_size_v<T> == 2)
        {
            return std::apply([&intervalMap]<typename... T0>(T0&&... args)
                              { return std::tuple(mapArgument(intervalMap, std::forward<T0>(args))...); },
                              std::forward<Arg>(arg));
        }
        else
        {
            using T1 = std::tuple_element_t<0, T>;
            using T3 = std::tuple_element_t<2, T>;
            static_assert(std::is_base_of_v<TokenBase, std::decay_t<T3>>
                          || std::is_base_of_v<TokenBase, std::decay_t<T1>>);
            if constexpr (std::is_base_of_v<TokenBase, std::decay_t<T1>>)
            {
                return std::make_tuple(std::get<0>(arg), map(intervalMap, std::get<1>(arg)).first,
                                       map(intervalMap, std::get<2>(arg)).second);
            }
            else
            {
                return std::make_tuple(map(intervalMap, std::get<0>(arg)).first,
                                       map(intervalMap, std::get<1>(arg)).second, std::get<2>(arg));
            }
        }
    }
    else
    {
        return std::forward<Arg>(arg);
    }
}

template <class Diag, class Tuple, std::size_t... ints>
auto mapArguments(const Diag&, const IntervalMap& intervalMap, Tuple&& tuple, std::index_sequence<ints...>)
{
    return std::make_tuple(
        [&tuple, &intervalMap]<typename T0>([[maybe_unused]] T0 value)
        {
            constexpr auto i = T0::value;
            if constexpr (static_cast<bool>(Diag::constraints[i] & Diag::LocationConstraint))
            {
                return mapArgument(intervalMap, std::get<i>(tuple));
            }
            else
            {
                return std::get<i>(tuple);
            }
        }(std::integral_constant<std::size_t, ints>{})...);
}

class Context : coral66::source_interface
{
    llvm::raw_ostream* m_reporter;
    std::vector<PPToken> m_result;
    std::string_view m_sourceSpace;
    std::string_view m_characterSpace;
    std::uint64_t& m_offset;
    std::vector<std::uint64_t> m_lineStarts;
    const IntervalMap& m_characterToSourceSpace;
    bool m_errorsOccurred = false;
    coral66::Source::File m_fakeFile;

    [[nodiscard]] std::uint64_t getLineNumber(std::uint32_t, const std::uint64_t offset) const noexcept override
    {
        auto result = std::ranges::lower_bound(m_lineStarts.begin(), m_lineStarts.end(), offset);
        CORAL66_ASSERT(result != m_lineStarts.end());
        return std::distance(m_lineStarts.begin(), result) + (*result == offset ? 1 : 0);
    }

    [[nodiscard]] std::uint64_t getLineStartOffset(std::uint32_t, const std::uint64_t line) const noexcept override
    {
        CORAL66_ASSERT(line - 1 < m_lineStarts.size());
        return m_lineStarts[line - 1];
    }

    [[nodiscard]] std::uint64_t getLineEndOffset(std::uint32_t, const std::uint64_t line) const noexcept override
    {
        CORAL66_ASSERT(line - 1 < m_lineStarts.size());
        if (line != m_lineStarts.size())
        {
            return m_lineStarts[line] - 1;
        }
        return m_sourceSpace.size();
    }

    [[nodiscard]] std::span<const coral66::Source::File> getFiles() const noexcept override
    {
        return {&m_fakeFile, &m_fakeFile + 1};
    }

    [[nodiscard]] std::span<const coral66::Source::PPRecord> getSubstitutions() const noexcept override
    {
        return {};
    }

public:
    std::uint64_t tokenStartOffset;
    bool leadingWhitespace = false;

    Context(std::string_view const sourceSpace, const IntervalMap& characterToSourceSpace,
            std::string_view const characterSpace, std::uint64_t& offset, std::vector<std::uint64_t> lineStarts,
            llvm::raw_ostream* reporter, std::string_view const path) noexcept
        : m_reporter(reporter),
          m_sourceSpace(sourceSpace),
          m_characterSpace(characterSpace),
          m_offset(offset),
          m_lineStarts(std::move(lineStarts)),
          m_characterToSourceSpace(characterToSourceSpace),
          m_fakeFile{coral66::to_string(path), coral66::to_string(sourceSpace), {}, false},
          tokenStartOffset(0)
    {
    }

    template <class Diag, class Location, class... Args>
    void report(const Diag& diag, Location&& location, Args&&... args)
    {
        if (diag.getSeverity() == coral66::Severity::Error)
        {
            m_errorsOccurred = true;
        }
        if (m_reporter)
        {
            std::apply(
                [&]<typename... T0>(T0&&... args1)
                {
                    if constexpr (std::is_integral_v<std::decay_t<Location>>)
                    {
                        *m_reporter << diag.args(map(m_characterToSourceSpace, location).first, *this,
                                                 std::forward<T0>(args1)...);
                        return;
                    }
                    if constexpr (coral66::IsTupleLike<std::decay_t<Location>>{})
                    {
                        if constexpr (std::tuple_size_v<std::decay_t<Location>> == 2)
                        {
                            using T1 = std::decay_t<std::tuple_element_t<0, std::decay_t<Location>>>;
                            using T2 = std::decay_t<std::tuple_element_t<1, std::decay_t<Location>>>;
                            if constexpr (std::is_integral_v<T1> && std::is_integral_v<T2>)
                            {
                                *m_reporter << diag.args(map(std::get<0>(location), map(std::get<1>(location))), *this,
                                                         std::forward<T0>(args1)...);
                            }
                        }
                    }
                    *m_reporter << diag.args(mapArgument(m_characterToSourceSpace, location), *this,
                                             std::forward<T0>(args1)...);
                },
                mapArguments(diag, m_characterToSourceSpace, std::forward_as_tuple(std::forward<Args>(args)...),
                             std::index_sequence_for<Args...>{}));
        }
    }

    [[nodiscard]] std::uint64_t getOffset() const
    {
        return m_offset;
    }

    template <class F>
    void withOffset(std::uint64_t const offset, F&& f)
    {
        auto exit = coral66::ScopeExit([offset = m_offset, this]() { m_offset = offset; });
        m_offset = offset;
        std::forward<F>(f)();
    }

    [[nodiscard]] std::vector<coral66::Lexer::PPToken>& getResult() noexcept
    {
        return m_result;
    }

    void push(std::uint64_t start, std::uint64_t const end, TokenType tokenType, const std::string& value = {})
    {
        auto sourceStart = map(m_characterToSourceSpace, start).first;
        auto sourceEnd = map(m_characterToSourceSpace, end - 1).second;
        auto& newToken =
            m_result.emplace_back(tokenType, sourceStart, sourceEnd - sourceStart, start, end - start, 0, 0, value);
        newToken.setLeadingWhitespace(leadingWhitespace);
        leadingWhitespace = false;
    }

    void push(const TokenType tokenType, const std::string& value = {})
    {
        push(tokenStartOffset, m_offset, tokenType, value);
    }

    void push(const std::uint64_t diff, const TokenType tokenType, const std::string& value = {})
    {
        push(tokenStartOffset, m_offset - diff, tokenType, value);
    }

    [[nodiscard]] std::string_view view(std::uint64_t const startOffset, std::uint64_t const endOffset) const
    {
        return m_characterSpace.substr(startOffset, endOffset - startOffset);
    }

    [[nodiscard]] std::string_view currentView() const
    {
        return view(tokenStartOffset, m_offset);
    }

    [[nodiscard]] bool errorsOccurred() const
    {
        return m_errorsOccurred;
    }
};

std::uint32_t hexToValue(std::string_view const value)
{
    std::uint32_t result;
    auto errors = std::from_chars(value.data(), value.data() + value.size(), result, 16);
    CORAL66_ASSERT(errors.ec == std::errc{});
    return result;
}

constexpr auto UTF32_MAX = 0x10FFFF;

/**
 * Callee responsible for right format
 * @param value string that either consist of 4 or 8 hex digits
 * @param location the location
 * @param arg arguments
 * @param context the context
 * @return Unicode value
 */
template <class T, class PosArg>
std::optional<std::uint32_t> universalCharacterToValue(std::string_view const value, std::uint64_t const location,
                                                       PosArg&& arg, T& context)
{
    CORAL66_ASSERT(value.size() == 4 || value.size() == 8);
    auto result = hexToValue(value);
    if (result < 0xA0)
    {
        if (result != '$' && result != '@' && result != '`')
        {
            context.report(coral66::Errors::Lexer::INVALID_UC_VALUE_MUSTNT_BE_LESS_THAN_A0, location,
                           std::forward<PosArg>(arg));
            return {};
        }
    }
    else if (result > UTF32_MAX)
    {
        context.report(coral66::Errors::Lexer::INVALID_UC_VALUE_MUST_FIT_IN_UTF32, location, std::forward<PosArg>(arg));
        return {};
    }
    else if ((result >= 0xD800 && result <= 0xDFFF))
    {
        context.report(coral66::Errors::Lexer::INVALID_UC_VALUE_MUSTNT_BE_IN_RANGE, location,
                       std::forward<PosArg>(arg));
        return {};
    }
    return result;
}

struct Start final
{
    StateMachine advance(std::uint32_t c, Context& context);
};

struct CharacterLiteral final
{
    [[maybe_unused]] bool wide = false;
    std::string characters;

    std::pair<StateMachine, bool> advance(char c, Context& context);
};

struct StringLiteral final
{
    [[maybe_unused]] bool wide = false;
    std::string characters;
    bool sawQuote = false;

    std::pair<StateMachine, bool> advance(char c, Context& context);
};

struct Text final
{
    std::string characters;

    std::pair<StateMachine, bool> advance(std::uint32_t c, Context& context);
};

struct PreprocessingNumber final
{
    std::string characters{};

    std::pair<StateMachine, bool> advance(std::uint32_t c, Context& context);
};

struct MaybeUC final
{
    std::unique_ptr<StateMachine> prevState{};
    [[maybe_unused]] bool error = false;

    std::pair<StateMachine, bool> advance(std::uint32_t c, Context& context) noexcept;
};

struct UniversalCharacter final
{
    bool big;
    std::optional<std::variant<Text, PreprocessingNumber>> suspHolder{};
    std::string characters{};

    std::pair<StateMachine, bool> advance(char c, Context& context);
};

struct LineComment final
{
    std::pair<StateMachine, bool> advance(char c, Context& context) noexcept;
};

struct BlockComment final
{
    std::optional<char> lastChar{};

    StateMachine advance(char c, Context& context) noexcept;
};

struct Dot final
{
    std::uint8_t dotCount = 1;

    std::pair<StateMachine, bool> advance(char c, Context& context);
};

struct Punctuation final
{
    std::underlying_type_t<TokenType> first;

private:
    [[maybe_unused]] static constexpr std::underlying_type_t<TokenType> POUND_PERCENT =
        std::numeric_limits<std::underlying_type_t<TokenType>>::max() - 1;

public:
    std::pair<StateMachine, bool> advance(char c, Context& context);
};

struct AfterInclude final
{
    char delimiter;
    std::string characters{};

    std::pair<StateMachine, bool> advance(char c, Context& context);
};

struct L final
{
    static std::pair<StateMachine, bool> advance(char c, Context& context) noexcept;
};

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
            if (coral66::isWhitespace(c))
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
                auto size = getNumUTF8ForUTF32(c);
                context.report(coral66::Errors::Lexer::NON_PRINTABLE_CHARACTER_N, context.getOffset() - size, buffer,
                               std::pair{context.getOffset() - size, context.getOffset()});
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

std::pair<StateMachine, bool> CharacterLiteral::advance(char const c, Context& context)
{
    if (c == '\''
        && (std::find_if_not(characters.rbegin(), characters.rend(), [](const char c) { return c == '\\'; })
            - characters.rbegin())
                   % 2
               == 0)
    {
        context.push(TokenType::Literal, characters);
        return {Start{}, true};
    }
    characters += c;
    if ((c == '\n' || c == '\r'))
    {
        context.push(TokenType::Literal, characters);
        return {Start{}, false};
    }
    return {std::move(*this), true};
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
        // In an PP Number we only go through the whole mechanism to check if the universal character is invalid.
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
        case static_cast<int>(TokenType::Division):
            // Keep comment handling?
            if (c == '/')
                return {LineComment{}, true};
            if (c == '*')
                return {BlockComment{}, true};
            context.push(1, TokenType::Division);
            return {Start{}, false};
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

std::pair<StateMachine, bool> AfterInclude::advance(char const c, Context& context)
{
    if (c != delimiter && c != '\n')
    {
        characters += c;
        return {std::move(*this), true};
    }

    if (c != '\n')
    {
        context.push(TokenType::StringLiteral, characters);
        return {Start{}, true};
    }

    // c is '\n' at this point

    if (delimiter == '"')
    {
        // If the delimiter is an " it means that the whole token would otherwise be interpreted as a string literal
        // if there wasn't an include directive in front. Therefore, no macro expansion could make this a valid include
        // directive

        context.report(coral66::Errors::Lexer::UNTERMINATED_INCLUDE_DIRECTIVE, context.tokenStartOffset,
                       std::pair{context.tokenStartOffset, context.getOffset() - 1});

        // if the delimiter is a < however then subsequent tokens will later be retokenized in the preprocessor
        // which might form a valid include after macro substitution
    }
    context.push(1, TokenType::StringLiteral, characters);
    return {Start{}, false};
}

std::pair<StateMachine, bool> LineComment::advance(char const c, Context&) noexcept
{
    if (c == '\n')
    {
        return {Start{}, false};
    }
    return {*this, true};
}

StateMachine BlockComment::advance(char c, Context& context) noexcept
{
    if (lastChar && *lastChar == '*' && c == '/')
    {
        context.leadingWhitespace = true;
        return Start{};
    }
    lastChar = c;
    return *this;
}

template <class T>
struct [[maybe_unused]] FirstArgOfMethod;

template <class R, class C, class U, class... Args>
struct [[maybe_unused]] FirstArgOfMethod<R (C::*)(U, Args...)>
{
    using Type = U;
};

template <class R, class C, class U, class... Args>
struct [[maybe_unused]] FirstArgOfMethod<R (C::*)(U, Args...) noexcept>
{
    using Type = U;
};

template <class R, class U, class... Args>
struct [[maybe_unused]] FirstArgOfMethod<R (*)(U, Args...)>
{
    using Type = U;
};

template <class R, class U, class... Args>
struct [[maybe_unused]] FirstArgOfMethod<R (*)(U, Args...) noexcept>
{
    using Type = U;
};
} // namespace

coral66::PPSourceObject coral66::Lexer::tokenize(std::string source, llvm::raw_ostream* reporter, bool* errorsOccurred,
                                                 std::string_view const sourcePath)
{
    if (errorsOccurred)
    {
        *errorsOccurred = false;
    }

    constexpr static std::string_view UTF8_BOM = "\xEF\xBB\xBF";
    if (source.size() >= 3 && source.substr(0, 3) == UTF8_BOM)
    {
        source = source.substr(3);
    }

    {
        std::string::size_type pos = 0;
        while ((pos = source.find("\r\n", pos)) != std::string::npos)
        {
            source.erase(pos, 1);
        }
    }
    source.shrink_to_fit();

    StateMachine stateMachine;
    std::uint64_t offset = 0;
    std::vector<std::uint64_t> starts = {0};
    for (auto iter : source)
    {
        offset++;
        if (iter == '\n')
        {
            starts.push_back(offset);
        }
    }
    starts.push_back(offset + 1);
    starts.shrink_to_fit();
    offset = 0;

    std::string charactersSpace;
    IntervalMap characterToSourceSpace;
    charactersSpace.reserve(source.size());
    {
        auto stringView = std::string_view(source);
        std::size_t curr = 0;
        while (!stringView.empty())
        {
            for (; curr < stringView.size() && stringView[curr] != '?' && stringView[curr] != '\\'; curr++)
                ;
            if (curr == stringView.size())
            {
                break;
            }
            if (curr + 1 == stringView.size() || (stringView[curr] == '\\' && stringView[curr + 1] != '\n')
                || (stringView[curr] == '?'
                    && (curr + 1 == stringView.size() || stringView[curr + 1] != '?' || curr + 2 == stringView.size()
                        || (stringView[curr + 2] != '=' && stringView[curr + 2] != '<' && stringView[curr + 2] != '('
                            && stringView[curr + 2] != '!' && stringView[curr + 2] != '/' && stringView[curr + 2] != '>'
                            && stringView[curr + 2] != ')' && stringView[curr + 2] != '-'
                            && stringView[curr + 2] != '\'' && stringView[curr + 2] != '/'))))
            {
                curr++;
                continue;
            }
            bool trigraph = false;
            auto view = stringView.substr(curr);
            if (view[0] == '\\')
            {
                view = view.substr(0, 2);
            }
            else if (view.substr(0, std::min<std::size_t>(view.size(), 4)) == "?\?/\n")
            {
                view = view.substr(0, 4);
            }
            else
            {
                trigraph = true;
                view = view.substr(0, 3);
            }
            auto pos = stringView.data() + curr - source.data();
            auto prefix = stringView.substr(0, curr);
            if (!prefix.empty())
            {
                characterToSourceSpace.emplace_back(charactersSpace.size(), pos - prefix.size(), pos);
            }
            charactersSpace += prefix;
            if (trigraph)
            {
                // If the first and only group didn't match it's a trigraph
                // While backslashes are removed and therefore not replaced we need to now replace the
                // trigraph
                constexpr std::array<std::pair<std::string_view, char>, 9> mapping = {{{"?\?=", '#'},
                                                                                       {"?\?(", '['},
                                                                                       {"?\?/", '\\'},
                                                                                       {"?\?)", ']'},
                                                                                       {"?\?'", '^'},
                                                                                       {"?\?<", '{'},
                                                                                       {"?\?!", '|'},
                                                                                       {"?\?>", '}'},
                                                                                       {"?\?-", '~'}}};
                const auto result =
                    std::ranges::find_if(mapping, coral66::compose(coral66::bind_front(std::equal_to<>{}, view),
                                                                   &decltype(mapping)::value_type::first));
                CORAL66_ASSERT(result != mapping.end());
                characterToSourceSpace.emplace_back(charactersSpace.size(), pos, pos + view.size());
                charactersSpace += result->second;
            }
            stringView.remove_prefix(prefix.size() + view.size());
            curr = 0;
        }
        if (!stringView.empty())
        {
            characterToSourceSpace.emplace_back(charactersSpace.size(), source.size() - stringView.size(),
                                                source.size());
            charactersSpace += stringView;
        }
        characterToSourceSpace.emplace_back(charactersSpace.size(), 0, 0);
    }
    charactersSpace.shrink_to_fit();

    Context context(source, characterToSourceSpace, charactersSpace, offset, starts, reporter, sourcePath);
    const auto* end = charactersSpace.data() + charactersSpace.size();
    for (const auto* iter = charactersSpace.data(); iter != end;)
    {
        std::uint64_t step = 1;
        std::uint64_t prevOffset = offset;
        auto visitor = [iter, &step, &stateMachine, &context, &offset, end, prevOffset]<typename T0>(T0&& state) -> bool
        {
            using T = std::decay_t<T0>;
            constexpr bool needsCodepoint =
                std::is_same_v<std::uint32_t, typename FirstArgOfMethod<decltype(&T::advance)>::Type>;
            std::conditional_t<needsCodepoint, std::uint32_t, char> c{};
            if constexpr (needsCodepoint)
            {
                llvm::UTF32 result;
                const auto* start = iter;
                if (llvm::convertUTF8Sequence(reinterpret_cast<const llvm::UTF8**>(&start),
                                              reinterpret_cast<const llvm::UTF8*>(end), &result, llvm::strictConversion)
                    != llvm::conversionOK)
                {
                    step = getNumBytesForUTF8(start, end);
                    context.report(Errors::Lexer::INVALID_UTF8_SEQUENCE, offset, std::pair{offset, offset + step});
                    return false;
                }
                c = result;
                step = std::distance(iter, start);
            }
            else
            {
                c = *iter;
            }
            offset = prevOffset + step;
            if constexpr (std::is_convertible_v<decltype(state.advance(c, context)), bool>)
            {
                return !state.advance(c, context);
            }
            else if constexpr (std::is_same_v<StateMachine, decltype(state.advance(c, context))>)
            {
                stateMachine = state.advance(c, context);
                if constexpr (std::is_same_v<std::decay_t<T0>, Start>)
                {
                    if (!std::holds_alternative<Start>(stateMachine))
                    {
                        context.tokenStartOffset = offset - step;
                    }
                }
                return false;
            }
            else if constexpr (std::is_void_v<decltype(state.advance(c, context))>)
            {
                state.advance(c, context);
                return false;
            }
            else
            {
                bool proceed;
                auto&& [lhs, rhs] = state.advance(c, context);
                if constexpr (std::is_same_v<std::decay_t<decltype(lhs)>, bool>)
                {
                    stateMachine = std::move(rhs);
                    proceed = lhs;
                }
                else
                {
                    stateMachine = std::move(lhs);
                    proceed = rhs;
                }
                if constexpr (std::is_same_v<std::decay_t<T0>, Start>)
                {
                    if (!std::holds_alternative<Start>(stateMachine))
                    {
                        context.tokenStartOffset = offset - step;
                    }
                }
                return !proceed;
            }
        };
        while (coral66::match(stateMachine, visitor))
        {
            offset = prevOffset;
        }
        offset = prevOffset + step;
        iter += step;
    }
    bool needNewline = coral66::match(
        stateMachine,
        [&context](BlockComment&)
        {
            context.report(Errors::Lexer::UNTERMINATED_BLOCK_COMMENT, context.tokenStartOffset,
                           std::pair{context.tokenStartOffset, context.getOffset()});
            return false;
        },
        [&context](const StringLiteral& s)
        {
            if (s.sawQuote)
            {
                context.push(TokenType::StringLiteral, s.characters);
                return false;
            }
            context.report(Errors::Lexer::UNTERMINATED_STRING_LITERAL, context.tokenStartOffset,
                           std::pair{context.tokenStartOffset, context.getOffset()});
            return false;
        },
        [&context](CharacterLiteral&)
        {
            context.report(Errors::Lexer::UNTERMINATED_CHARACTER_LITERAL, context.tokenStartOffset,
                           std::pair{context.tokenStartOffset, context.getOffset()});
            return false;
        },
        [&context](AfterInclude&)
        {
            context.report(Errors::Lexer::UNTERMINATED_INCLUDE_DIRECTIVE, context.tokenStartOffset,
                           std::pair{context.tokenStartOffset, context.getOffset()});
            return false;
        },
        [](Start&) { return false; }, [](LineComment&) { return false; }, [](auto&&) { return true; });
    if (needNewline)
    {
        offset++;
        characterToSourceSpace.back() = {charactersSpace.size(), source.size(), source.size()};
        characterToSourceSpace.emplace_back(charactersSpace.size() + 1, 0, 0);
        auto visitor = [offset, &context, &stateMachine]<typename T0>(T0&& state) -> bool
        {
            if constexpr (std::is_same_v<std::decay_t<T0>, Start>)
            {
                return false;
            }
            std::uint32_t c = '\n';
            if constexpr (std::is_convertible_v<decltype(state.advance(c, context)), bool>)
            {
                return !state.advance(c, context);
            }
            else if constexpr (std::is_same_v<StateMachine, decltype(state.advance(c, context))>)
            {
                stateMachine = state.advance(c, context);
                if constexpr (std::is_same_v<std::decay_t<T0>, Start>)
                {
                    if (!std::holds_alternative<Start>(stateMachine))
                    {
                        context.tokenStartOffset = offset - 1;
                    }
                }
                return false;
            }
            else if constexpr (std::is_void_v<decltype(state.advance(c, context))>)
            {
                state.advance(c, context);
                return false;
            }
            else
            {
                bool proceed;
                auto&& [lhs, rhs] = state.advance(c, context);
                if constexpr (std::is_same_v<std::decay_t<decltype(lhs)>, bool>)
                {
                    stateMachine = std::move(rhs);
                    proceed = lhs;
                }
                else
                {
                    stateMachine = std::move(lhs);
                    proceed = rhs;
                }
                if constexpr (std::is_same_v<std::decay_t<T0>, Start>)
                {
                    if (!std::holds_alternative<Start>(stateMachine))
                    {
                        context.tokenStartOffset = offset - 1;
                    }
                }
                return !proceed;
            }
        };
        while (coral66::match(stateMachine, visitor))
            ;
    }
    characterToSourceSpace.shrink_to_fit();

    if (errorsOccurred)
    {
        *errorsOccurred = context.errorsOccurred();
    }

    return PPSourceObject(std::move(context.getResult()),
                          {Source::File{to_string(sourcePath), std::move(source), std::move(starts), false}}, {},
                          {{std::move(characterToSourceSpace)}});
}

[[nodiscard]] std::string_view coral66::Lexer::TokenBase::getRepresentation(const source_interface& sourceObject) const
{
    return std::string_view(sourceObject.getFiles()[static_cast<std::uint64_t>(m_fileID)].source)
        .substr(m_offset, m_length);
}

[[nodiscard]] std::uint64_t coral66::Lexer::TokenBase::getLine(const source_interface& sourceObject) const noexcept
{
    return sourceObject.getLineNumber(m_fileID, m_offset);
}

[[nodiscard]] std::uint64_t coral66::Lexer::TokenBase::getColumn(const source_interface& sourceObject) const noexcept
{
    auto line = sourceObject.getLineNumber(m_fileID, m_offset);
    return m_offset - sourceObject.getLineStartOffset(m_fileID, line) + 1;
}

namespace
{
struct ConversionContext
{
    llvm::raw_ostream* reporter;
    const coral66::PPSourceInterface& sourceInterface;
    const PPToken& token;
    bool* errorsOccurred;

    template <class Diag, class Location, class... Args>
    void report(const Diag& diag, Location&& location, Args&&... args) const
    {
        if (diag.getSeverity() == coral66::Severity::Error && errorsOccurred)
        {
            *errorsOccurred = true;
        }
        if (reporter)
        {
            std::apply(
                [&]<typename... T0>(T0&&... args1)
                {
                    if constexpr (std::is_integral_v<std::decay_t<Location>>)
                    {
                        *reporter << diag.args(
                            std::forward_as_tuple(
                                token, map(sourceInterface.getIntervalMaps()[token.getFileId()], location).first),
                            sourceInterface, std::forward<T0>(args1)...);
                        return;
                    }
                    if constexpr (coral66::IsTupleLike<std::decay_t<Location>>{})
                    {
                        if constexpr (std::tuple_size_v<std::decay_t<Location>> == 2)
                        {
                            using T1 = std::decay_t<std::tuple_element_t<0, std::decay_t<Location>>>;
                            using T2 = std::decay_t<std::tuple_element_t<1, std::decay_t<Location>>>;
                            if constexpr (std::is_integral_v<T1> && std::is_integral_v<T2>)
                            {
                                *reporter << diag.args(std::forward_as_tuple(token, map(std::get<0>(location)),
                                                                             map(std::get<1>(location))),
                                                       sourceInterface, std::forward<T0>(args1)...);
                            }
                        }
                    }
                    *reporter << diag.args(mapArgument(sourceInterface.getIntervalMaps()[token.getFileId()], location),
                                           sourceInterface, std::forward<T0>(args1)...);
                },
                mapArguments(diag, sourceInterface.getIntervalMaps()[token.getFileId()],
                             std::forward_as_tuple(std::forward<Args>(args)...), std::index_sequence_for<Args...>{}));
        }
    }
};

std::optional<std::uint32_t> escapeCharToValue(char const escape, std::uint64_t backslash, ConversionContext& context)
{
    switch (escape)
    {
        case '\'': return '\'';
        case '"': return '"';
        case 'b': return '\b';
        case 't': return '\t';
        case '\\': return '\\';
        case 'f': return '\f';
        case 'v': return '\v';
        case '?': return '\?';
        case 'n': return '\n';
        case 'a': return '\a';
        case 'r': return '\r';
        case ' ':
        {
            context.report(coral66::Errors::Lexer::EXPECTED_CHARACTER_AFTER_BACKSLASH, backslash + 1,
                           std::forward_as_tuple(context.token, backslash, backslash + 2));

            return {};
        }
        default:
        {
            context.report(coral66::Errors::Lexer::INVALID_ESCAPE_SEQUENCE_N, backslash + 1, std::string("\\") + escape,
                           std::forward_as_tuple(context.token, backslash, backslash + 1));

            return {};
        }
    }
}

std::uint32_t octalToValue(std::string_view const value)
{
    std::uint32_t result;
    auto errors = std::from_chars(value.data(), value.data() + value.size(), result, 8);
    CORAL66_ASSERT(errors.ec == std::errc{});
    return result;
}

enum class Literal
{
    CharLiteral,
    StringLiteral,
};

std::pair<std::vector<llvm::UTF32>, bool> processCharacters(std::string_view const characters,
                                                            ConversionContext& context, bool wide,
                                                            Literal const literalType)
{
    constexpr std::uint32_t largestCharacter = []() -> std::uint32_t
    {
        std::uint8_t size = 1;
        return 0xFFFFFFFFu >> (32 - 8 * size);
    }();
    std::vector<llvm::UTF32> result;
    result.resize(characters.size());
    auto* resultStart = result.data();
    auto* resultEnd = result.data() + result.size();

    const auto* end = characters.data() + characters.size();
    bool errorOccurred = false;
    for (const auto* iter = characters.data(); iter != end;)
    {
        auto offset = context.token.getCharSpaceOffset() + (wide ? 2 : 1) + (iter - characters.data());
        if (*iter == '\n')
        {
            if (literalType == Literal::CharLiteral)
            {
                context.report(coral66::Errors::Lexer::NEWLINE_IN_CHARACTER_LITERAL_USE_BACKLASH_N, offset,
                               std::forward_as_tuple(context.token, offset));
            }
            else
            {
                context.report(coral66::Errors::Lexer::NEWLINE_IN_STRING_LITERAL_USE_BACKLASH_N, offset,
                               std::forward_as_tuple(context.token, offset));
            }
            iter++;
            errorOccurred = true;
            continue;
        }
        if (*iter != '\\')
        {
            const auto* start = iter;
            iter = std::find_if(iter, end, [](char c) { return c == '\\' || c == '\n'; });

            auto res = llvm::ConvertUTF8toUTF32(reinterpret_cast<const llvm::UTF8**>(&start),
                                                reinterpret_cast<const llvm::UTF8*>(iter), &resultStart, resultEnd,
                                                llvm::strictConversion);
            if (res != llvm::conversionOK)
            {
                auto invalidStart = context.token.getCharSpaceOffset() + (wide ? 2 : 1) + (start - characters.data());
                auto invalidEnd = invalidStart + coral66::getNumBytesForUTF8(start, end) - 1;
                context.report(coral66::Errors::Lexer::INVALID_UTF8_SEQUENCE, invalidStart,
                               std::make_tuple(context.token, invalidStart, invalidEnd));
                errorOccurred = true;
            }
            continue;
        }
        // Normally we could assume that if *iter == '\\' that iter + 1 != end. That is because if *iter == '\\' and
        // iter + 1 == end the last character would be '\\' and following that '\'' or '\"'.
        // Therefore, the character literal wouldn't have ended, and we wouldn't be here.

        // But if we didn't go through a preprocessor a #include followed by a string literal could be present and
        // therefore contain such a character. For now, we will just break at such an occurrence but in the future
        // we might simply want to disable the special handling of string literals after #include for API consumers
        // that do not want to preprocess
        if (iter + 1 == end)
        {
            break;
        }

        if (iter[1] == 'u' || iter[1] == 'U')
        {
            const bool big = iter[1] == 'U';
            iter += 2;
            if (iter == end
                || (!(*iter >= '0' && *iter <= '9') && !(*iter >= 'a' && *iter <= 'f')
                    && !(*iter >= 'A' && *iter <= 'F')))
            {
                // First character followed after \u or \U is not a hex digit, or it's the end of string
                // Let's assume the user thought \u might be an escape character
                auto start = context.token.getCharSpaceOffset() + (wide ? 2 : 1) + (iter - characters.data() - 2);
                context.report(coral66::Errors::Lexer::INVALID_ESCAPE_SEQUENCE_N, start,
                               std::string_view(big ? "\\U" : "\\u"),
                               std::forward_as_tuple(context.token, start, start + 1));
                errorOccurred = true;
                continue;
            }

            const auto* hexStart = iter;
            const auto* hexEnd = std::find_if(
                hexStart, hexStart + std::min<std::size_t>(std::distance(hexStart, end), big ? 8 : 4),
                [](char c) { return !(c >= '0' && c <= '9') && !(c >= 'a' && c <= 'f') && !(c >= 'A' && c <= 'F'); });
            if (std::distance(hexStart, hexEnd) != (big ? 8 : 4))
            {
                auto start = context.token.getCharSpaceOffset() + (wide ? 2 : 1) + (iter - characters.data()) - 2;
                context.report(
                    coral66::Errors::Lexer::INVALID_UC_EXPECTED_N_MORE_DIGITS, start,
                    (big ? 8 : 4) - std::distance(hexStart, hexEnd),
                    std::forward_as_tuple(context.token, start, start + std::distance(hexStart, hexEnd) + 1));
                errorOccurred = true;
                iter = hexEnd;
                continue;
            }
            auto uc = universalCharacterToValue(
                {hexStart, static_cast<std::size_t>(hexEnd - hexStart)}, offset,
                std::forward_as_tuple(context.token, offset, offset + 1 + (big ? 8 : 4)), context);
            if (uc)
            {
                *resultStart = *uc;
                resultStart++;
            }
            else
            {
                errorOccurred = true;
            }
            iter = hexEnd;
            continue;
        }
        if (iter[1] == 'x')
        {
            iter += 2;
            const auto* lastHex =
                std::find_if(iter, end, [](char c)
                             { return !(c >= '0' && c <= '9') && !(c >= 'a' && c <= 'f') && !(c >= 'A' && c <= 'F'); });
            if (lastHex == iter)
            {
                auto start = context.token.getCharSpaceOffset() + (wide ? 2 : 1) + (iter - characters.data() - 2);
                context.report(coral66::Errors::Lexer::AT_LEAST_ONE_HEXADECIMAL_DIGIT_REQUIRED, start,
                               std::forward_as_tuple(context.token, start, start + 1));
                errorOccurred = true;
                continue;
            }

            iter = lastHex;
        }
        else if (iter[1] >= '0' && iter[1] <= '9')
        {
            // We take 8 and 9 here as well to tell the user it's an invalid octal instead of an invalid simple
            // escape sequence
            iter++;
            const auto* lastOctal = std::find_if(iter, iter + std::min<std::size_t>(3, std::distance(iter, end)),
                                                 [](char c) { return c < '0' || c > '7'; });
            if (lastOctal == iter)
            {
                // First character is 8 or 9. That's why we didn't encounter a single octal digit.
                // Also since there must be at least one character after \, lastOctal is definitely not end
                // here.
                context.report(coral66::Errors::Lexer::INVALID_OCTAL_CHARACTER, offset, std::string_view(lastOctal, 1),
                               std::forward_as_tuple(context.token, offset, offset + 1));
                errorOccurred = true;
                continue;
            }

            auto value = octalToValue({iter, static_cast<std::size_t>(lastOctal - iter)});
            if (value > largestCharacter)
            {
                auto start = context.token.getCharSpaceOffset() + (wide ? 2 : 1) + (iter - characters.data() - 1);
                context.report(coral66::Errors::Lexer::CHARACTER_TOO_LARGE_FOR_LITERAL_TYPE, start,
                               std::forward_as_tuple(context.token, start, start + lastOctal - iter));
                errorOccurred = true;
                iter = lastOctal;
                continue;
            }
            *resultStart = value;
            resultStart++;

            iter = lastOctal;
        }
        else
        {
            // Escape sequence or illegal escape
            if (auto character = escapeCharToValue(iter[1], offset, context))
            {
                *resultStart = *character;
                resultStart++;
            }
            else
            {
                errorOccurred = true;
            }
            iter += 2;
        }
    }

    if (literalType == Literal::CharLiteral)
    {
        for (auto* iter = result.data(); iter != resultStart; iter++)
        {
            if (*iter > largestCharacter)
            {
                context.report(coral66::Errors::Lexer::CHARACTER_TOO_LARGE_FOR_LITERAL_TYPE, context.token,
                               context.token);
                errorOccurred = true;
            }
        }
    }
    result.resize(std::distance(result.data(), resultStart));
    return {result, errorOccurred};
}

std::optional<std::pair<CToken::ValueType, CToken::Type>> processNumber(const char* begin, const char* end,
                                                                        std::uint64_t, ConversionContext&)
{
    std::string text(begin, end);
    bool isFloat = text.find('.') != std::string::npos || text.find('e') != std::string::npos
                   || text.find('E') != std::string::npos;

    if (isFloat)
    {
        llvm::APFloat number(llvm::APFloat::IEEEdouble());
        auto result = number.convertFromString(text, llvm::APFloat::rmNearestTiesToEven);
        if (!result)
        {
            // Fallback or error
            return {};
        }
        return {{std::move(number), CToken::Floating}};
    }
    else
    {
        // Integer (Decimal)
        // Coral 66 integers are decimal. OCTAL is handled via OCTAL().
        llvm::StringRef ref(text);
        llvm::APInt value;
        if (ref.getAsInteger(10, value))
        {
            return {};
        }
        // Use default size (e.g. 32 or 64-bit signed/unsigned)
        // For now, let's make it 64-bit signed APSInt
        if (value.getBitWidth() < 64)
        {
            value = value.sextOrTrunc(64);
        }
        return {{llvm::APSInt(value, false), CToken::Integer}};
    }
}
} // namespace

coral66::CSourceObject coral66::Lexer::toCTokens(PPSourceObject&& sourceObject, llvm::raw_ostream* reporter,
                                                 bool* errorsOccurred)
{
    auto tokens = toCTokens(sourceObject.data().data(), sourceObject.data().data() + sourceObject.data().size(),
                            sourceObject, reporter, errorsOccurred);
    tokens.shrink_to_fit();
    return {std::move(tokens), std::move(sourceObject.getFiles()), std::move(sourceObject.getSubstitutions())};
}

coral66::CSourceObject coral66::Lexer::toCTokens(const PPSourceObject& sourceObject, llvm::raw_ostream* reporter,
                                                 bool* errorsOccurred)
{
    auto tokens = toCTokens(sourceObject.data().data(), sourceObject.data().data() + sourceObject.data().size(),
                            sourceObject, reporter, errorsOccurred);
    tokens.shrink_to_fit();
    return {std::move(tokens),
            {sourceObject.getFiles().begin(), sourceObject.getFiles().end()},
            {sourceObject.getSubstitutions().begin(), sourceObject.getSubstitutions().end()}};
}

std::vector<coral66::Lexer::CToken> coral66::Lexer::toCTokens(PPTokenIterator begin, PPTokenIterator end,
                                                              const PPSourceInterface& sourceInterface,
                                                              llvm::raw_ostream* reporter, bool* errorsOccurred)
{
    if (errorsOccurred)
    {
        *errorsOccurred = false;
    }
    std::vector<CToken> result;
    result.reserve(end - begin);
    for (const auto* iter = begin; iter != end; iter++)
    {
        switch (iter->getTokenType())
        {
            case TokenType::Miscellaneous:
                if (errorsOccurred)
                {
                    *errorsOccurred = true;
                }
                if (reporter)
                {
                    *reporter << Errors::Lexer::UNEXPECTED_CHARACTER.args(*iter, sourceInterface, *iter);
                }
                break;
            case TokenType::Newline: break;
            case TokenType::Identifier:
                // An identifier consists of an
                // arbitrary sequence of lower case letters and digits,
                // starting with a letter.
                // It cannot be a keyword or contain spaces.

                // Filter keywords
                if (isKeyword(iter->getValue()))
                {
                    result.emplace_back(charactersToKeyword(iter->getValue()), iter->getOffset(), iter->getLength(),
                                        iter->getFileId(), iter->getMacroId());
                    break;
                }

                // Starts with a letter
                if (!isalpha(iter->getValue()[0]))
                {
                    break;
                }
                // Not alphanumeric or contains spaces or uppercase characters
                if (const auto res =
                        std::ranges::find_if(iter->getValue().begin(), iter->getValue().end(), [](const char c)
                                             { return (!isalnum(c) || (c == ' ' || std::isupper(c))); });
                    res != iter->getValue().end())
                {
                    break;
                }

                result.emplace_back(TokenType::Identifier, iter->getOffset(), iter->getLength(), iter->getFileId(),
                                    iter->getMacroId(), coral66::to_string(iter->getValue()));
                break;
            case TokenType::PPNumber:
            {
                ConversionContext context{reporter, sourceInterface, *iter, errorsOccurred};
                auto number = processNumber(iter->getValue().data(), iter->getValue().data() + iter->getValue().size(),
                                            iter->getCharSpaceOffset(), context);
                if (number)
                {
                    result.emplace_back(TokenType::Literal, iter->getOffset(), iter->getLength(), iter->getFileId(),
                                        iter->getMacroId(), std::move(number->first), number->second);
                }
                break;
            }
            case TokenType::StringLiteral:
            {
                auto maybeToken = parseStringLiteral(*iter, sourceInterface, reporter);
                if (!maybeToken)
                {
                    if (errorsOccurred)
                    {
                        *errorsOccurred = true;
                    }
                }
                else
                {
                    result.push_back(std::move(*maybeToken));
                }
                break;
            }
            case TokenType::Literal:
            {
                // Should not be reached with current lexer changes, but kept for safety/compatibility
                ConversionContext context{reporter, sourceInterface, *iter, errorsOccurred};
                bool wide = false; // No wide support
                auto [chars, errorOccurred] = processCharacters(iter->getValue(), context, wide, Literal::CharLiteral);
                if (chars.empty())
                {
                    break;
                }
                result.emplace_back(
                    TokenType::Literal, iter->getOffset(), iter->getLength(), iter->getFileId(), iter->getMacroId(),
                    llvm::APSInt(llvm::APInt(64, static_cast<std::uint64_t>(chars[0]), true), false), CToken::Integer);
                break;
            }
            default:
                result.emplace_back(iter->getTokenType(), iter->getOffset(), iter->getLength(), iter->getFileId(),
                                    iter->getMacroId());
        }
    }
    return result;
}

std::optional<CToken> coral66::Lexer::parseStringLiteral(const PPToken& ppToken,
                                                                         const PPSourceInterface& sourceInterface,
                                                                         llvm::raw_ostream* reporter)
{
    CORAL66_ASSERT(ppToken.getTokenType() == coral66::Lexer::TokenType::StringLiteral);
    bool errorsOccurred = false;
    ConversionContext context{reporter, sourceInterface, ppToken, &errorsOccurred};
    bool wide = ppToken.getRepresentation(sourceInterface)[0] == 'L';
    auto [chars, errorOccurred] = processCharacters(ppToken.getValue(), context, wide, Literal::StringLiteral);
    if (errorOccurred)
    {
        return {};
    }
    if (!wide)
    {
        const auto* start = chars.data();
        std::vector<llvm::UTF8> utf8(chars.size() * 4);
        auto* dest = utf8.data();
        if (llvm::ConvertUTF32toUTF8(&start, start + chars.size(), &dest, dest + utf8.size(), llvm::strictConversion)
            != llvm::conversionOK)
        {
            CORAL66_UNREACHABLE;
        }

        if (wide)
        {
            return CToken(TokenType::StringLiteral, ppToken.getOffset(), ppToken.getLength(), ppToken.getFileId(),
                          ppToken.getMacroId(), NonCharString{{utf8.data(), dest}, NonCharString::Wide});
        }
        return CToken(TokenType::StringLiteral, ppToken.getOffset(), ppToken.getLength(), ppToken.getFileId(),
                      ppToken.getMacroId(), std::string(utf8.data(), dest));
    }

    return CToken(TokenType::StringLiteral, ppToken.getOffset(), ppToken.getLength(), ppToken.getFileId(),
                  ppToken.getMacroId(), NonCharString{chars, NonCharString::Wide});
}

std::string_view coral66::Lexer::tokenName(coral66::Lexer::TokenType const tokenType, bool const quote)
{
    switch (tokenType)
    {
        // Symbols from Appendix 2 of the specification
        case TokenType::Identifier: return "identifier";
        case TokenType::OpenParentheses: return quote ? "'('" : "(";
        case TokenType::CloseParentheses: return quote ? "')'" : ")";
        case TokenType::StringLiteral: return "string literal";
        case TokenType::Literal: return "literal";
        case TokenType::SemiColon: return quote ? "';'" : ";";
        case TokenType::Comma: return quote ? "','" : ",";
        case TokenType::Minus: return quote ? "'-'" : "-";
        case TokenType::Plus: return quote ? "'+'" : "+";
        case TokenType::Asterisk: return quote ? "'*'" : "*";
        case TokenType::Division: return quote ? "'/'" : "/";
        case TokenType::Equal: return quote ? "'='" : "=";
        case TokenType::NotEqual: return quote ? "'!='" : "!=";
        case TokenType::LessThan: return quote ? "'<'" : "<";
        case TokenType::LessThanOrEqual: return quote ? "'<='" : "<=";
        case TokenType::GreaterThan: return quote ? "'>'" : ">";
        case TokenType::GreaterThanOrEqual: return quote ? "'>='" : ">=";
        case TokenType::Assignment: return quote ? "':='" : ":=";
        case TokenType::Colon: return quote ? "':'" : ":";
        case TokenType::OpenSquareBracket: return quote ? "'['" : "[";
        case TokenType::CloseSquareBracket: return quote ? "']'" : "]";
        case TokenType::Dot: return quote ? "'.'" : ".";
        case TokenType::Newline: return quote ? "newline" : "\n";
        case TokenType::CloseBrace: return quote ? "'}'" : "}";
        case TokenType::OpenBrace: return quote ? "'{'" : "{";
        case TokenType::Miscellaneous: return "Miscellaneous";
        case TokenType::PPNumber: return "PPNumber";
        case TokenType::PPToken: return "PPToken";

        // Coral Keywords, not stropped, from Appendix 2 of the specification
        case TokenType::AbsoluteKeyword: return "ABSOLUTE";
        case TokenType::AndKeyword: return "AND";
        case TokenType::AnswerKeyword: return "ANSWER";
        case TokenType::ArrayKeyword: return "ARRAY";
        case TokenType::BeginKeyword: return "BEGIN";
        case TokenType::BitKeyword: return "BIT";
        case TokenType::BitsKeyword: return "BITS";
        case TokenType::CodeKeyword: return "CODE";
        case TokenType::CommentKeyword: return "COMMENT";
        case TokenType::CommonKeyword: return "COMMON";
        case TokenType::DefineKeyword: return "DEFINE";
        case TokenType::DeleteKeyword: return "DELETE";
        case TokenType::DifferKeyword: return "DIFFER";
        case TokenType::DoKeyword: return "DO";
        case TokenType::ElseKeyword: return "ELSE";
        case TokenType::EndKeyword: return "END";
        case TokenType::ExternalKeyword: return "EXTERNAL";
        case TokenType::FixedKeyword: return "FIXED";
        case TokenType::FloatingKeyword: return "FLOATING";
        case TokenType::ForKeyword: return "FOR";
        case TokenType::GotoKeyword: return "GOTO";
        case TokenType::IfKeyword: return "IF";
        case TokenType::IntegerKeyword: return "INTEGER";
        case TokenType::LabelKeyword: return "LABEL";
        case TokenType::LibraryKeyword: return "LIBRARY";
        case TokenType::LiteralKeyword: return "LITERAL";
        case TokenType::LocationKeyword: return "LOCATION";
        case TokenType::MaskKeyword: return "MASK";
        case TokenType::OctalKeyword: return "OCTAL";
        case TokenType::OrKeyword: return "OR";
        case TokenType::OverlayKeyword: return "OVERLAY";
        case TokenType::PresetKeyword: return "PRESET";
        case TokenType::ProcedureKeyword: return "PROCEDURE";
        case TokenType::ProgramKeyword: return "PROGRAM";
        case TokenType::RecursiveKeyword: return "RECURSIVE";
        case TokenType::SegmentKeyword: return "SEGMENT";
        case TokenType::StepKeyword: return "STEP";
        case TokenType::SwitchKeyword: return "SWITCH";
        case TokenType::TableKeyword: return "TABLE";
        case TokenType::ThenKeyword: return "THEN";
        case TokenType::UnionKeyword: return "UNION";
        case TokenType::UnsignedKeyword: return "UNSIGNED";
        case TokenType::UntilKeyword: return "UNTIL";
        case TokenType::ValueKeyword: return "VALUE";
        case TokenType::WhileKeyword: return "WHILE";
        case TokenType::WithKeyword: return "WITH";
    }
    CORAL66_UNREACHABLE;
}

std::string coral66::Lexer::normalizeSpelling(std::string_view tokenSpelling)
{
    std::string result;
    result.reserve(tokenSpelling.size());
    std::size_t i = 0;
    while (true)
    {
        for (; i < tokenSpelling.size() && tokenSpelling[i] != '?' && tokenSpelling[i] != '\\'; i++)
            ;
        if (i >= tokenSpelling.size())
        {
            result += tokenSpelling;
            break;
        }
        if (tokenSpelling.substr(i, 2) == "\\\n")
        {
            result += tokenSpelling.substr(0, i);
            tokenSpelling = tokenSpelling.substr(i + 2);
            i += 2;
            continue;
        }
        if (tokenSpelling.substr(i, 4) == "?\?/\n")
        {
            result += tokenSpelling.substr(0, i);
            tokenSpelling = tokenSpelling.substr(i + 4);
            i += 4;
            continue;
        }
        static const std::unordered_map<std::string_view, char> mapping = {{"?\?=", '#'}, {"?\?(", '['}, {"?\?/", '\\'},
                                                                           {"?\?)", ']'}, {"?\?'", '^'}, {"?\?<", '{'},
                                                                           {"?\?!", '|'}, {"?\?>", '}'}, {"?\?-", '~'}};
        auto rep = mapping.find(tokenSpelling.substr(i, 3));
        if (rep == mapping.end())
        {
            i += 3;
            continue;
        }
        result += tokenSpelling.substr(0, i);
        result += rep->second;
        tokenSpelling = tokenSpelling.substr(i + 3);
        i += 3;
    }
    return result;
}

bool coral66::Lexer::needsWhitespaceInBetween(TokenType const left, TokenType const right) noexcept
{
    if (isText(left))
    {
        switch (right)
        {
            case TokenType::StringLiteral:
            case TokenType::Literal:
            case TokenType::PPNumber: return true;
            default:
                if (isText(right))
                {
                    return true;
                }
                break;
        }
        return false;
    }
    // Simplified for Coral: most punctuation separates itself.
    // But we might want to prevent accidental merging if we have ambiguous multi-char operators.
    // For now, minimal logic:
    if (left == TokenType::PPNumber && isText(right))
        return true;
    if (left == TokenType::PPNumber && right == TokenType::PPNumber)
        return true;
    return false;
}

bool coral66::Lexer::isText(TokenType const tokenType)
{
    switch (tokenType)
    {
        case TokenType::Identifier:
        case TokenType::BeginKeyword:
        case TokenType::EndKeyword:
        case TokenType::ProcedureKeyword:
        case TokenType::ProgramKeyword:
        case TokenType::SegmentKeyword:
        case TokenType::IfKeyword:
        case TokenType::ThenKeyword:
        case TokenType::ElseKeyword:
        case TokenType::ForKeyword:
        case TokenType::StepKeyword:
        case TokenType::UntilKeyword:
        case TokenType::DoKeyword:
        case TokenType::GotoKeyword:
        case TokenType::AnswerKeyword:
        case TokenType::IntegerKeyword:
        case TokenType::FloatingKeyword:
        case TokenType::FixedKeyword:
        case TokenType::BitsKeyword:
        case TokenType::ArrayKeyword:
        case TokenType::TableKeyword:
        case TokenType::PresetKeyword:
        case TokenType::OverlayKeyword:
        case TokenType::ValueKeyword:
        case TokenType::LocationKeyword:
        case TokenType::CommonKeyword:
        case TokenType::CommentKeyword:
        case TokenType::DefineKeyword:
        case TokenType::CodeKeyword: return true;
        default: return false;
    }
}
