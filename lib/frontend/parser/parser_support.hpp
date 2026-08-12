/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 18/03/2026
 */

#pragma once

#include "lib/frontend/common/diagnostic_collection.hpp"

/*
 * Parser support functions.
 */

namespace coral66::parser
{

/*
 * Diagnostics collection helper.
 */
using DiagnosticCollector = DiagnosticCollection<detail::Diagnostic::DiagnosticBase>;

} // namespace coral66::parser
