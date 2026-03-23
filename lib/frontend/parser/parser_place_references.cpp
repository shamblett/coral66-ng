/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 23/03/2026
 */

#include "parser.hpp"

#include <llvm/Support/ConvertUTF.h>

#include <algorithm>

#include "lib/frontend/common/messages/errors.hpp"
#include "parser_support.hpp"

namespace coral66::parser
{
/*
 * Section 5
 *
 * Coral 66 place references comprise  :-
 *
 * Switches
 *
 */

std::unique_ptr<Syntax::PlaceReference> parsePlaceReference(lexer::CTokenIterator& begin, lexer::CTokenIterator end,
                                                            Context& context)
{
    std::unique_ptr<Syntax::PlaceReference> result;
    return result;
}
} // namespace coral66::parser
