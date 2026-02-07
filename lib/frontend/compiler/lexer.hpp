/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "lib/frontend/common/custom_diagnostics.hpp"
#include "lib/frontend/common/language_options.hpp"
#include "lib/frontend/common/lexer_common.hpp"
#include "lib/support/utilities.hpp"

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
using TokenIterator = const TokenBase* CORAL66_NON_NULL;
using CTokenIterator = const CToken* CORAL66_NON_NULL;
using PPTokenIterator = const PPToken* CORAL66_NON_NULL;

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
    static std::string inFormat(Lexer::TokenType arg, const source_interface*)
    {
        return coral66::to_string(tokenName(arg));
    }

    static std::string inArg(Lexer::TokenType arg, const source_interface*)
    {
        return coral66::to_string(tokenValue(arg));
    }
};

template <>
struct StringConverter<Lexer::TokenBase>
{
    static std::string inFormat(const Lexer::TokenBase& arg, const source_interface* sourceInterface)
    {
        CORAL66_ASSERT(sourceInterface);
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

    static std::string inArg(const Lexer::TokenBase& arg, const source_interface* sourceInterface)
    {
        CORAL66_ASSERT(sourceInterface);
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
