/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <vector>

#include "Semantics.hpp"
#include "support/IntrusiveVariantValue.hpp"

namespace coral66::Semantics
{

class ProgramInterface
{
public:
    virtual ~ProgramInterface() = default;

    [[nodiscard]] virtual const LanguageOptions& getLanguageOptions() const = 0;

    // Add methods if needed for symbol lookup or size calculations
    // For now, minimal interface to satisfy getSizeOf signature
};

} // namespace coral66::Semantics
