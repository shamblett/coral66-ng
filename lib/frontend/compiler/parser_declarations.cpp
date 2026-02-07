/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "parser.hpp"

#include <algorithm>
#include <span>
#include <unordered_set>

#include "lib/frontend/common/error_messages.hpp"
#include "lib/support/ScopeExit.hpp"
#include "parser_utilities.hpp"
#include "semantic_utilities.hpp"
#include "semantics.hpp"

/**
 * @file ParserDeclarations.cpp
 * @brief Implements parsing functions for C declarations and statements.
 *
 * This file contains the implementations of the recursive descent parser for
 * top-level declarations (TranslationUnit, ExternalDeclaration), function
 * definitions, struct/union/enum specifiers, declarators, and statements
 * (CompoundStatement, etc.).
 */

//Empty for Coral66 at the moment