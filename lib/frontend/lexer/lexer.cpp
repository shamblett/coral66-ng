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

#include "lexer_support.hpp"
#include "lib/frontend/common/messages/errors.hpp"
#include "lib/frontend/common/source_object.hpp"
#include "lib/support/text.hpp"

using namespace coral66::lexer;

coral66::PPSourceObject coral66::lexer::tokenize(std::string source, llvm::raw_ostream* reporter, bool* errorsOccurred,
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
                    context.report(errors::lexer::INVALID_UTF8_SEQUENCE, offset, std::pair{offset, offset + step});
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
        while (match(stateMachine, visitor))
        {
            offset = prevOffset;
        }
        offset = prevOffset + step;
        iter += step;
    }
    const bool needNewline = match(
        stateMachine,
        // ReSharper disable once CppDFAConstantFunctionResult
        [&context](const StringLiteral& s)
        {
            if (s.sawQuote)
            {
                context.push(TokenType::StringLiteral, s.characters);
                return false;
            }
            context.report(errors::lexer::UNTERMINATED_STRING_LITERAL, context.tokenStartOffset,
                           std::pair{context.tokenStartOffset, context.getOffset()});
            return false;
        },
        [&context](CharacterLiteral&)
        {
            context.report(errors::lexer::UNTERMINATED_CHARACTER_LITERAL, context.tokenStartOffset,
                           std::pair{context.tokenStartOffset, context.getOffset()});
            return false;
        },
        [](Start&) { return false; }, [](auto&&) { return true; });
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
        while (match(stateMachine, visitor))
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

[[nodiscard]] std::string_view TokenBase::getRepresentation(const source_interface& sourceObject) const
{
    return std::string_view(sourceObject.getFiles()[static_cast<std::uint64_t>(m_fileID)].source)
        .substr(m_offset, m_length);
}

[[nodiscard]] std::uint64_t TokenBase::getLine(const source_interface& sourceObject) const noexcept
{
    return sourceObject.getLineNumber(m_fileID, m_offset);
}

[[nodiscard]] std::uint64_t TokenBase::getColumn(const source_interface& sourceObject) const noexcept
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
            context.report(coral66::errors::lexer::EXPECTED_CHARACTER_AFTER_BACKSLASH, backslash + 1,
                           std::forward_as_tuple(context.token, backslash, backslash + 2));

            return {};
        }
        default:
        {
            context.report(coral66::errors::lexer::INVALID_ESCAPE_SEQUENCE_N, backslash + 1, std::string("\\") + escape,
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
                context.report(coral66::errors::lexer::NEWLINE_IN_CHARACTER_LITERAL_USE_BACKLASH_N, offset,
                               std::forward_as_tuple(context.token, offset));
            }
            else
            {
                context.report(coral66::errors::lexer::NEWLINE_IN_STRING_LITERAL_USE_BACKLASH_N, offset,
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

            const auto res = llvm::ConvertUTF8toUTF32(reinterpret_cast<const llvm::UTF8**>(&start),
                                                      reinterpret_cast<const llvm::UTF8*>(iter), &resultStart,
                                                      resultEnd, llvm::strictConversion);
            if (res != llvm::conversionOK)
            {
                auto invalidStart = context.token.getCharSpaceOffset() + (wide ? 2 : 1) + (start - characters.data());
                auto invalidEnd = invalidStart + coral66::getNumBytesForUTF8(start, end) - 1;
                context.report(coral66::errors::lexer::INVALID_UTF8_SEQUENCE, invalidStart,
                               std::make_tuple(context.token, invalidStart, invalidEnd));
                errorOccurred = true;
            }
            continue;
        }

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
                auto start = context.token.getCharSpaceOffset() + (wide ? 2 : 1) + (iter - characters.data() - 2);
                context.report(coral66::errors::lexer::INVALID_ESCAPE_SEQUENCE_N, start,
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
                    coral66::errors::lexer::INVALID_UC_EXPECTED_N_MORE_DIGITS, start,
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
                context.report(coral66::errors::lexer::AT_LEAST_ONE_HEXADECIMAL_DIGIT_REQUIRED, start,
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
                context.report(coral66::errors::lexer::INVALID_OCTAL_CHARACTER, offset, std::string_view(lastOctal, 1),
                               std::forward_as_tuple(context.token, offset, offset + 1));
                errorOccurred = true;
                continue;
            }

            auto value = octalToValue({iter, static_cast<std::size_t>(lastOctal - iter)});
            if (value > largestCharacter)
            {
                auto start = context.token.getCharSpaceOffset() + (wide ? 2 : 1) + (iter - characters.data() - 1);
                context.report(coral66::errors::lexer::CHARACTER_TOO_LARGE_FOR_LITERAL_TYPE, start,
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
                context.report(coral66::errors::lexer::CHARACTER_TOO_LARGE_FOR_LITERAL_TYPE, context.token,
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
        const llvm::StringRef ref(text);
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

coral66::CSourceObject coral66::lexer::toCTokens(PPSourceObject&& sourceObject, llvm::raw_ostream* reporter,
                                                 bool* errorsOccurred)
{
    auto tokens = toCTokens(sourceObject.data().data(), sourceObject.data().data() + sourceObject.data().size(),
                            sourceObject, reporter, errorsOccurred);
    tokens.shrink_to_fit();
    return {std::move(tokens), std::move(sourceObject.getFiles()), std::move(sourceObject.getSubstitutions())};
}

coral66::CSourceObject coral66::lexer::toCTokens(const PPSourceObject& sourceObject, llvm::raw_ostream* reporter,
                                                 bool* errorsOccurred)
{
    auto tokens = toCTokens(sourceObject.data().data(), sourceObject.data().data() + sourceObject.data().size(),
                            sourceObject, reporter, errorsOccurred);
    tokens.shrink_to_fit();
    return {std::move(tokens),
            {sourceObject.getFiles().begin(), sourceObject.getFiles().end()},
            {sourceObject.getSubstitutions().begin(), sourceObject.getSubstitutions().end()}};
}

std::vector<CToken> coral66::lexer::toCTokens(PPTokenIterator begin, PPTokenIterator end,
                                              const PPSourceInterface& sourceInterface, llvm::raw_ostream* reporter,
                                              bool* errorsOccurred)
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
                    *reporter << errors::lexer::UNEXPECTED_CHARACTER.args(*iter, sourceInterface, *iter);
                }
                break;
            case TokenType::Newline: break;
            case TokenType::Identifier:

                // Filter keywords
                if (isKeyword(iter->getValue()))
                {
                    result.emplace_back(charactersToKeyword(iter->getValue()), iter->getOffset(), iter->getLength(),
                                        iter->getFileId(), iter->getMacroId());
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
                constexpr bool wide = false; // No wide support
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

std::optional<CToken> coral66::lexer::parseStringLiteral(const PPToken& PPToken,
                                                         const PPSourceInterface& sourceInterface,
                                                         llvm::raw_ostream* reporter)
{
    CORAL66_ASSERT(PPToken.getTokenType() == coral66::lexer::TokenType::StringLiteral);
    bool errorsOccurred = false;
    ConversionContext context{reporter, sourceInterface, PPToken, &errorsOccurred};
    bool wide = PPToken.getRepresentation(sourceInterface)[0] == 'L';
    auto [chars, errorOccurred] = processCharacters(PPToken.getValue(), context, wide, Literal::StringLiteral);
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

        return CToken(TokenType::StringLiteral, PPToken.getOffset(), PPToken.getLength(), PPToken.getFileId(),
                      PPToken.getMacroId(), std::string(utf8.data(), dest));
    }

    return CToken(TokenType::StringLiteral, PPToken.getOffset(), PPToken.getLength(), PPToken.getFileId(),
                  PPToken.getMacroId(), NonCharString{chars, NonCharString::Wide});
}

std::string_view coral66::lexer::tokenName(coral66::lexer::TokenType const tokenType, bool const quote)
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

std::string coral66::lexer::normalizeSpelling(std::string_view tokenSpelling)
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

bool coral66::lexer::needsWhitespaceInBetween(TokenType const left, TokenType const right) noexcept
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

    if (left == TokenType::PPNumber && isText(right))
        return true;
    if (left == TokenType::PPNumber && right == TokenType::PPNumber)
        return true;
    return false;
}

bool coral66::lexer::isText(TokenType const tokenType)
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
