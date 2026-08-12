/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include "semantics.hpp"

namespace coral66::semantics
{

class ProgramInterface
{
public:
    virtual ~ProgramInterface() = default;

    // Add methods if needed for symbol lookup or size calculations
    // For now, minimal interface to satisfy getSizeOf signature
};

} // namespace coral66::semantics
