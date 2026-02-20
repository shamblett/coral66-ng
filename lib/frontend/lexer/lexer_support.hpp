/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include "lib/frontend/common/messages/errors.hpp"
#include "lib/frontend/common/messages/message.hpp"
#include "lib/frontend/common/source_object.hpp"
#include "lib/support/scope_exit.hpp"

/**
 * Lexer support functions
 */

namespace coral66::Lexer
{

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

bool isKeyword(std::string_view characters);

TokenType charactersToKeyword(std::string_view characters);

std::uint32_t hexToValue(std::string_view value);

constexpr auto UTF32_MAX = 0x10FFFF;

using StateMachine = std::variant<Start, CharacterLiteral, StringLiteral, Text, PreprocessingNumber, MaybeUC,
                                  UniversalCharacter, LineComment, BlockComment, Dot, Punctuation, AfterInclude, L>;

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

inline std::pair<std::uint64_t, std::uint64_t> map(const IntervalMap& intervalMap, std::uint64_t const offset)
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

class Context : source_interface
{
    llvm::raw_ostream* m_reporter;
    std::vector<PPToken> m_result;
    std::string_view m_sourceSpace;
    std::string_view m_characterSpace;
    std::uint64_t& m_offset;
    std::vector<std::uint64_t> m_lineStarts;
    const IntervalMap& m_characterToSourceSpace;
    bool m_errorsOccurred = false;
    Source::File m_fakeFile;

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

} // namespace coral66::Lexer