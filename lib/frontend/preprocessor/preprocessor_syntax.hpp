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

/**
 * <GroupPart> == <TextBlock>
 */
using GroupPart = std::variant<TextBlock>;

/**
 * <Group> ::= <GroupPart> { <GroupPart> }
 */
struct Group final
{
    std::vector<GroupPart> groupPart;
};

/**
 * <File> := { <Group> }
 */
struct File final
{
    std::vector<Group> groups;
};

} // namespace coral66::PP
