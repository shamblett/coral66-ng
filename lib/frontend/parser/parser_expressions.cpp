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
 * Section 6
 *
 * Coral 66 expressions comprise  :-
 *
 * Simple expressions
 *
 * Primaries
 *
 * Word Logic
 *
 * Conditional Expressions
 *
 */

namespace
{
[[maybe_unused]] bool isPostFixOperator(const coral66::lexer::CToken& token)
{
    switch (token.getTokenType())
    {
        case coral66::lexer::TokenType::Dot:
        case coral66::lexer::TokenType::OpenSquareBracket:
        case coral66::lexer::TokenType::OpenParentheses:
        default: break;
    }
    return false;
}
} // namespace