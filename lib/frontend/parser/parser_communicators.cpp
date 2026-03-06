/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "parser.hpp"

#include <llvm/Support/ConvertUTF.h>

#include <algorithm>

#include "lib/frontend/common/messages/errors.hpp"

namespace coral66::parser
{
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

std::unique_ptr<Syntax::Communicator> parseCommunicator(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                        Context& context)
{
    std::unique_ptr<Syntax::Communicator> result;
    return result;
}
} // namespace coral66::parser
