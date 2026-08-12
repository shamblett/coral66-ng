/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 17/03/2026
 */

#pragma once

#include <vector>

#include "lib/frontend/common/diagnostic_collection.hpp"

/*
 * Preprocessor support functions.
 */

namespace coral66::preprocessor
{

/*
 * Diagnostics collection helper.
 */
using DiagnosticCollector = DiagnosticCollection<detail::Diagnostic::DiagnosticBase>;

} // namespace coral66::preprocessor
