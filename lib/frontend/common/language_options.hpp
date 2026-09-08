/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

/**
 * Language definitions and options
 *
 */

#include <unordered_set>

#include "lib/support/triple.hpp"

namespace coral66
{
struct LanguageOptions
{
    std::unordered_set<std::string_view> enabledWarnings{};
    bool freeStanding{};
    enum class Extension
    {
        None,
    } extension{Extension::None};
    enum class BuiltInVaList
    {
        x86_64ABI
    } vaListKind{};
};

} // namespace coral66
