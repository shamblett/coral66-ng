/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "Parser.hpp"

/**
 * @file ParserExpressions.cpp
 * @brief Implements parsing functions for C expressions.
 *
 * This file contains the implementations of the recursive descent parser for
 * all Coral expression types, including assignment and primary expressions (literals, identifiers).
 */

#include <llvm/Support/ConvertUTF.h>

#include <algorithm>

#include "ParserUtilities.hpp"
#include "lib/frontend/common/error_messages.hpp"

using namespace coral66::Syntax;

namespace {
[[maybe_unused]] bool isPostFixOperator(const coral66::Lexer::CToken& token)
{
  switch (token.getTokenType())
  {
  case coral66::Lexer::TokenType::Dot:
  case coral66::Lexer::TokenType::OpenSquareBracket:
  case coral66::Lexer::TokenType::OpenParentheses:
  default: break;
  }
  return false;
}
}