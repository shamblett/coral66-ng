/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "parser_utilities.hpp"

#include "../common/messages/errors.hpp"

bool coral66::Parser::expectIdentifier(lexer::CTokenIterator& begin, const lexer::CTokenIterator end, Context& context,
                                       std::string_view& value, const coral66::FunctionRef<Message()> additional)
{
    if (begin == end || begin->getTokenType() != lexer::TokenType::Identifier)
    {
        if (begin == end)
        {
            context.log(coral66::Errors::Parser::EXPECTED_N.args(*(end - 1), context.getSourceInterface(),
                                                                 lexer::TokenType::Identifier, *(end - 1)));
        }
        else
        {
            context.log(coral66::Errors::Parser::EXPECTED_N_INSTEAD_OF_N.args(*begin, context.getSourceInterface(),
                                                                              lexer::TokenType::Identifier, *begin));
        }
        if (additional)
        {
            context.log(additional());
        }
        return false;
    }
    value = begin->getText();
    begin++;
    return true;
}

bool coral66::Parser::expect(lexer::TokenType expected, lexer::CTokenIterator& begin, const lexer::CTokenIterator end,
                             Context& context, const coral66::FunctionRef<Message()> additional)
{
    if (begin == end || begin->getTokenType() != expected)
    {
        if (begin == end)
        {
            context.log(coral66::Errors::Parser::EXPECTED_N.args(diag::after(*(end - 1)), context.getSourceInterface(),
                                                                 expected, *(end - 1)));
        }
        else
        {
            context.log(coral66::Errors::Parser::EXPECTED_N_INSTEAD_OF_N.args(*begin, context.getSourceInterface(),
                                                                              expected, *begin));
        }
        if (additional)
        {
            context.log(additional());
        }
        return false;
    }
    begin++;
    return true;
}

bool coral66::Parser::firstIsInDeclarator(const lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInPointer(token, context) || firstIsInDirectDeclarator(token, context);
}

bool coral66::Parser::firstIsInDirectDeclarator(const lexer::CToken& token, const coral66::Parser::Context&)
{
    return token.getTokenType() == lexer::TokenType::Identifier
           || token.getTokenType() == lexer::TokenType::OpenParentheses;
}

bool coral66::Parser::firstIsInAbstractDeclarator(const lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInPointer(token, context) || firstIsInDirectAbstractDeclarator(token, context);
}

bool coral66::Parser::firstIsInDirectAbstractDeclarator(const lexer::CToken& token, const coral66::Parser::Context&)
{
    return token.getTokenType() == lexer::TokenType::OpenParentheses
           || token.getTokenType() == lexer::TokenType::OpenSquareBracket;
}

bool coral66::Parser::firstIsInPointer(const lexer::CToken& token, const coral66::Parser::Context&)
{
    return token.getTokenType() == lexer::TokenType::Asterisk;
}

bool coral66::Parser::firstIsInInitializer(const lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInAssignmentExpression(token, context) || token.getTokenType() == lexer::TokenType::OpenBrace;
}

[[maybe_unused]] bool coral66::Parser::firstIsInInitializerList(const lexer::CToken& token,
                                                                const coral66::Parser::Context& context)
{
    return token.getTokenType() == lexer::TokenType::OpenSquareBracket || token.getTokenType() == lexer::TokenType::Dot
           || firstIsInInitializer(token, context);
}

bool coral66::Parser::firstIsInStatement(const lexer::CToken& token, const coral66::Parser::Context& context)
{
    return token.getTokenType() == lexer::TokenType::IfKeyword || token.getTokenType() == lexer::TokenType::ForKeyword
           || token.getTokenType() == lexer::TokenType::OpenBrace
           || token.getTokenType() == lexer::TokenType::Identifier
           || token.getTokenType() == lexer::TokenType::DoKeyword
           || token.getTokenType() == lexer::TokenType::GotoKeyword
           || token.getTokenType() == lexer::TokenType::SemiColon || firstIsInExpression(token, context);
}

bool coral66::Parser::firstIsInExpression(const lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInAssignmentExpression(token, context);
}

bool coral66::Parser::firstIsInAssignmentExpression(const lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInConditionalExpression(token, context);
}

bool coral66::Parser::firstIsInConditionalExpression(const lexer::CToken& token,
                                                     const coral66::Parser::Context& context)
{
    return firstIsInLogicalOrExpression(token, context);
}

bool coral66::Parser::firstIsInLogicalOrExpression(const lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInLogicalAndExpression(token, context);
}

bool coral66::Parser::firstIsInLogicalAndExpression(const lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInBitOrExpression(token, context);
}

bool coral66::Parser::firstIsInBitOrExpression(const lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInBitXorExpression(token, context);
}

bool coral66::Parser::firstIsInBitXorExpression(const lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInBitAndExpression(token, context);
}

bool coral66::Parser::firstIsInBitAndExpression(const lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInEqualityExpression(token, context);
}

bool coral66::Parser::firstIsInEqualityExpression(const lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInRelationalExpression(token, context);
}

bool coral66::Parser::firstIsInRelationalExpression(const lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInShiftExpression(token, context);
}

bool coral66::Parser::firstIsInShiftExpression(const lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInAdditiveExpression(token, context);
}

bool coral66::Parser::firstIsInAdditiveExpression(const lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInTerm(token, context);
}

bool coral66::Parser::firstIsInTerm(const lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInCastExpression(token, context);
}

bool coral66::Parser::firstIsInCastExpression(const lexer::CToken& token, const coral66::Parser::Context& context)
{
    return token.getTokenType() == lexer::TokenType::OpenParentheses || firstIsInUnaryExpression(token, context);
}

bool coral66::Parser::firstIsInUnaryExpression(const lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInPostFixExpression(token, context) || token.getTokenType() == lexer::TokenType::Asterisk
           || token.getTokenType() == lexer::TokenType::Plus || token.getTokenType() == lexer::TokenType::Minus;
}

bool coral66::Parser::firstIsInPostFixExpression(const lexer::CToken& token, const coral66::Parser::Context& context)
{
    return token.getTokenType() == lexer::TokenType::OpenParentheses || firstIsInPrimaryExpression(token, context);
}

bool coral66::Parser::firstIsInPrimaryExpression(const lexer::CToken& token, const coral66::Parser::Context&)
{
    return token.getTokenType() == lexer::TokenType::OpenParentheses
           || token.getTokenType() == lexer::TokenType::Identifier || token.getTokenType() == lexer::TokenType::Literal
           || token.getTokenType() == lexer::TokenType::StringLiteral;
}
