/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "parser_utilities.hpp"

#include "lib/frontend/common/messages/errors.hpp"

bool coral66::parser::expectIdentifier(lexer::CTokenIterator& begin, const lexer::CTokenIterator end, Context& context,
                                       std::string_view& value, const coral66::FunctionRef<Message()> additional)
{
    if (begin == end || begin->getTokenType() != lexer::TokenType::Identifier)
    {
        if (begin == end)
        {
            context.log(coral66::errors::parser::EXPECTED_N.args(*(end - 1), context.getSourceInterface(),
                                                                 lexer::TokenType::Identifier, *(end - 1)));
        }
        else
        {
            context.log(coral66::errors::parser::EXPECTED_N_INSTEAD_OF_N.args(*begin, context.getSourceInterface(),
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

bool coral66::parser::expect(lexer::TokenType expected, lexer::CTokenIterator& begin, const lexer::CTokenIterator end,
                             Context& context, const coral66::FunctionRef<Message()> additional)
{
    if (begin == end || begin->getTokenType() != expected)
    {
        if (begin == end)
        {
            context.log(coral66::errors::parser::EXPECTED_N.args(diag::after(*(end - 1)), context.getSourceInterface(),
                                                                 expected, *(end - 1)));
        }
        else
        {
            context.log(coral66::errors::parser::EXPECTED_N_INSTEAD_OF_N.args(*begin, context.getSourceInterface(),
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

bool coral66::parser::firstIsInDeclarator(const lexer::CToken& token, const coral66::parser::Context& context)
{
    return firstIsInPointer(token, context) || firstIsInDirectDeclarator(token, context);
}

bool coral66::parser::firstIsInDirectDeclarator(const lexer::CToken& token, const coral66::parser::Context&)
{
    return token.getTokenType() == lexer::TokenType::Identifier
           || token.getTokenType() == lexer::TokenType::OpenParentheses;
}

bool coral66::parser::firstIsInAbstractDeclarator(const lexer::CToken& token, const coral66::parser::Context& context)
{
    return firstIsInPointer(token, context) || firstIsInDirectAbstractDeclarator(token, context);
}

bool coral66::parser::firstIsInDirectAbstractDeclarator(const lexer::CToken& token, const coral66::parser::Context&)
{
    return token.getTokenType() == lexer::TokenType::OpenParentheses
           || token.getTokenType() == lexer::TokenType::OpenSquareBracket;
}

bool coral66::parser::firstIsInPointer(const lexer::CToken& token, const coral66::parser::Context&)
{
    return token.getTokenType() == lexer::TokenType::Asterisk;
}

bool coral66::parser::firstIsInInitializer(const lexer::CToken& token, const coral66::parser::Context& context)
{
    return firstIsInAssignmentExpression(token, context);
}

[[maybe_unused]] bool coral66::parser::firstIsInInitializerList(const lexer::CToken& token,
                                                                const coral66::parser::Context& context)
{
    return token.getTokenType() == lexer::TokenType::OpenSquareBracket || token.getTokenType() == lexer::TokenType::Dot
           || firstIsInInitializer(token, context);
}

bool coral66::parser::firstIsInStatement(const lexer::CToken& token, const coral66::parser::Context& context)
{
    return token.getTokenType() == lexer::TokenType::IfKeyword || token.getTokenType() == lexer::TokenType::ForKeyword

           || token.getTokenType() == lexer::TokenType::Identifier
           || token.getTokenType() == lexer::TokenType::DoKeyword
           || token.getTokenType() == lexer::TokenType::GotoKeyword
           || token.getTokenType() == lexer::TokenType::SemiColon || firstIsInExpression(token, context);
}

bool coral66::parser::firstIsInExpression(const lexer::CToken& token, const coral66::parser::Context& context)
{
    return firstIsInAssignmentExpression(token, context);
}

bool coral66::parser::firstIsInAssignmentExpression(const lexer::CToken& token, const coral66::parser::Context& context)
{
    return firstIsInConditionalExpression(token, context);
}

bool coral66::parser::firstIsInConditionalExpression(const lexer::CToken& token,
                                                     const coral66::parser::Context& context)
{
    return firstIsInLogicalOrExpression(token, context);
}

bool coral66::parser::firstIsInLogicalOrExpression(const lexer::CToken& token, const coral66::parser::Context& context)
{
    return firstIsInLogicalAndExpression(token, context);
}

bool coral66::parser::firstIsInLogicalAndExpression(const lexer::CToken& token, const coral66::parser::Context& context)
{
    return firstIsInBitOrExpression(token, context);
}

bool coral66::parser::firstIsInBitOrExpression(const lexer::CToken& token, const coral66::parser::Context& context)
{
    return firstIsInBitXorExpression(token, context);
}

bool coral66::parser::firstIsInBitXorExpression(const lexer::CToken& token, const coral66::parser::Context& context)
{
    return firstIsInBitAndExpression(token, context);
}

bool coral66::parser::firstIsInBitAndExpression(const lexer::CToken& token, const coral66::parser::Context& context)
{
    return firstIsInEqualityExpression(token, context);
}

bool coral66::parser::firstIsInEqualityExpression(const lexer::CToken& token, const coral66::parser::Context& context)
{
    return firstIsInRelationalExpression(token, context);
}

bool coral66::parser::firstIsInRelationalExpression(const lexer::CToken& token, const coral66::parser::Context& context)
{
    return firstIsInShiftExpression(token, context);
}

bool coral66::parser::firstIsInShiftExpression(const lexer::CToken& token, const coral66::parser::Context& context)
{
    return firstIsInAdditiveExpression(token, context);
}

bool coral66::parser::firstIsInAdditiveExpression(const lexer::CToken& token, const coral66::parser::Context& context)
{
    return firstIsInTerm(token, context);
}

bool coral66::parser::firstIsInTerm(const lexer::CToken& token, const coral66::parser::Context& context)
{
    return firstIsInCastExpression(token, context);
}

bool coral66::parser::firstIsInCastExpression(const lexer::CToken& token, const coral66::parser::Context& context)
{
    return token.getTokenType() == lexer::TokenType::OpenParentheses || firstIsInUnaryExpression(token, context);
}

bool coral66::parser::firstIsInUnaryExpression(const lexer::CToken& token, const coral66::parser::Context& context)
{
    return firstIsInPostFixExpression(token, context) || token.getTokenType() == lexer::TokenType::Asterisk
           || token.getTokenType() == lexer::TokenType::Plus || token.getTokenType() == lexer::TokenType::Minus;
}

bool coral66::parser::firstIsInPostFixExpression(const lexer::CToken& token, const coral66::parser::Context& context)
{
    return token.getTokenType() == lexer::TokenType::OpenParentheses || firstIsInPrimaryExpression(token, context);
}

bool coral66::parser::firstIsInPrimaryExpression(const lexer::CToken& token, const coral66::parser::Context&)
{
    return token.getTokenType() == lexer::TokenType::OpenParentheses
           || token.getTokenType() == lexer::TokenType::Identifier || token.getTokenType() == lexer::TokenType::Literal
           || token.getTokenType() == lexer::TokenType::StringLiteral;
}
