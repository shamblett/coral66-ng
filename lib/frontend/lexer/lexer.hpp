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
#include "lib/frontend/common/diagnostic.hpp"
#include "lib/frontend/common/lexer_common.hpp"
#include "lib/frontend/common/source_interface.hpp"
#include "lib/support/utilities.hpp"

/**
 * Defines the Lexer and Token interfaces.
 *
 * This file contains the definitions for token types, the base Token class,
 * and specialized token classes for the Preprocessor (PPToken) and the Coral66 language (CToken).
 * It also declares the main entry points for tokenizing source code.
 */

namespace coral66
{

template <class T>
class SourceObject;

using CSourceObject = SourceObject<lexer::CToken>;
using PPSourceObject = SourceObject<lexer::PPToken>;

namespace lexer
{
using TokenIterator = const TokenBase* CORAL66_NON_NULL;
using CTokenIterator = const CToken* CORAL66_NON_NULL;
using PPTokenIterator = const PPToken* CORAL66_NON_NULL;

/**
 * Tokenizes a raw source string into a stream of Preprocessor Tokens (PPTokens).
 *
 * @param source The source code string.
 * @param reporter Stream to report lexing errors to.
 * @param errorsOccurred Pointer to a bool set to true if errors occur.
 * @param sourcePath Path to the source file (for diagnostics).
 * @param clearDiagnostics Request to not clear diagnostics if false.
 * @return A SourceObject containing the PPTokens.
 */
PPSourceObject tokenize(std::string source, llvm::raw_ostream* reporter = &llvm::errs(), bool* errorsOccurred = nullptr,
                        std::string_view sourcePath = "<stdin>", bool clearDiagnostics = true);

/**
 * Converts a stream of PPTokens into a stream of CTokens.
 *
 * This typically involves running the preprocessor (macro expansion, etc.).
 *
 * @param sourceObject The input PPTokens.
 * @param reporter Stream to report errors to.
 * @param errorsOccurred Pointer to a bool set to true if errors occur.
 * @return A sourceObject containing the CTokens.
 */
CSourceObject toCTokens(PPSourceObject&& sourceObject, llvm::raw_ostream* reporter = &llvm::errs(),
                        bool* errorsOccurred = nullptr);

/**
 *
 * @param sourceObject The input PPTokens.
 * @param reporter stream to report errors to.
 * @param errorsOccurred Pointer to a bool set to true if errors occur.
 * @return A CSourceObject containing the CTokens.
 */
CSourceObject toCTokens(const PPSourceObject& sourceObject, llvm::raw_ostream* reporter = &llvm::errs(),
                        bool* errorsOccurred = nullptr);

/**
 * Converts a range of PPTokens to CTokens.
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

std::optional<CToken> parseStringLiteral(const PPToken& PPToken, const PPSourceInterface& sourceInterface,
                                         llvm::raw_ostream* reporter = nullptr);

bool needsWhitespaceInBetween(TokenType left, TokenType right) noexcept;

} // namespace lexer

namespace diagnostics
{
template <>
struct CustomFormat<U't', U'o', U'k', U'e', U'n', U'T', U'y', U'p', U'e'>
{
    template <class T>
    std::string operator()(const T& token) const
    {
        static_assert(std::is_base_of_v<lexer::TokenBase, T>, "Argument to %tokenType must be a token class");
        auto view = lexer::tokenName(token.getTokenType());
        return std::string(view.begin(), view.end());
    }
};

template <>
struct StringConverter<lexer::TokenType>
{
    template <class... Ctx>
    static std::string inFormat(const lexer::TokenType arg, Ctx&&... /*ctx*/)
    {
        return coral66::to_string(tokenName(arg));
    }

    template <class... Ctx>
    static std::string inArg(const lexer::TokenType arg, Ctx&&... /*ctx*/)
    {
        return coral66::to_string(tokenName(arg, false));
    }
};

template <>
struct StringConverter<lexer::TokenBase>
{
    static std::string inFormat(const lexer::TokenBase& arg, const source_interface& sourceInterface)
    {
        auto spelling = lexer::normalizeSpelling(arg.getRepresentation(sourceInterface));
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

    static std::string inArg(const lexer::TokenBase& arg, const source_interface& sourceInterface)
    {
        auto spelling = lexer::normalizeSpelling(arg.getRepresentation(sourceInterface));
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
struct StringConverter<lexer::CToken> : StringConverter<lexer::TokenBase>
{
};

template <>
struct StringConverter<lexer::PPToken> : StringConverter<lexer::TokenBase>
{
};

} // namespace diagnostics

} // namespace coral66
