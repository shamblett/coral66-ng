/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "parser.hpp"

#include <llvm/Support/ConvertUTF.h>

#include <algorithm>

#include "lib/frontend/common/messages/errors.hpp"

using namespace coral66::Syntax;

/*
 * Section 9
 *
 * Coral 66 communicators comprise  :-
 *
 * Common
 *
 * Library
 *
 * External
 *
 * Absolute
 *
 */
