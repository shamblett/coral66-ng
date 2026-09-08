/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

/**
 * Note definitions.
 *
 */

#include "lib/frontend/common/diagnostic.hpp"
#include "lib/support/constexpr.hpp"

namespace coral66::notes::pp
{
constexpr auto TO_MATCH_N_HERE = ::coral66::makeDiagnostic<"To match %0 here:", PointAt<0>>(Severity::Note, "");
constexpr auto PREVIOUSLY_DECLARED_HERE = ::coral66::makeDiagnostic<"Previously declared here:", Underline<0>>(Severity::Note, "");
} // namespace coral66::notes::pp
