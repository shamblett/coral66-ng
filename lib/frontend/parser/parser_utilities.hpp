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

#include "lib/support/function_ref.hpp"
#include "parser.hpp"

namespace coral66::parser
{
bool isAssignment(lexer::TokenType type);

void validateFixedFormat(lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context);

bool expectIdentifier(lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context,
                      std::string_view& value, FunctionRef<Message()> additional = {});

bool expect(lexer::TokenType expected, lexer::CTokenIterator& begin, lexer::CTokenIterator end, Context& context,
            FunctionRef<Message()> additional = {});

bool firstIsInExternalDeclaration(const lexer::CToken& token, const Context& context);

bool firstIsInDeclarator(const lexer::CToken& token, const Context& context);

bool firstIsInDirectDeclarator(const lexer::CToken& token, const Context& context);

bool firstIsInParameterTypeList(const lexer::CToken& token, const Context& context);

bool firstIsInAbstractDeclarator(const lexer::CToken& token, const Context& context);

bool firstIsInDirectAbstractDeclarator(const lexer::CToken& token, const Context& context);

bool firstIsInPointer(const lexer::CToken& token, const Context& context);

bool firstIsInCompoundItem(const lexer::CToken& token, const Context& context);

bool firstIsInInitializer(const lexer::CToken& token, const Context& context);

bool firstIsInInitializerList(const lexer::CToken& token, const Context& context);

bool firstIsInStatement(const lexer::CToken& token, const Context& context);

bool firstIsInExpression(const lexer::CToken& token, const Context& context);

bool firstIsInAssignmentExpression(const lexer::CToken& token, const Context& context);

bool firstIsInConditionalExpression(const lexer::CToken& token, const Context& context);

bool firstIsInLogicalOrExpression(const lexer::CToken& token, const Context& context);

bool firstIsInLogicalAndExpression(const lexer::CToken& token, const Context& context);

bool firstIsInBitOrExpression(const lexer::CToken& token, const Context& context);

bool firstIsInBitXorExpression(const lexer::CToken& token, const Context& context);

bool firstIsInBitAndExpression(const lexer::CToken& token, const Context& context);

bool firstIsInEqualityExpression(const lexer::CToken& token, const Context& context);

bool firstIsInRelationalExpression(const lexer::CToken& token, const Context& context);

bool firstIsInShiftExpression(const lexer::CToken& token, const Context& context);

bool firstIsInAdditiveExpression(const lexer::CToken& token, const Context& context);

bool firstIsInTerm(const lexer::CToken& token, const Context& context);

bool firstIsInTypeName(const lexer::CToken& token, const Context& context);

bool firstIsInCastExpression(const lexer::CToken& token, const Context& context);

bool firstIsInUnaryExpression(const lexer::CToken& token, const Context& context);

bool firstIsInPostFixExpression(const lexer::CToken& token, const Context& context);

bool firstIsInPrimaryExpression(const lexer::CToken& token, const Context& context);

inline const char* parameterModeToString(Syntax::ParameterDeclaration::ParameterMode mode)
{
    using Mode = Syntax::ParameterDeclaration::ParameterMode;
    switch (mode)
    {
        case Mode::Value: return "Value";
        case Mode::Location: return "Location";
        case Mode::Array: return "Array";
        case Mode::Label: return "Label";
        case Mode::Switch: return "Switch";
        case Mode::Procedure: return "Procedure";
        case Mode::UntypedValue: return "UntypedValue";
        case Mode::UntypedLocation: return "UntypedLocation";
        default: return "Unknown";
    }
}

} // namespace coral66::parser
