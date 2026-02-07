/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include "lexer_common.hpp"
#include "lib/frontend/common/language_options.hpp"
#include "lib/frontend/common/source_interface.hpp"
#include "lib/support/Utilities.hpp"

/**
 * @file SourceObject.hpp
 * @brief Defines the SourceObject class, a concrete implementation of SourceInterface.
 *
 * SourceObject acts as the central storage for all source-related data for a translation unit.
 * It holds:
 * - The stream of tokens (either CTokens or PPTokens)
 * - The list of source files and their content
 * - Records of preprocessor substitutions
 *
 * It provides efficient lookups for line numbers and source locations, mapping tokens back
 * to the original source text.
 */

namespace coral66
{
namespace Source
{
struct File
{
    std::string path;
    std::string source;
    std::vector<std::uint64_t> starts;
    bool systemHeader;
    std::optional<std::pair<std::uint32_t, std::uint64_t>> includedBy{};
    std::vector<std::tuple<std::uint64_t, std::optional<std::string>, std::uint64_t>> lineAndFileMapping{};
};

struct Substitution
{
    Lexer::PPToken macroIdentifier;
    Lexer::PPToken replacedIdentifier;
    std::optional<Lexer::PPToken> closeParentheses;
    bool empty;
};

struct Stringification
{
    std::vector<Lexer::PPToken> stringified;
    Lexer::PPToken replacedIdentifier;
};

struct TokenConcatenation
{
    Lexer::PPToken leftToken;
    Lexer::PPToken rightToken;
};
} // namespace Source

namespace detail
{
template <class T>
class SourceObjectStorage;

template <>
class SourceObjectStorage<Lexer::CToken> : public source_interface
{
    LanguageOptions m_languageOptions = LanguageOptions::native();

public:
    SourceObjectStorage() = default;

    SourceObjectStorage(LanguageOptions&& languageOptions) : m_languageOptions(std::move(languageOptions)) {}

    [[nodiscard]] const LanguageOptions& getLanguageOptions() const noexcept override
    {
        return m_languageOptions;
    }
};

template <>
class SourceObjectStorage<Lexer::PPToken> : public PPSourceInterface
{
    std::vector<Lexer::IntervalMap> m_intervalMap;
    const LanguageOptions* m_languageOptions;

public:
    SourceObjectStorage() = default;

    SourceObjectStorage(std::vector<Lexer::IntervalMap> intervalMap,
                        not_null<const LanguageOptions> const languageOptions)
        : m_intervalMap(std::move(intervalMap)), m_languageOptions(languageOptions)
    {
    }

    std::span<const Lexer::IntervalMap> getIntervalMaps() const noexcept override
    {
        return m_intervalMap;
    }

    std::vector<Lexer::IntervalMap>& getIntervalMap()
    {
        return m_intervalMap;
    }

    [[nodiscard]] const LanguageOptions& getLanguageOptions() const noexcept override
    {
        CORAL66_ASSERT(m_languageOptions);
        return *m_languageOptions;
    }
};
} // namespace detail

template <class T>
class SourceObject final : public detail::SourceObjectStorage<T>
{
    std::vector<T> m_tokens;
    std::vector<Source::File> m_files;
    std::vector<Source::PPRecord> m_substitutions;

public:
    SourceObject() = default;

    template <class U = T, std::enable_if_t<!std::is_same_v<Lexer::PPToken, U>>* = nullptr>
    SourceObject(std::vector<T> tokens, std::vector<Source::File> files, LanguageOptions languageOptions,
                 std::vector<Source::PPRecord> substitutions)
        : detail::SourceObjectStorage<T>(std::move(languageOptions)),
          m_tokens(std::move(tokens)),
          m_files(std::move(files)),
          m_substitutions(std::move(substitutions))
    {
    }

    template <class U = T, std::enable_if_t<std::is_same_v<Lexer::PPToken, U>>* = nullptr>
    SourceObject(std::vector<T> tokens, std::vector<Source::File> files,
                 not_null<const LanguageOptions> languageOptions, std::vector<Source::PPRecord> substitutions,
                 std::vector<Lexer::IntervalMap> intervalMaps)
        : detail::SourceObjectStorage<T>(std::move(intervalMaps), languageOptions),
          m_tokens(std::move(tokens)),
          m_files(std::move(files)),
          m_substitutions(std::move(substitutions))
    {
    }

    ~SourceObject() override = default;
    SourceObject(const SourceObject&) = delete;
    SourceObject& operator=(const SourceObject&) = delete;
    SourceObject(SourceObject&&)
#if defined(__clang__)
        noexcept
#endif
        = default;
    SourceObject& operator=(SourceObject&&)
#if defined(__clang__)
        noexcept
#endif
        = default;

    [[nodiscard]] std::uint64_t getLineNumber(std::uint32_t const fileID,
                                              std::uint64_t const offset) const noexcept override
    {
        CORAL66_ASSERT(fileID < m_files.size());
        auto result = std::ranges::lower_bound(m_files[fileID].starts, offset);
        CORAL66_ASSERT(result != m_files[fileID].starts.end());
        return std::distance(m_files[fileID].starts.begin(), result) + (*result == offset ? 1 : 0);
    }

    [[nodiscard]] std::uint64_t getLineStartOffset(std::uint32_t const fileID,
                                                   std::uint64_t const line) const noexcept override
    {
        CORAL66_ASSERT(fileID < m_files.size());
        CORAL66_ASSERT(line - 1 < m_files[fileID].starts.size());
        return m_files[fileID].starts[line - 1];
    }

    [[nodiscard]] std::uint64_t getLineEndOffset(std::uint32_t const fileID,
                                                 std::uint64_t const line) const noexcept override
    {
        CORAL66_ASSERT(fileID < m_files.size());
        CORAL66_ASSERT(line < m_files[fileID].starts.size());
        return m_files[fileID].starts[line] - 1;
    }

    [[nodiscard]] const std::vector<T>& data() const noexcept
    {
        return m_tokens;
    }

    [[nodiscard]] std::vector<T>& data() noexcept
    {
        return m_tokens;
    }

    [[nodiscard]] std::span<const Source::PPRecord> getSubstitutions() const noexcept override
    {
        return m_substitutions;
    }

    [[nodiscard]] std::vector<Source::PPRecord>& getSubstitutions() noexcept
    {
        return m_substitutions;
    }

    [[nodiscard]] std::span<const Source::File> getFiles() const noexcept override
    {
        return m_files;
    }

    [[nodiscard]] std::vector<Source::File>& getFiles() noexcept
    {
        return m_files;
    }
};

using CSourceObject = SourceObject<Lexer::CToken>;
using PPSourceObject = SourceObject<Lexer::PPToken>;

extern template class SourceObject<Lexer::CToken>;

extern template class SourceObject<Lexer::PPToken>;

} // namespace coral66
