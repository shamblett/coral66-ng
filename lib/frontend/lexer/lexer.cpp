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
#include <cmath>
#include <numeric>
#include <optional>

#include "lexer_support.hpp"
#include "lib/frontend/common/messages/errors.hpp"
#include "lib/frontend/common/source_object.hpp"
#include "lib/support/text.hpp"
#include "lib/support/vendor/ctrack/include/ctrack.hpp"

using namespace coral66::lexer;
using namespace coral66::detail;

coral66::PPSourceObject coral66::lexer::tokenize(std::string source, llvm::raw_ostream* reporter, bool* errorsOccurred,
                                                 std::string_view const sourcePath, bool clearDiagnostics)
{
    CTRACK_NAME("Lexer");
    if (clearDiagnostics)
    {
        DiagnosticCollector::clearDiagnostics();
    }

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

    std::string charactersSpace = source;
    IntervalMap characterToSourceSpace;
    characterToSourceSpace.emplace_back(0, 0, source.size());
    characterToSourceSpace.emplace_back(source.size(), 0, 0);

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
                llvm::UTF32* targetStart = &result;
                auto convRes = llvm::ConvertUTF8toUTF32(
                    reinterpret_cast<const llvm::UTF8**>(&start),
                    reinterpret_cast<const llvm::UTF8*>(end),
                    &targetStart, targetStart + 1, llvm::strictConversion);

                if (convRes != llvm::conversionOK && convRes != llvm::targetExhausted)
                {
                    step = getNumBytesForUTF8(iter, end);
                    context.report(errors::lexer::INVALID_UTF8_SEQUENCE, offset, std::pair{offset, offset + step});
                    DiagnosticCollector::addDiagnostic(errors::lexer::INVALID_UTF8_SEQUENCE);
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
        // ReSharper disable once CppPassValueParameterByConstReference
        // ReSharper disable once CppParameterMayBeConstPtrOrRef
        [&context](StringLiteral& s)
        {
            if (s.sawQuote)
            {
                context.push(TokenType::StringLiteral, s.characters);
                return false;
            }
            context.report(errors::lexer::UNTERMINATED_STRING_LITERAL, context.tokenStartOffset,
                           std::pair{context.tokenStartOffset, context.getOffset()});
            DiagnosticCollector::addDiagnostic(errors::lexer::UNTERMINATED_STRING_LITERAL);
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

    if (std::holds_alternative<CommentSentence>(stateMachine))
    {
        context.report(errors::lexer::UNTERMINATED_COMMENT_KEYWORD_SEQUENCE, context.tokenStartOffset,
                       std::pair{context.tokenStartOffset, context.getOffset()});
        DiagnosticCollector::addDiagnostic(errors::lexer::UNTERMINATED_COMMENT_KEYWORD_SEQUENCE);
    }
    if (std::holds_alternative<BracketedComment>(stateMachine))
    {
        context.report(errors::lexer::UNTERMINATED_BRACKETED_COMMENT, context.tokenStartOffset,
                       std::pair{context.tokenStartOffset, context.getOffset()});
        DiagnosticCollector::addDiagnostic(errors::lexer::UNTERMINATED_BRACKETED_COMMENT);
    }
    characterToSourceSpace.shrink_to_fit();

    if (errorsOccurred)
    {
        *errorsOccurred = context.errorsOccurred();
    }

    auto result = PPSourceObject(std::move(context.getResult()),
                                 {Source::File{to_string(sourcePath), std::move(source), std::move(starts), false}}, {},
                                 {{std::move(characterToSourceSpace)}});
    result.addDiagnostics(DiagnosticCollector::getDiagnostics());
    return result;
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
                    if constexpr (coral66::TupleLike<std::decay_t<Location>>)
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

std::optional<std::pair<CToken::ValueType, CToken::Type>> processNumber(const char* begin, const char* end,
                                                                        std::uint64_t, ConversionContext&)
{
    const std::string text(begin, end);

    if (text.find('.') != std::string::npos || text.find('e') != std::string::npos)
    {
        llvm::APFloat number(llvm::APFloat::IEEEdouble());
        if (auto result = number.convertFromString(text, llvm::APFloat::rmNearestTiesToEven); !result)
        {
            return {};
        }
        return {{std::move(number), CToken::Floating}};
    }
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
    auto result = CSourceObject(std::move(tokens), {sourceObject.getFiles().begin(), sourceObject.getFiles().end()},
                                {sourceObject.getSubstitutions().begin(), sourceObject.getSubstitutions().end()});
    result.addDiagnostics(DiagnosticCollector::getDiagnostics());
    return result;
}

std::vector<CToken> coral66::lexer::toCTokens(const PPTokenIterator begin, const PPTokenIterator end,
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
        if (iter->getTokenType() == TokenType::PPNumber || iter->getTokenType() == TokenType::Apostrophe)
        {
            std::string numberText;
            auto startIter = iter;

            if (iter->getTokenType() == TokenType::PPNumber)
            {
                numberText += std::string(iter->getValue());
                auto next = iter + 1;
                if (next != end && next->getTokenType() == TokenType::Apostrophe)
                {
                    numberText += "e";
                    iter = next;
                    next = iter + 1;
                    if (next != end
                        && (next->getTokenType() == TokenType::Plus || next->getTokenType() == TokenType::Minus))
                    {
                        numberText += (next->getTokenType() == TokenType::Plus) ? "+" : "-";
                        iter = next;
                        next = iter + 1;
                    }
                    if (next != end && next->getTokenType() == TokenType::PPNumber)
                    {
                        numberText += std::string(next->getValue());
                        iter = next;
                    }
                }
            }
            else // TokenType::Apostrophe
            {
                numberText += "1e";
                auto next = iter + 1;
                if (next != end
                    && (next->getTokenType() == TokenType::Plus || next->getTokenType() == TokenType::Minus))
                {
                    numberText += (next->getTokenType() == TokenType::Plus) ? "+" : "-";
                    iter = next;
                    next = iter + 1;
                }
                if (next != end && next->getTokenType() == TokenType::PPNumber)
                {
                    numberText += std::string(next->getValue());
                    iter = next;
                }
            }

            ConversionContext context{reporter, sourceInterface, *startIter, errorsOccurred};
            auto number = processNumber(numberText.data(), numberText.data() + numberText.size(),
                                        startIter->getCharSpaceOffset(), context);
            if (number)
            {
                std::uint64_t length = (iter->getOffset() + iter->getLength()) - startIter->getOffset();
                result.emplace_back(TokenType::Literal, startIter->getOffset(), length, startIter->getFileId(),
                                    startIter->getMacroId(), std::move(number->first), number->second);
            }
            else
            {
                if (errorsOccurred)
                {
                    *errorsOccurred = true;
                }
                if (reporter)
                {
                    *reporter << errors::lexer::MALFORMED_NUMBER_LITERAL.args(*startIter, sourceInterface, *startIter);
                    DiagnosticCollector::addDiagnostic(errors::lexer::MALFORMED_NUMBER_LITERAL);
                }
            }
            continue;
        }

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
                    DiagnosticCollector::addDiagnostic(errors::lexer::UNEXPECTED_CHARACTER);
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
    std::string value = std::string(PPToken.getValue());
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
    {
        value = value.substr(1, value.size() - 2);
    }
    std::string finalValue;
    for (size_t i = 0; i < value.size(); ++i)
    {
        if (value[i] == '"' && i + 1 < value.size() && value[i + 1] == '"')
        {
            finalValue += '"';
            i++; // skip next quote
        }
        else
        {
            finalValue += value[i];
        }
    }
    value = std::move(finalValue);
    return CToken(TokenType::StringLiteral, PPToken.getOffset(), PPToken.getLength(), PPToken.getFileId(),
                  PPToken.getMacroId(), std::move(value));
}

std::string_view coral66::lexer::tokenName(TokenType const tokenType, bool const quote)
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
        case TokenType::Apostrophe: return quote ? "''" : "'";
        case TokenType::Assignment: return quote ? "':='" : ":=";
        case TokenType::Colon: return quote ? "':'" : ":";
        case TokenType::OpenSquareBracket: return quote ? "'['" : "[";
        case TokenType::CloseSquareBracket: return quote ? "']'" : "]";
        case TokenType::Dot: return quote ? "'.'" : ".";
        case TokenType::Newline: return quote ? "newline" : "\n";
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

        // No Token
        case TokenType::NoToken: return "None";
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
        for (; i < tokenSpelling.size() && tokenSpelling[i] != '\\'; i++)
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
            i = 0;
            continue;
        }
        result += tokenSpelling.substr(0, i + 1);
        tokenSpelling = tokenSpelling.substr(i + 1);
        i = 0;
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
