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

#include "diagnostic.hpp"
#include "lexer_common.hpp"
#include "lib/frontend/common/source_interface.hpp"
#include "lib/support/utilities.hpp"

/**
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
    lexer::PPToken macroIdentifier;
    lexer::PPToken replacedIdentifier;
    std::optional<lexer::PPToken> closeParentheses;
    bool empty;
};

struct Stringification
{
    std::vector<lexer::PPToken> stringified;
    lexer::PPToken replacedIdentifier;
};

struct TokenConcatenation
{
    lexer::PPToken leftToken;
    lexer::PPToken rightToken;
};
} // namespace Source

namespace detail
{
template <class T>
class SourceObjectStorage;

template <>
class SourceObjectStorage<lexer::CToken> : public source_interface
{
public:
    SourceObjectStorage() = default;
};

template <>
class SourceObjectStorage<lexer::PPToken> : public PPSourceInterface
{
    std::vector<lexer::IntervalMap> m_intervalMap;

public:
    SourceObjectStorage() = default;

    explicit SourceObjectStorage(std::vector<lexer::IntervalMap> intervalMap) : m_intervalMap(std::move(intervalMap)) {}

    [[nodiscard]] std::span<const lexer::IntervalMap> getIntervalMaps() const noexcept override
    {
        return m_intervalMap;
    }

    std::vector<lexer::IntervalMap>& getIntervalMap()
    {
        return m_intervalMap;
    }
};
} // namespace detail

template <class T>
class SourceObject final : public detail::SourceObjectStorage<T>
{
    std::vector<T> m_tokens;
    std::vector<Source::File> m_files;
    std::vector<Source::PPRecord> m_substitutions;
    std::vector<detail::Diagnostic::DiagnosticBase> m_diagnostics;

public:
    SourceObject() = default;

    template <class U = T, std::enable_if_t<!std::is_same_v<lexer::PPToken, U>>* = nullptr>
    SourceObject(std::vector<T> tokens, std::vector<Source::File> files, std::vector<Source::PPRecord> substitutions)
        : m_tokens(std::move(tokens)), m_files(std::move(files)), m_substitutions(std::move(substitutions))
    {
    }

    template <class U = T, std::enable_if_t<std::is_same_v<lexer::PPToken, U>>* = nullptr>
    SourceObject(std::vector<T> tokens, std::vector<Source::File> files, std::vector<Source::PPRecord> substitutions,
                 std::vector<lexer::IntervalMap> intervalMaps)
        : detail::SourceObjectStorage<T>(std::move(intervalMaps)),
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

    template <typename DiagnosticType>
    void addDiagnostic(const DiagnosticType& diagnostic)
    {
        m_diagnostics.emplace_back(diagnostic);
    }

    template <typename DiagnosticType>
    bool findDiagnostic(const DiagnosticType& diagnostic) const
    {
        if (auto result = std::ranges::find_if(m_diagnostics.begin(), m_diagnostics.end(), [&diagnostic](auto const& item)
                                               { return item.getName() == diagnostic.getName(); });
            result != m_diagnostics.end())
        {
            return true;
        }
        return false;
    }
};

using CSourceObject = SourceObject<lexer::CToken>;
using PPSourceObject = SourceObject<lexer::PPToken>;

extern template class SourceObject<lexer::CToken>;

extern template class SourceObject<lexer::PPToken>;

} // namespace coral66
