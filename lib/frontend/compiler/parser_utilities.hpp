/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

/**
 * Parser Utilities
 *
 */

#include <string_view>

#include "lib/support/FunctionRef.hpp"
#include "parser.hpp"

namespace coral66::Parser
{
bool isAssignment(Lexer::TokenType type);

bool expectIdentifier(Lexer::CTokenIterator& begin, Lexer::CTokenIterator end, Context& context,
                      std::string_view& value, coral66::FunctionRef<Message()> additional = {});

bool expect(Lexer::TokenType expected, Lexer::CTokenIterator& begin, Lexer::CTokenIterator end, Context& context,
            coral66::FunctionRef<Message()> additional = {});

bool firstIsInExternalDeclaration(const Lexer::CToken& token, const coral66::Parser::Context& context);

bool firstIsInDeclarator(const Lexer::CToken& token, const coral66::Parser::Context& context);

bool firstIsInDirectDeclarator(const Lexer::CToken& token, const coral66::Parser::Context& context);

bool firstIsInParameterTypeList(const Lexer::CToken& token, const coral66::Parser::Context& context);

bool firstIsInAbstractDeclarator(const Lexer::CToken& token, const coral66::Parser::Context& context);

bool firstIsInDirectAbstractDeclarator(const Lexer::CToken& token, const coral66::Parser::Context& context);

bool firstIsInPointer(const Lexer::CToken& token, const coral66::Parser::Context& context);

bool firstIsInCompoundItem(const Lexer::CToken& token, const coral66::Parser::Context& context);

bool firstIsInInitializer(const Lexer::CToken& token, const coral66::Parser::Context& context);

bool firstIsInInitializerList(const Lexer::CToken& token, const coral66::Parser::Context& context);

bool firstIsInStatement(const Lexer::CToken& token, const coral66::Parser::Context& context);

bool firstIsInExpression(const Lexer::CToken& token, const coral66::Parser::Context& context);

bool firstIsInAssignmentExpression(const Lexer::CToken& token, const coral66::Parser::Context& context);

bool firstIsInConditionalExpression(const Lexer::CToken& token, const coral66::Parser::Context& context);

bool firstIsInLogicalOrExpression(const Lexer::CToken& token, const coral66::Parser::Context& context);

bool firstIsInLogicalAndExpression(const Lexer::CToken& token, const coral66::Parser::Context& context);

bool firstIsInBitOrExpression(const Lexer::CToken& token, const coral66::Parser::Context& context);

bool firstIsInBitXorExpression(const Lexer::CToken& token, const coral66::Parser::Context& context);

bool firstIsInBitAndExpression(const Lexer::CToken& token, const coral66::Parser::Context& context);

bool firstIsInEqualityExpression(const Lexer::CToken& token, const coral66::Parser::Context& context);

bool firstIsInRelationalExpression(const Lexer::CToken& token, const coral66::Parser::Context& context);

bool firstIsInShiftExpression(const Lexer::CToken& token, const coral66::Parser::Context& context);

bool firstIsInAdditiveExpression(const Lexer::CToken& token, const coral66::Parser::Context& context);

bool firstIsInTerm(const Lexer::CToken& token, const coral66::Parser::Context& context);

bool firstIsInTypeName(const Lexer::CToken& token, const coral66::Parser::Context& context);

bool firstIsInCastExpression(const Lexer::CToken& token, const coral66::Parser::Context& context);

bool firstIsInUnaryExpression(const Lexer::CToken& token, const coral66::Parser::Context& context);

bool firstIsInPostFixExpression(const Lexer::CToken& token, const coral66::Parser::Context& context);

bool firstIsInPrimaryExpression(const Lexer::CToken& token, const coral66::Parser::Context& context);
} // namespace coral66::Parser
