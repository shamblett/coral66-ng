/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/APSInt.h>
#include <llvm/Support/raw_ostream.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

#include "lib/support/Utilities.hpp"
#include "lib/frontend/common/CustomDiagnostics.hpp"
#include "lib/frontend/common/LanguageOptions.hpp"
#include "lib/frontend/common/LexerCommon.hpp"

/**
 * @file Lexer.hpp
 * @brief Defines the Lexer and Token interfaces.
 *
 * This file contains the definitions for token types, the base Token class,
 * and specialized token classes for the Preprocessor (PPToken) and the C language (CToken).
 * It also declares the main entry points for tokenizing source code.
 */

namespace coral66
{


template <class T>
class SourceObject;

using CSourceObject = SourceObject<Lexer::CToken>;
using PPSourceObject = SourceObject<Lexer::PPToken>;

namespace Lexer
{
using TokenIterator = const TokenBase* CLD_NON_NULL;
using CTokenIterator = const CToken* CLD_NON_NULL;
using PPTokenIterator = const PPToken* CLD_NON_NULL;

/**
 * @brief Tokenizes a raw source string into a stream of Preprocessor Tokens (PPTokens).
 *
 * @param source The source code string.
 * @param languageOptions Options controlling the lexing behavior (e.g., C version).
 * @param reporter Stream to report lexing errors to.
 * @param errorsOccurred Pointer to a bool set to true if errors occur.
 * @param sourcePath Path to the source file (for diagnostics).
 * @return A SourceObject containing the PPTokens.
 */
PPSourceObject tokenize(std::string source, not_null<const LanguageOptions> languageOptions,
                        llvm::raw_ostream* reporter = &llvm::errs(), bool* errorsOccurred = nullptr,
                        std::string_view sourcePath = "<stdin>");

/**
 * @brief Converts a stream of PPTokens into a stream of CTokens.
 *
 * This typically involves running the preprocessor (macro expansion, etc.).
 *
 * @param sourceObject The input PPTokens.
 * @param reporter Stream to report errors to.
 * @param errorsOccurred Pointer to a bool set to true if errors occur.
 * @return A SourceObject containing the CTokens.
 */
CSourceObject toCTokens(PPSourceObject&& sourceObject, llvm::raw_ostream* reporter = &llvm::errs(),
                        bool* errorsOccurred = nullptr);

CSourceObject toCTokens(const PPSourceObject& sourceObject, llvm::raw_ostream* reporter = &llvm::errs(),
                        bool* errorsOccurred = nullptr);

/**
 * @brief Converts a range of PPTokens to CTokens.
 *
 * @param begin Iterator to the start of the PPToken range.
 * @param end Iterator to the end of the PPToken range.
 * @param sourceInterface Interface for accessing source code details.
 * @param reporter Stream to report errors to.
 * @param errorsOccurred Pointer to a bool set to true if errors occur.
 * @return A vector of CTokens.
 */
std::vector<CToken> toCTokens(PPTokenIterator begin, PPTokenIterator end, const PPSourceInterface& sourceInterface,
                              llvm::raw_ostream* reporter = &llvm::errs(), bool* errorsOccurred = nullptr);

using IntervalMap = std::vector<std::tuple<std::uint64_t, std::uint64_t, std::uint64_t>>;




static_assert(!std::is_polymorphic_v<PPToken>);

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
        return std::ranges::equal(characters,wideString);
    }

    bool operator==(const NonCharString& other) const
    {
        if (type != other.type)
        {
            return false;
        }
        return std::ranges::equal(characters,other.characters);
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

    static Type fromUnderlyingType(LanguageOptions::UnderlyingType const underlyingType)
    {
        switch (underlyingType)
        {
            case LanguageOptions::UnderlyingType::UnsignedShort: 
            case LanguageOptions::UnderlyingType::Int: 
            case LanguageOptions::UnderlyingType::UnsignedInt: 
            case LanguageOptions::UnderlyingType::Long: 
            case LanguageOptions::UnderlyingType::UnsignedLong: 
            case LanguageOptions::UnderlyingType::LongLong: 
            case LanguageOptions::UnderlyingType::UnsignedLongLong: return Integer;
        }
        CLD_UNREACHABLE;
    }

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

static_assert(!std::is_polymorphic_v<CToken>);

std::optional<CToken> parseStringLiteral(const PPToken& ppToken, const PPSourceInterface& sourceInterface,
                                         llvm::raw_ostream* reporter = nullptr);

bool needsWhitespaceInBetween(TokenType left, TokenType right) noexcept;

} // namespace Lexer

namespace diag
{
template <>
struct CustomFormat<U't', U'o', U'k', U'e', U'n', U'T', U'y', U'p', U'e'>
{
    template <class T>
    std::string operator()(const T& token) const
    {
        static_assert(std::is_base_of_v<Lexer::TokenBase, T>, "Argument to %tokenType must be a token class");
        auto view = Lexer::tokenName(token.getTokenType());
        return std::string(view.begin(), view.end());
    }
};

template <>
struct StringConverter<Lexer::TokenType>
{
    static std::string inFormat(Lexer::TokenType arg, const SourceInterface*)
    {
        return coral66::to_string(tokenName(arg));
    }

    static std::string inArg(Lexer::TokenType arg, const SourceInterface*)
    {
        return coral66::to_string(tokenValue(arg));
    }
};

template <>
struct StringConverter<Lexer::TokenBase>
{
    static std::string inFormat(const Lexer::TokenBase& arg, const SourceInterface* sourceInterface)
    {
        CLD_ASSERT(sourceInterface);
        auto spelling = Lexer::normalizeSpelling(arg.getRepresentation(*sourceInterface));
        for (std::size_t i = 0; i < spelling.size(); i++)
        {
            if (spelling[i] != '\n')
            {
                continue;
            }
            spelling.replace(i, 1, "\\n");
        }
        return "'" + spelling + "'";
    }

    static std::string inArg(const Lexer::TokenBase& arg, const SourceInterface* sourceInterface)
    {
        CLD_ASSERT(sourceInterface);
        auto spelling = Lexer::normalizeSpelling(arg.getRepresentation(*sourceInterface));
        for (std::size_t i = 0; i < spelling.size(); i++)
        {
            if (spelling[i] != '\n')
            {
                continue;
            }
            spelling.replace(i, 1, "\\n");
        }
        return spelling;
    }
};

template <>
struct StringConverter<Lexer::CToken> : StringConverter<Lexer::TokenBase>
{
};

template <>
struct StringConverter<Lexer::PPToken> : StringConverter<Lexer::TokenBase>
{
};

} // namespace diag

} // namespace coral66
