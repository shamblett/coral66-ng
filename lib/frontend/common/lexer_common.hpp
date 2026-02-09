/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/APSInt.h>
#include <llvm/Support/raw_ostream.h>

#include <string>
#include <string_view>
#include <variant>

#include "lib/support/utilities.hpp"
#include "source_interface.hpp"

namespace coral66::Lexer
{
struct NonCharString;
class CToken;
class PPToken;

/**
 * @brief Enumeration of all possible token types in the C language and Preprocessor.
 */
enum class TokenType : std::uint8_t
{
    Identifier,
    OpenParentheses,
    CloseParentheses,
    OpenBrace,
    CloseBrace,
    Literal,
    StringLiteral,
    SemiColon,
    Comma,
    Minus,
    BitWiseNegation,
    LogicalNegation,
    Plus,
    Asterisk,
    Division,
    Percent,
    LogicAnd,
    LogicOr,
    Ampersand,
    BitOr,
    BitXor,
    Equal,
    NotEqual,
    LessThan,
    LessThanOrEqual,
    GreaterThan,
    GreaterThanOrEqual,
    Assignment,
    Colon,
    QuestionMark,
    Newline, ///<[PP]
    OpenSquareBracket,
    CloseSquareBracket,
    Dot,

    // Coral 66 Keywords
    BeginKeyword,
    EndKeyword,
    ProcedureKeyword,
    ProgramKeyword,
    SegmentKeyword,
    IfKeyword,
    ThenKeyword,
    ElseKeyword,
    ForKeyword,
    StepKeyword,
    UntilKeyword,
    DoKeyword,
    GotoKeyword,
    AnswerKeyword,
    IntegerKeyword,
    FloatingKeyword,
    FixedKeyword,
    BitsKeyword,
    ArrayKeyword,
    TableKeyword,
    PresetKeyword,
    OverlayKeyword,
    ValueKeyword,
    LocationKeyword,
    CommonKeyword,
    CommentKeyword,
    DefineKeyword,
    CodeKeyword,

    PPNumber,      ///<[PP]
    Backslash,     ///<[PP]
    Pound,         ///<[PP]
    DoublePound,   ///<[PP]
    Miscellaneous, ///<[PP]
    TOKEN_MAX_VALUE = Miscellaneous,
};

bool isText(TokenType tokenType);

/**
 * @param tokenType Token
 * @return name of the token. If the token is a punctuation it's surrounded in '
 */
std::string_view tokenName(TokenType tokenType);

/**
 * @param tokenType Token
 * @return generic value of the token. For non punctuations this is just a description. e.g. identifier
 */
std::string_view tokenValue(TokenType tokenType);

/**
 * Normalises the spelling of a token by removing backslash newline pairs and replacing trigraphs
 *
 * Does not change Digraphs as those are distinct and different spellings of a particular token type
 * @param tokenSpelling Spelling of a token
 * @return Normalized spelling of that token
 */
std::string normalizeSpelling(std::string_view tokenSpelling);

/**
 * @brief Base class for all tokens.
 *
 * Stores common information such as the token type, its position (offset, length, fileID),
 * and whether it was inserted by a macro.
 */
class TokenBase
{
protected:
    bool m_leadingWhitespace : 1;
    TokenType m_tokenType;   ///< Type of the token
    std::uint32_t m_macroId; ///< MacroID. All tokens with the same ID have been inserted by the same macro
    std::uint32_t m_fileID;
    std::uint64_t m_offset; ///< Offset of the token. That is bytes offset to the first character of the
    std::uint64_t m_length;

    TokenBase() = default;

    TokenBase(TokenType const tokenType, std::uint64_t const offset, std::uint64_t const length,
              std::uint32_t const fileID, std::uint32_t macroID)
        : m_leadingWhitespace(false),
          m_tokenType(tokenType),
          m_macroId(macroID),
          m_fileID(fileID),
          m_offset(offset),
          m_length(length)
    {
    }

public:
    [[nodiscard]] std::string_view getRepresentation(const source_interface& sourceObject) const;

    [[nodiscard]] std::uint64_t getLine(const source_interface& sourceObject) const noexcept;

    [[nodiscard]] std::uint64_t getColumn(const source_interface& sourceObject) const noexcept;

    [[nodiscard]] TokenType getTokenType() const noexcept
    {
        return m_tokenType;
    }

    [[nodiscard]] bool isMacroInserted() const noexcept
    {
        return static_cast<bool>(m_macroId);
    }

    [[nodiscard]] std::uint64_t getOffset() const noexcept
    {
        return m_offset;
    }

    [[nodiscard]] std::size_t getLength() const noexcept
    {
        return m_length;
    }

    [[nodiscard]] std::uint32_t getMacroId() const noexcept
    {
        return m_macroId;
    }

    void setMacroId(std::uint32_t macroId) noexcept
    {
        m_macroId = macroId;
    }

    [[nodiscard]] std::uint32_t getFileId() const
    {
        return m_fileID;
    }

    void setFileId(std::uint32_t fileId) noexcept
    {
        m_fileID = fileId;
    }

    [[nodiscard]] bool hasLeadingWhitespace() const noexcept
    {
        return m_leadingWhitespace;
    }

    void setLeadingWhitespace(bool leadingWhitespace) noexcept
    {
        m_leadingWhitespace = leadingWhitespace;
    }
};

/**
 * @brief A Preprocessor Token.
 *
 * Represents a token produced by the initial lexing phase. It contains raw string values
 * and information relevant to the preprocessor (like character space offsets).
 */
class PPToken final : public TokenBase
{
    std::string m_value;
    std::uint64_t m_charSpaceOffset;
    std::uint64_t m_charSpaceLength; /**< Length of the token after trigraphs and Backslash Newline pairs in its
                                      representation have been removed*/

public:
    PPToken(TokenType tokenType, std::uint64_t const offset, std::uint64_t const length,
            std::uint64_t const charSpaceOffset, std::uint64_t const charSpaceLength, std::uint32_t const fileID,
            std::uint32_t const macroID = 0, std::string_view const value = {})
        : TokenBase(tokenType, offset, length, fileID, macroID),
          m_value(value.begin(), value.end()),
          m_charSpaceOffset(charSpaceOffset),
          m_charSpaceLength(charSpaceLength)
    {
    }

    [[nodiscard]] std::uint64_t getCharSpaceOffset() const noexcept
    {
        return m_charSpaceOffset;
    }

    [[nodiscard]] std::uint64_t getCharSpaceLength() const
    {
        return m_charSpaceLength;
    }

    [[nodiscard]] std::string_view getValue() const noexcept
    {
        return m_value;
    }
};

struct NonCharString
{
    std::vector<std::uint32_t> characters;
    enum Type : std::uint8_t
    {
        Wide
    } type;

    bool operator==(const std::wstring& wideString) const
    {
        if (type != Wide)
        {
            return false;
        }
        return std::ranges::equal(characters, wideString);
    }

    bool operator==(const NonCharString& other) const
    {
        if (type != other.type)
        {
            return false;
        }
        return std::ranges::equal(characters, other.characters);
    }
};

/**
 * @brief A C Token.
 *
 * Represents a token after preprocessing, used by the parser. It may contain
 * parsed semantic values (like integer or floating-point values) in addition to the
 * basic token information.
 */
class CToken final : public TokenBase
{
    using variant = std::variant<std::monostate, llvm::APSInt, llvm::APFloat, std::string, NonCharString>;
    variant m_value; ///< Optional value of the token

public:
    enum Type : std::uint8_t
    {
        None,
        Integer,
        Floating,
        Fixed
    };

private:
    Type m_type;

public:
    using ValueType = variant;

    CToken(TokenType tokenType, std::uint64_t offset, std::uint64_t length, std::uint32_t fileId, std::uint32_t macroId,
           variant value = std::monostate{}, Type type = Type::None)
        : TokenBase(tokenType, offset, length, fileId, macroId), m_value(std::move(value)), m_type(type)
    {
    }

    [[nodiscard]] const variant& getValue() const noexcept
    {
        return m_value;
    }

    [[nodiscard]] std::string_view getText() const noexcept
    {
        return coral66::get<std::string>(m_value);
    }

    [[nodiscard]] Type getType() const noexcept
    {
        return m_type;
    }

    void setType(Type type) noexcept
    {
        m_type = type;
    }

    void setValue(const variant& value) noexcept
    {
        m_value = value;
    }
};
} // namespace coral66::Lexer
