/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "lexer.hpp"

#include <llvm/Support/ConvertUTF.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/UnicodeCharRanges.h>

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

#pragma region UnicodeRanges

// C99 Annex D
constexpr llvm::sys::UnicodeCharRange C99AllowedIDCharRanges[] = {
    // Latin (1)
    {0x00AA, 0x00AA},

    // Special characters (1)
    {0x00B5, 0x00B5},
    {0x00B7, 0x00B7},

    // Latin (2)
    {0x00BA, 0x00BA},
    {0x00C0, 0x00D6},
    {0x00D8, 0x00F6},
    {0x00F8, 0x01F5},
    {0x01FA, 0x0217},
    {0x0250, 0x02A8},

    // Special characters (2)
    {0x02B0, 0x02B8},
    {0x02BB, 0x02BB},
    {0x02BD, 0x02C1},
    {0x02D0, 0x02D1},
    {0x02E0, 0x02E4},
    {0x037A, 0x037A},

    // Greek (1)
    {0x0386, 0x0386},
    {0x0388, 0x038A},
    {0x038C, 0x038C},
    {0x038E, 0x03A1},
    {0x03A3, 0x03CE},
    {0x03D0, 0x03D6},
    {0x03DA, 0x03DA},
    {0x03DC, 0x03DC},
    {0x03DE, 0x03DE},
    {0x03E0, 0x03E0},
    {0x03E2, 0x03F3},

    // Cyrillic
    {0x0401, 0x040C},
    {0x040E, 0x044F},
    {0x0451, 0x045C},
    {0x045E, 0x0481},
    {0x0490, 0x04C4},
    {0x04C7, 0x04C8},
    {0x04CB, 0x04CC},
    {0x04D0, 0x04EB},
    {0x04EE, 0x04F5},
    {0x04F8, 0x04F9},

    // Armenian (1)
    {0x0531, 0x0556},

    // Special characters (3)
    {0x0559, 0x0559},

    // Armenian (2)
    {0x0561, 0x0587},

    // Hebrew
    {0x05B0, 0x05B9},
    {0x05BB, 0x05BD},
    {0x05BF, 0x05BF},
    {0x05C1, 0x05C2},
    {0x05D0, 0x05EA},
    {0x05F0, 0x05F2},

    // Arabic (1)
    {0x0621, 0x063A},
    {0x0640, 0x0652},

    // Digits (1)
    {0x0660, 0x0669},

    // Arabic (2)
    {0x0670, 0x06B7},
    {0x06BA, 0x06BE},
    {0x06C0, 0x06CE},
    {0x06D0, 0x06DC},
    {0x06E5, 0x06E8},
    {0x06EA, 0x06ED},

    // Digits (2)
    {0x06F0, 0x06F9},

    // Devanagari and Special character 0x093D.
    {0x0901, 0x0903},
    {0x0905, 0x0939},
    {0x093D, 0x094D},
    {0x0950, 0x0952},
    {0x0958, 0x0963},

    // Digits (3)
    {0x0966, 0x096F},

    // Bengali (1)
    {0x0981, 0x0983},
    {0x0985, 0x098C},
    {0x098F, 0x0990},
    {0x0993, 0x09A8},
    {0x09AA, 0x09B0},
    {0x09B2, 0x09B2},
    {0x09B6, 0x09B9},
    {0x09BE, 0x09C4},
    {0x09C7, 0x09C8},
    {0x09CB, 0x09CD},
    {0x09DC, 0x09DD},
    {0x09DF, 0x09E3},

    // Digits (4)
    {0x09E6, 0x09EF},

    // Bengali (2)
    {0x09F0, 0x09F1},

    // Gurmukhi (1)
    {0x0A02, 0x0A02},
    {0x0A05, 0x0A0A},
    {0x0A0F, 0x0A10},
    {0x0A13, 0x0A28},
    {0x0A2A, 0x0A30},
    {0x0A32, 0x0A33},
    {0x0A35, 0x0A36},
    {0x0A38, 0x0A39},
    {0x0A3E, 0x0A42},
    {0x0A47, 0x0A48},
    {0x0A4B, 0x0A4D},
    {0x0A59, 0x0A5C},
    {0x0A5E, 0x0A5E},

    // Digits (5)
    {0x0A66, 0x0A6F},

    // Gurmukhi (2)
    {0x0A74, 0x0A74},

    // Gujarti
    {0x0A81, 0x0A83},
    {0x0A85, 0x0A8B},
    {0x0A8D, 0x0A8D},
    {0x0A8F, 0x0A91},
    {0x0A93, 0x0AA8},
    {0x0AAA, 0x0AB0},
    {0x0AB2, 0x0AB3},
    {0x0AB5, 0x0AB9},
    {0x0ABD, 0x0AC5},
    {0x0AC7, 0x0AC9},
    {0x0ACB, 0x0ACD},
    {0x0AD0, 0x0AD0},
    {0x0AE0, 0x0AE0},

    // Digits (6)
    {0x0AE6, 0x0AEF},

    // Oriya and Special character 0x0B3D
    {0x0B01, 0x0B03},
    {0x0B05, 0x0B0C},
    {0x0B0F, 0x0B10},
    {0x0B13, 0x0B28},
    {0x0B2A, 0x0B30},
    {0x0B32, 0x0B33},
    {0x0B36, 0x0B39},
    {0x0B3D, 0x0B43},
    {0x0B47, 0x0B48},
    {0x0B4B, 0x0B4D},
    {0x0B5C, 0x0B5D},
    {0x0B5F, 0x0B61},

    // Digits (7)
    {0x0B66, 0x0B6F},

    // Tamil
    {0x0B82, 0x0B83},
    {0x0B85, 0x0B8A},
    {0x0B8E, 0x0B90},
    {0x0B92, 0x0B95},
    {0x0B99, 0x0B9A},
    {0x0B9C, 0x0B9C},
    {0x0B9E, 0x0B9F},
    {0x0BA3, 0x0BA4},
    {0x0BA8, 0x0BAA},
    {0x0BAE, 0x0BB5},
    {0x0BB7, 0x0BB9},
    {0x0BBE, 0x0BC2},
    {0x0BC6, 0x0BC8},
    {0x0BCA, 0x0BCD},

    // Digits (8)
    {0x0BE7, 0x0BEF},

    // Telugu
    {0x0C01, 0x0C03},
    {0x0C05, 0x0C0C},
    {0x0C0E, 0x0C10},
    {0x0C12, 0x0C28},
    {0x0C2A, 0x0C33},
    {0x0C35, 0x0C39},
    {0x0C3E, 0x0C44},
    {0x0C46, 0x0C48},
    {0x0C4A, 0x0C4D},
    {0x0C60, 0x0C61},

    // Digits (9)
    {0x0C66, 0x0C6F},

    // Kannada
    {0x0C82, 0x0C83},
    {0x0C85, 0x0C8C},
    {0x0C8E, 0x0C90},
    {0x0C92, 0x0CA8},
    {0x0CAA, 0x0CB3},
    {0x0CB5, 0x0CB9},
    {0x0CBE, 0x0CC4},
    {0x0CC6, 0x0CC8},
    {0x0CCA, 0x0CCD},
    {0x0CDE, 0x0CDE},
    {0x0CE0, 0x0CE1},

    // Digits (10)
    {0x0CE6, 0x0CEF},

    // Malayam
    {0x0D02, 0x0D03},
    {0x0D05, 0x0D0C},
    {0x0D0E, 0x0D10},
    {0x0D12, 0x0D28},
    {0x0D2A, 0x0D39},
    {0x0D3E, 0x0D43},
    {0x0D46, 0x0D48},
    {0x0D4A, 0x0D4D},
    {0x0D60, 0x0D61},

    // Digits (11)
    {0x0D66, 0x0D6F},

    // Thai...including Digits { 0x0E50, 0x0E59 }
    {0x0E01, 0x0E3A},
    {0x0E40, 0x0E5B},

    // Lao (1)
    {0x0E81, 0x0E82},
    {0x0E84, 0x0E84},
    {0x0E87, 0x0E88},
    {0x0E8A, 0x0E8A},
    {0x0E8D, 0x0E8D},
    {0x0E94, 0x0E97},
    {0x0E99, 0x0E9F},
    {0x0EA1, 0x0EA3},
    {0x0EA5, 0x0EA5},
    {0x0EA7, 0x0EA7},
    {0x0EAA, 0x0EAB},
    {0x0EAD, 0x0EAE},
    {0x0EB0, 0x0EB9},
    {0x0EBB, 0x0EBD},
    {0x0EC0, 0x0EC4},
    {0x0EC6, 0x0EC6},
    {0x0EC8, 0x0ECD},

    // Digits (12)
    {0x0ED0, 0x0ED9},

    // Lao (2)
    {0x0EDC, 0x0EDD},

    // Tibetan (1)
    {0x0F00, 0x0F00},
    {0x0F18, 0x0F19},

    // Digits (13)
    {0x0F20, 0x0F33},

    // Tibetan (2)
    {0x0F35, 0x0F35},
    {0x0F37, 0x0F37},
    {0x0F39, 0x0F39},
    {0x0F3E, 0x0F47},
    {0x0F49, 0x0F69},
    {0x0F71, 0x0F84},
    {0x0F86, 0x0F8B},
    {0x0F90, 0x0F95},
    {0x0F97, 0x0F97},
    {0x0F99, 0x0FAD},
    {0x0FB1, 0x0FB7},
    {0x0FB9, 0x0FB9},

    // Georgian
    {0x10A0, 0x10C5},
    {0x10D0, 0x10F6},

    // Latin (3)
    {0x1E00, 0x1E9B},
    {0x1EA0, 0x1EF9},

    // Greek (2)
    {0x1F00, 0x1F15},
    {0x1F18, 0x1F1D},
    {0x1F20, 0x1F45},
    {0x1F48, 0x1F4D},
    {0x1F50, 0x1F57},
    {0x1F59, 0x1F59},
    {0x1F5B, 0x1F5B},
    {0x1F5D, 0x1F5D},
    {0x1F5F, 0x1F7D},
    {0x1F80, 0x1FB4},
    {0x1FB6, 0x1FBC},

    // Special characters (4)
    {0x1FBE, 0x1FBE},

    // Greek (3)
    {0x1FC2, 0x1FC4},
    {0x1FC6, 0x1FCC},
    {0x1FD0, 0x1FD3},
    {0x1FD6, 0x1FDB},
    {0x1FE0, 0x1FEC},
    {0x1FF2, 0x1FF4},
    {0x1FF6, 0x1FFC},

    // Special characters (5)
    {0x203F, 0x2040},

    // Latin (4)
    {0x207F, 0x207F},

    // Special characters (6)
    {0x2102, 0x2102},
    {0x2107, 0x2107},
    {0x210A, 0x2113},
    {0x2115, 0x2115},
    {0x2118, 0x211D},
    {0x2124, 0x2124},
    {0x2126, 0x2126},
    {0x2128, 0x2128},
    {0x212A, 0x2131},
    {0x2133, 0x2138},
    {0x2160, 0x2182},
    {0x3005, 0x3007},
    {0x3021, 0x3029},

    // Hiragana
    {0x3041, 0x3093},
    {0x309B, 0x309C},

    // Katakana
    {0x30A1, 0x30F6},
    {0x30FB, 0x30FC},

    // Bopmofo [sic]
    {0x3105, 0x312C},

    // CJK Unified Ideographs
    {0x4E00, 0x9FA5},

    // Hangul,
    {0xAC00, 0xD7A3}};

const llvm::sys::UnicodeCharSet C99AllowedIDChar(C99AllowedIDCharRanges);

// C99 6.4.2.1p3: The initial character [of an identifier] shall not be a
// universal character name designating a digit.
// C99 Annex D defines these characters as "Digits".
constexpr llvm::sys::UnicodeCharRange C99DisallowedInitialIDCharRanges[] = {
    {0x0660, 0x0669}, {0x06F0, 0x06F9}, {0x0966, 0x096F}, {0x09E6, 0x09EF}, {0x0A66, 0x0A6F},
    {0x0AE6, 0x0AEF}, {0x0B66, 0x0B6F}, {0x0BE7, 0x0BEF}, {0x0C66, 0x0C6F}, {0x0CE6, 0x0CEF},
    {0x0D66, 0x0D6F}, {0x0E50, 0x0E59}, {0x0ED0, 0x0ED9}, {0x0F20, 0x0F33}};

const llvm::sys::UnicodeCharSet C99DisallowedInitialIDChar(C99DisallowedInitialIDCharRanges);

#pragma endregion UnicodeRanges

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
        // does this make sense?
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
            static_assert(std::is_base_of_v<coral66::Lexer::TokenBase, std::decay_t<T3>>
                          || std::is_base_of_v<coral66::Lexer::TokenBase, std::decay_t<T1>>);
            if constexpr (std::is_base_of_v<coral66::Lexer::TokenBase, std::decay_t<T1>>)
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

class Context : private coral66::source_interface
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

    [[nodiscard]] std::uint64_t getLineNumber(std::uint32_t, std::uint64_t offset) const noexcept override
    {
        auto result = std::ranges::lower_bound(m_lineStarts.begin(), m_lineStarts.end(), offset);
        CORAL66_ASSERT(result != m_lineStarts.end());
        return std::distance(m_lineStarts.begin(), result) + (*result == offset ? 1 : 0);
    }

    [[nodiscard]] std::uint64_t getLineStartOffset(std::uint32_t, std::uint64_t line) const noexcept override
    {
        CORAL66_ASSERT(line - 1 < m_lineStarts.size());
        return m_lineStarts[line - 1];
    }

    [[nodiscard]] std::uint64_t getLineEndOffset(std::uint32_t, std::uint64_t line) const noexcept override
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

    void push(TokenType tokenType, const std::string& value = {})
    {
        push(tokenStartOffset, m_offset, tokenType, value);
    }

    void push(std::uint64_t diff, TokenType tokenType, const std::string& value = {})
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
        case '\'': return StringLiteral{}; // Coral uses single quotes for strings
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
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'
                || (C99AllowedIDChar.contains(c) && !C99DisallowedInitialIDChar.contains(c)))
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
        && (std::find_if_not(characters.rbegin(), characters.rend(), [](char c) { return c == '\\'; })
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
    if (c == '"')
    {
        return {StringLiteral{true, {}}, true};
    }
    if (c == '\'')
    {
        return {CharacterLiteral{true, {}}, true};
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

    if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') && !(c >= '0' && c <= '9') && c != '_'
        && !C99AllowedIDChar.contains(c))
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

    if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') && !(c >= '0' && c <= '9') && c != '_' && c != '.'
        && (((characters.back() | toLower) != 'e' && (characters.back() | toLower) != 'p') || (c != '+' && c != '-'))
        && !C99AllowedIDChar.contains(c))
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
        // TODO: Warning that the universal character is treated as backslash followed by identifier
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

    // suspHolder is either empty or a Text from here on

    // According to 6.4.2.1 Paragraph 3 of the C99 Standard these restrictions apply only to an identifier.
    // This means even if an identifier-nondigit element is allowed in a PP Number it doesn't have to meet those
    // requirements per my interpretation
    if (!C99AllowedIDChar.contains(*result) || !(suspHolder || !C99DisallowedInitialIDChar.contains(*result)))
    {
        // The value of the universal character cannot continue the Text that we have or is not able to start a new
        // Text. We therefore push Text if it's in suspHolder, push a backslash and push the rest as a new Text

        if (suspHolder)
        {
            return {Text{(big ? "U" : "u") + characters}, true};
        }

        context.report(coral66::Errors::Lexer::UNEXPECTED_CHARACTER, ucStart, std::pair{ucStart, context.getOffset()});
        return {Start{}, true};
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
        case static_cast<int>(TokenType::Assignment):
            if (c == '=')
            {
                context.push(TokenType::Equal);
                return {Start{}, true};
            }
            context.push(1, TokenType::Assignment);
            return {Start{}, false};
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
    const coral66::Lexer::PPToken& token;
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
    bool errorOccured = false;
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
            errorOccured = true;
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
                errorOccured = true;
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
            bool big = iter[1] == 'U';
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
                errorOccured = true;
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
                errorOccured = true;
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
                errorOccured = true;
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
                errorOccured = true;
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
                errorOccured = true;
                continue;
            }

            auto value = octalToValue({iter, static_cast<std::size_t>(lastOctal - iter)});
            if (value > largestCharacter)
            {
                auto start = context.token.getCharSpaceOffset() + (wide ? 2 : 1) + (iter - characters.data() - 1);
                context.report(coral66::Errors::Lexer::CHARACTER_TOO_LARGE_FOR_LITERAL_TYPE, start,
                               std::forward_as_tuple(context.token, start, start + lastOctal - iter));
                errorOccured = true;
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
                errorOccured = true;
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
                errorOccured = true;
            }
        }
    }
    result.resize(std::distance(result.data(), resultStart));
    return {result, errorOccured};
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
                // Identifiers need to always be valid
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
                bool wide = false; // No wide support
                auto [chars, errorOccured] = processCharacters(iter->getValue(), context, wide, Literal::CharLiteral);
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

std::optional<coral66::Lexer::CToken> coral66::Lexer::parseStringLiteral(const PPToken& ppToken,
                                                                         const PPSourceInterface& sourceInterface,
                                                                         llvm::raw_ostream* reporter)
{
    CORAL66_ASSERT(ppToken.getTokenType() == coral66::Lexer::TokenType::StringLiteral);
    bool errorsOccurred = false;
    ConversionContext context{reporter, sourceInterface, ppToken, &errorsOccurred};
    bool wide = ppToken.getRepresentation(sourceInterface)[0] == 'L';
    auto [chars, errorOccured] = processCharacters(ppToken.getValue(), context, wide, Literal::StringLiteral);
    if (errorOccured)
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
