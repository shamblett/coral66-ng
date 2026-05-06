/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

#include <span>
#include <variant>
#include <vector>

#include "lib/frontend/lexer/lexer.hpp"

/**
 * Defines the Preprocessor Syntax interface.
 *
 * This file contains the data structures representing the abstract syntax tree
 * for preprocessor directives, macro definitions, and text blocks.
 */

namespace coral66::pp
{
struct Group;

struct DefineDirective final
{
    lexer::PPTokenIterator defineToken;
    std::span<const lexer::PPToken> tokens;
};

struct ControlLine final
{
    std::variant<lexer::PPTokenIterator, DefineDirective> variant;
};

struct TextBlock final
{
    std::span<const lexer::PPToken> tokens;
};

using GroupPart = std::variant<TextBlock, ControlLine>;

struct Group final
{
    std::vector<GroupPart> groupPart;
};

struct File final
{
    std::vector<Group> groups;
};

} // namespace coral66::pp
