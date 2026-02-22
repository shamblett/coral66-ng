/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <memory>
#include <optional>
#include <span>
#include <variant>
#include <vector>

#include "lib/frontend/lexer/lexer.hpp"

namespace coral66::PP
{
struct Group;


/**
 * <TextBlock> ::= <TOKEN> <TokenType::Newline> { <TOKEN> <TokenType::Newline> }
 */
struct TextBlock final
{
    std::span<const Lexer::PPToken> tokens;
};

} // namespace coral66::PP
