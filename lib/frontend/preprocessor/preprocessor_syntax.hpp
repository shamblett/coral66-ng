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

namespace coral66::pp
{
struct Group;

struct DefineDirective final
{
    lexer::PPTokenIterator defineToken;
    std::span<const lexer::PPToken> tokens;
};

/**
 * <NonDirective> ::= <TokenType::Pound> <TOKEN> { <TOKEN> } <TokenType::Newline>
 */
struct NonDirective final
{
    lexer::PPTokenIterator begin;
    lexer::PPTokenIterator end;
};

struct UnknownDirective
{
    lexer::PPTokenIterator identifier;
};

struct ControlLine final
{
    std::variant<lexer::PPTokenIterator, DefineDirective> variant;
};

/**
 * <TextBlock> ::= <TOKEN> <TokenType::Newline> { <TOKEN> <TokenType::Newline> }
 */
struct TextBlock final
{
    std::span<const coral66::lexer::PPToken> tokens;
};

/**
 * <GroupPart> == <TextBlock>
 */
using GroupPart = std::variant<TextBlock, ControlLine, UnknownDirective, NonDirective>;

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

} // namespace coral66::pp
