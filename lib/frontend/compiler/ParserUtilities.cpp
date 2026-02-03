/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#include "ParserUtilities.hpp"

#include "lib/frontend/common/ErrorMessages.hpp"

bool coral66::Parser::expectIdentifier(Lexer::CTokenIterator& begin, const Lexer::CTokenIterator end, Context& context,
                                   std::string_view& value, const coral66::FunctionRef<Message()> additional)
{
    if (begin == end || begin->getTokenType() != Lexer::TokenType::Identifier)
    {
        if (begin == end)
        {
            context.log(coral66::Errors::Parser::EXPECTED_N.args(*(end - 1), context.getSourceInterface(),
                                                             Lexer::TokenType::Identifier, *(end - 1)));
        }
        else
        {
            context.log(coral66::Errors::Parser::EXPECTED_N_INSTEAD_OF_N.args(*begin, context.getSourceInterface(),
                                                                          Lexer::TokenType::Identifier, *begin));
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

bool coral66::Parser::expect(Lexer::TokenType expected, Lexer::CTokenIterator& begin, const Lexer::CTokenIterator end,
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

bool coral66::Parser::firstIsInExternalDeclaration(const Lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInDeclaration(token, context) || firstIsInFunctionDefinition(token, context);
}

bool coral66::Parser::firstIsInFunctionDefinition(const Lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInDeclarationSpecifier(token, context);
}

bool coral66::Parser::firstIsInDeclaration(const Lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInDeclarationSpecifier(token, context);
}



bool coral66::Parser::firstIsInDeclarator(const Lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInPointer(token, context) || firstIsInDirectDeclarator(token, context);
}

bool coral66::Parser::firstIsInDirectDeclarator(const Lexer::CToken& token, const coral66::Parser::Context&)
{
    return token.getTokenType() == Lexer::TokenType::Identifier
           || token.getTokenType() == Lexer::TokenType::OpenParentheses;
}

[[maybe_unused]] bool coral66::Parser::firstIsInParameterTypeList(const Lexer::CToken& token,
                                                              const coral66::Parser::Context& context)
{
    return firstIsInParameterList(token, context);
}

bool coral66::Parser::firstIsInAbstractDeclarator(const Lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInPointer(token, context) || firstIsInDirectAbstractDeclarator(token, context);
}

bool coral66::Parser::firstIsInDirectAbstractDeclarator(const Lexer::CToken& token, const coral66::Parser::Context&)
{
    return token.getTokenType() == Lexer::TokenType::OpenParentheses
           || token.getTokenType() == Lexer::TokenType::OpenSquareBracket;
}

bool coral66::Parser::firstIsInParameterList(const Lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInDeclarationSpecifier(token, context);
}

bool coral66::Parser::firstIsInPointer(const Lexer::CToken& token, const coral66::Parser::Context&)
{
    return token.getTokenType() == Lexer::TokenType::Asterisk;
}

[[maybe_unused]] bool coral66::Parser::firstIsInCompoundItem(const Lexer::CToken& token,
                                                         const coral66::Parser::Context& context)
{
    return firstIsInDeclaration(token, context) || firstIsInStatement(token, context);
}

bool coral66::Parser::firstIsInInitializer(const Lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInAssignmentExpression(token, context) || token.getTokenType() == Lexer::TokenType::OpenBrace;
}

[[maybe_unused]] bool coral66::Parser::firstIsInInitializerList(const Lexer::CToken& token,
                                                            const coral66::Parser::Context& context)
{
    return token.getTokenType() == Lexer::TokenType::OpenSquareBracket || token.getTokenType() == Lexer::TokenType::Dot
           || firstIsInInitializer(token, context);
}

bool coral66::Parser::firstIsInStatement(const Lexer::CToken& token, const coral66::Parser::Context& context)
{
    return token.getTokenType() == Lexer::TokenType::IfKeyword || token.getTokenType() == Lexer::TokenType::ForKeyword
           || token.getTokenType() == Lexer::TokenType::OpenBrace
           || token.getTokenType() == Lexer::TokenType::Identifier
           || token.getTokenType() == Lexer::TokenType::DoKeyword
           || token.getTokenType() == Lexer::TokenType::GotoKeyword
           || token.getTokenType() == Lexer::TokenType::SemiColon || firstIsInExpression(token, context);
}

bool coral66::Parser::firstIsInExpression(const Lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInAssignmentExpression(token, context);
}

bool coral66::Parser::firstIsInAssignmentExpression(const Lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInConditionalExpression(token, context);
}

bool coral66::Parser::firstIsInConditionalExpression(const Lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInLogicalOrExpression(token, context);
}

bool coral66::Parser::firstIsInLogicalOrExpression(const Lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInLogicalAndExpression(token, context);
}

bool coral66::Parser::firstIsInLogicalAndExpression(const Lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInBitOrExpression(token, context);
}

bool coral66::Parser::firstIsInBitOrExpression(const Lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInBitXorExpression(token, context);
}

bool coral66::Parser::firstIsInBitXorExpression(const Lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInBitAndExpression(token, context);
}

bool coral66::Parser::firstIsInBitAndExpression(const Lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInEqualityExpression(token, context);
}

bool coral66::Parser::firstIsInEqualityExpression(const Lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInRelationalExpression(token, context);
}

bool coral66::Parser::firstIsInRelationalExpression(const Lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInShiftExpression(token, context);
}

bool coral66::Parser::firstIsInShiftExpression(const Lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInAdditiveExpression(token, context);
}

bool coral66::Parser::firstIsInAdditiveExpression(const Lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInTerm(token, context);
}

bool coral66::Parser::firstIsInTerm(const Lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInCastExpression(token, context);
}

[[maybe_unused]] bool coral66::Parser::firstIsInTypeName(const Lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInSpecifierQualifier(token, context);
}

bool coral66::Parser::firstIsInCastExpression(const Lexer::CToken& token, const coral66::Parser::Context& context)
{
    return token.getTokenType() == Lexer::TokenType::OpenParentheses || firstIsInUnaryExpression(token, context);
}

bool coral66::Parser::firstIsInUnaryExpression(const Lexer::CToken& token, const coral66::Parser::Context& context)
{
    return firstIsInPostFixExpression(token, context) ||
           token.getTokenType() == Lexer::TokenType::Ampersand
           || token.getTokenType() == Lexer::TokenType::Asterisk || token.getTokenType() == Lexer::TokenType::Plus
           || token.getTokenType() == Lexer::TokenType::Minus
           || token.getTokenType() == Lexer::TokenType::BitWiseNegation
           || token.getTokenType() == Lexer::TokenType::LogicalNegation;
}

bool coral66::Parser::firstIsInPostFixExpression(const Lexer::CToken& token, const coral66::Parser::Context& context)
{
    return token.getTokenType() == Lexer::TokenType::OpenParentheses || firstIsInPrimaryExpression(token, context);
}

bool coral66::Parser::firstIsInPrimaryExpression(const Lexer::CToken& token, const coral66::Parser::Context&)
{
    return token.getTokenType() == Lexer::TokenType::OpenParentheses
           || token.getTokenType() == Lexer::TokenType::Identifier || token.getTokenType() == Lexer::TokenType::Literal
           || token.getTokenType() == Lexer::TokenType::StringLiteral;
}
