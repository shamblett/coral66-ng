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

struct DefineDirective final
{
    Lexer::PPTokenIterator defineToken;
    std::span<const Lexer::PPToken> tokens;
};


struct ControlLine final {

    std::variant<Lexer::PPTokenIterator, DefineDirective> variant;
};

/**
 * <TextBlock> ::= <TOKEN> <TokenType::Newline> { <TOKEN> <TokenType::Newline> }
 */
struct TextBlock final
{
    std::span<const coral66::Lexer::PPToken> tokens;
};

/**
 * <GroupPart> == <TextBlock>
 */
using GroupPart = std::variant<TextBlock, ControlLine>;

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
