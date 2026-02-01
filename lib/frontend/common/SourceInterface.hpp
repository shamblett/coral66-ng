/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <cstdint>
#include <span>
#include <variant>

#include "LanguageOptions.hpp"

/**
 * @file SourceInterface.hpp
 * @brief Defines the abstract interface for accessing source code information.
 *
 * The SourceInterface (and PPSourceInterface) provides an abstraction layer for
 * querying details about the source code, such as:
 * - Line numbers and column offsets
 * - File information
 * - Preprocessor macro substitutions
 *
 * This abstraction allows different parts of the compiler (Lexer, Preprocessor, Diagnostics)
 * to interact with the source code text and token streams uniformly.
 */

namespace coral66
{
namespace Lexer
{
class TokenBase;
using IntervalMap = std::vector<std::tuple<std::uint64_t, std::uint64_t, std::uint64_t>>;
} // namespace Lexer

namespace Source
{
struct File;
struct Substitution;
struct Stringification;
struct TokenConcatenation;
using PPRecord =
    std::variant<std::monostate, Source::Substitution, Source::Stringification, Source::TokenConcatenation>;
} // namespace Source

class SourceInterface
{
public:
    SourceInterface() = default;
    virtual ~SourceInterface() = default;
    SourceInterface(const SourceInterface&) = default;
    SourceInterface& operator=(const SourceInterface&) = default;
    SourceInterface(SourceInterface&&) noexcept = default;
    SourceInterface& operator=(SourceInterface&&) noexcept = default;

    [[nodiscard]] virtual std::uint64_t getLineNumber(std::uint32_t fileID, std::uint64_t offset) const noexcept = 0;

    [[nodiscard]] virtual std::uint64_t getLineStartOffset(std::uint32_t fileID, std::uint64_t line) const noexcept = 0;

    [[nodiscard]] virtual std::uint64_t getLineEndOffset(std::uint32_t fileID, std::uint64_t line) const noexcept = 0;

    [[nodiscard]] virtual std::span<const Source::File> getFiles() const noexcept = 0;

    [[nodiscard]] virtual std::span<const Source::PPRecord> getSubstitutions() const noexcept = 0;

    [[nodiscard]] virtual const LanguageOptions& getLanguageOptions() const noexcept = 0;
};

class PPSourceInterface : virtual public SourceInterface
{
public:
    PPSourceInterface() = default;
     ~PPSourceInterface()override = default;
    PPSourceInterface(const PPSourceInterface&) = default;
    PPSourceInterface& operator=(const PPSourceInterface&) = default;
    PPSourceInterface(PPSourceInterface&&) noexcept = default;
    PPSourceInterface& operator=(PPSourceInterface&&) noexcept = default;

    [[nodiscard]] virtual std::span<const Lexer::IntervalMap> getIntervalMaps() const noexcept = 0;
};
} // namespace coral66
