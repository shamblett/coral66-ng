/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 24/04/2026
 */

#pragma once

#include "lib/frontend/common/diagnostic_collection.hpp"

/*
 * Semantic support functions.
 */

namespace coral66::semantics
{

/*
 * Diagnostics collection helper.
 */
using DiagnosticCollector = DiagnosticCollection<detail::Diagnostic::DiagnosticBase>;

} // namespace coral66::semantics
