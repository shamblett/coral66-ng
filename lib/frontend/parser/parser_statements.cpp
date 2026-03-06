/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 06/03/2026
 */

#include "parser.hpp"

#include <llvm/Support/ConvertUTF.h>

#include <algorithm>

#include "lib/frontend/common/messages/errors.hpp"

using namespace coral66::Syntax;

/*
 * Section 5 and 7
 *
 * Coral 66 statements comprise :-
 *
 * Assignments
 *
 * Goto
 *
 * Procedure
 *
 * Answer
 *
 * Code
 *
 * Compound
 *
 * Blocks
 *
 * Dummy
 *
 * Conditional
 *
 * For
 *
 * SWitches
 */
