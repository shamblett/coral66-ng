/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

/**
 * Error message definitions for Coral 66
 *
 */

#include "lib/frontend/common/diagnostic.hpp"
#include "lib/support/constexpr.hpp"

#define CREATE_ERROR_2(variableName, format)                                                          \
    namespace detail                                                                                  \
    {                                                                                                 \
    constexpr auto P99_PASTE2(variableName, Text) = ::coral66::Constexpr::basic_fixed_string{format}; \
    }                                                                                                 \
    constexpr auto variableName =                                                                     \
        ::coral66::makeDiagnostic<detail::P99_PASTE2(variableName, Text)>(::coral66::Severity::Error, #variableName)

#define CREATE_ERROR_3(...) CREATE_ERROR_VAARG(__VA_ARGS__)
#define CREATE_ERROR_4(...) CREATE_ERROR_VAARG(__VA_ARGS__)
#define CREATE_ERROR_5(...) CREATE_ERROR_VAARG(__VA_ARGS__)
#define CREATE_ERROR_6(...) CREATE_ERROR_VAARG(__VA_ARGS__)
#define CREATE_ERROR_7(...) CREATE_ERROR_VAARG(__VA_ARGS__)
#define CREATE_ERROR_8(...) CREATE_ERROR_VAARG(__VA_ARGS__)

#define CREATE_ERROR(...) P99_PASTE2(CREATE_ERROR_, CORAL66_MACRO_COUNT_ARGUMENTS(__VA_ARGS__))(__VA_ARGS__)

#define CREATE_ERROR_VAARG(variableName, format, ...)                                                             \
    namespace detail                                                                                              \
    {                                                                                                             \
    constexpr auto P99_PASTE2(variableName, Text) = ::coral66::Constexpr::basic_fixed_string{format};             \
    }                                                                                                             \
    constexpr auto variableName = ::coral66::makeDiagnostic<detail::P99_PASTE2(variableName, Text), __VA_ARGS__>( \
        ::coral66::Severity::Error, #variableName)

#define CREATE_UNIQUE_ERROR_3(variableName, name, format)                                             \
    namespace detail                                                                                  \
    {                                                                                                 \
    constexpr auto P99_PASTE2(variableName, Text) = ::coral66::Constexpr::basic_fixed_string{format}; \
    }                                                                                                 \
    constexpr auto variableName =                                                                     \
        ::coral66::makeDiagnostic<detail::P99_PASTE2(variableName, Text)>(::coral66::Severity::Error, name)

namespace coral66::errors
{
namespace parser
{
CREATE_ERROR(EXPECTED_N, "Expected %0", InsertAfter<1, 0>);
CREATE_ERROR(EXPECTED_CONDITION, "Expected Condition %0", InsertAfter<1, 0>);
CREATE_ERROR(EXPECTED_BEGIN, "Expected 'BEGIN' keyword", InsertAfter<0>);
CREATE_ERROR(EXPECTED_END, "Expected 'END' keyword", InsertAfter<0>);
CREATE_ERROR(EXPECTED_IDENTIFIER, "Expected identifier", InsertAfter<0>);
CREATE_ERROR(EXPECTED_EXPRESSION, "Expected expression", InsertAfter<0>);
CREATE_ERROR(EXPECTED_SIMPLE_EXPRESSION, "Expected simple expression", InsertAfter<0>);
CREATE_ERROR(EXPECTED_TERM, "Expected term", InsertAfter<0>);
CREATE_ERROR(EXPECTED_FACTOR, "Expected factor", InsertAfter<0>);
CREATE_ERROR(EXPECTED_CLOSED_PARENTHESIS, "Expected closed parenthesis", InsertAfter<0>);
CREATE_ERROR(EXPECTED_OPEN_PARENTHESIS, "Expected open parenthesis", InsertAfter<0>);
CREATE_ERROR(EXPECTED_OPEN_SQUARE_BRACKET, "Expected open square backet", InsertAfter<0>);
CREATE_ERROR(EXPECTED_TYPED_PRIMARY, "Expected typed primary", InsertAfter<0>);
CREATE_ERROR(EXPECTED_WORD_REFERENCE, "Expected word reference", InsertAfter<0>);
CREATE_ERROR(EXPECTED_FRACTION_BITS, "Expected fraction bits", InsertAfter<0>);
CREATE_ERROR(EXPECTED_UNION, "Expected union", InsertAfter<0>);
CREATE_ERROR(EXPECTED_MASK, "Expected mask", InsertAfter<0>);
CREATE_ERROR(EXPECTED_STATEMENT, "Expected statement", InsertAfter<0>);
CREATE_ERROR(EXPECTED_DECLARATION, "Expected declaration", InsertAfter<0>);
CREATE_ERROR(EXPECTED_COMPARATOR, "Expected comparator", InsertAfter<0>);
CREATE_ERROR(EXPECTED_ASSIGNMENT, "Expected assignment ':='", InsertAfter<0>);
CREATE_ERROR(EXPECTED_THEN, "Expected 'THEN' in IF statement", InsertAfter<0>);
CREATE_ERROR(EXPECTED_ELSE, "Expected 'ELSE' in IF statement", InsertAfter<0>);
CREATE_ERROR(EXPECTED_DO, "Expected 'DO' in FOR statement", InsertAfter<0>);
CREATE_ERROR(EXPECTED_UNTIL, "Expected 'UNTIL' in FOR statement", InsertAfter<0>);
CREATE_ERROR(EXPECTED_STEP, "Expected 'STEP' in FOR statement", InsertAfter<0>);
CREATE_ERROR(INVALID_FIXED_FORMAT, "Invalid format for FIXED type. Expected (total, fraction)", Underline<0>);
CREATE_ERROR(INVALID_BITS_FORMAT, "Invalid format for BITS type", Underline<0>);
CREATE_ERROR(EXPECTED_N_INSTEAD_OF_N, "Expected %0 instead of %1", PointAt<1>);
CREATE_ERROR(EXPECTED_N_AFTER_N, "Expected %0 after %1", InsertAfter<1, 0>);
} // namespace parser

namespace Semantics
{
CREATE_ERROR(UNDECLARED_IDENTIFIER_N, "Undeclared identifier %0", Underline<0>);
CREATE_ERROR(REDEFINITION_OF_SYMBOL_N, "Redefinition of symbol %0", Underline<0>);
CREATE_ERROR(TYPE_MISMATCH, "Type mismatch in expression", Underline<0>);
CREATE_ERROR(EXPECTED_INTEGER_TYPE, "Expected INTEGER type", AnnotateExpr<0>);
CREATE_ERROR(EXPECTED_FLOATING_TYPE, "Expected FLOATING type", AnnotateExpr<0>);
CREATE_ERROR(CANNOT_ASSIGN_TO_CONST, "Cannot assign to constant or value parameter", Underline<0>);
CREATE_ERROR(TABLE_ELEMENT_NOT_FOUND_N, "Element %0 not found in table", Underline<0>);
CREATE_ERROR(INVALID_BIT_RANGE, "Invalid bit range specified", Underline<0>);
CREATE_ERROR(LABEL_NOT_FOUND_N, "Label %0 not found", Underline<0>);
CREATE_ERROR(PROCEDURE_ARGUMENT_COUNT_MISMATCH, "Incorrect number of arguments for procedure call", Underline<0>);
CREATE_ERROR(PROCEDURE_ARGUMENT_TYPE_MISMATCH, "Argument type mismatch", AnnotateExpr<0>);
CREATE_ERROR(LOCATION_PARAMETER_REQUIRES_LVALUE, "LOCATION parameter requires a variable (L-value)", AnnotateExpr<0>);
CREATE_ERROR(INVALID_SCALE_FOR_FIXED, "Invalid scale for FIXED point operation", Underline<0>);
} // namespace Semantics

namespace CLI
{
CREATE_ERROR(FAILED_TO_OPEN_SOURCE_FILE, "Failed to open source file '%0'");
CREATE_ERROR(NO_SOURCE_FILES_SPECIFIED, "No source files specified");
CREATE_ERROR(SOURCE_FILE_EXTENSION_NOT_COR, "The source file extension is not .cor '%0'");
CREATE_ERROR(FAILED_TO_OPEN_FILE_FOR_OUTPUT, "Failed to open file '%0' for output");
} // namespace CLI

namespace lexer
{
// Adding some Lexer errors that might be used by Lexer.cpp
CREATE_ERROR(INVALID_UTF8_SEQUENCE, "Invalid UTF-8 Sequence", PointAt<0>);
CREATE_ERROR(UNEXPECTED_CHARACTER, "Unexpected character", PointAt<0>);
CREATE_ERROR(NON_PRINTABLE_CHARACTER_N, "Non printable character '%0'", PointAt<1>);
CREATE_ERROR(UNTERMINATED_STRING_LITERAL, "Unterminated string literal", Underline<0>);
CREATE_ERROR(UNTERMINATED_BRACKETED_COMMENT, "Unterminated bracketed comment", Underline<0>);
CREATE_ERROR(UNTERMINATED_STROPPED_KEYWORD, "Unterminated stropped keyword", Underline<0>);
CREATE_ERROR(INVALID_STROPPED_KEYWORD_N, "Invalid stropped keyword '%0'", PointAt<1>);
CREATE_ERROR(UNTERMINATED_COMMENT_KEYWORD_SEQUENCE, "Unterminated COMMENT keyword sequence", Underline<0>);
CREATE_ERROR(MALFORMED_END_KEYWORD_SEQUENCE, "Malformed END keyword sequence", Underline<0>);
CREATE_ERROR(MALFORMED_NUMBER_LITERAL, "Malformed number literal", Underline<0>);
} // namespace lexer

namespace pp
{

CREATE_ERROR(KEYWORD_CANNOT_BE_A_MACRO_IDENTIFIER, "Macro name %0 cannot be a keyword", InsertAfter<1, 0>);

CREATE_ERROR(REDEFINITION_OF_MACRO_PARAMETER_N, "Redefinition of macro parameter %0", Underline<0>);

CREATE_ERROR(WHITESPACE_REQUIRED_AFTER_OBJECT_MACRO_DEFINITION, "Whitespace required after object macro definition",
             Underline<0>);

CREATE_ERROR(DEFINED_CANNOT_BE_USED_AS_MACRO_NAME, "'defined' cannot be used as macro name", Underline<0>);

CREATE_ERROR(NOT_ENOUGH_ARGUMENTS_FOR_MACRO_N_EXPECTED_N_GOT_N, "Not enough arguments for macro %0. Expected %1 got %2",
             Underline<0>);

CREATE_ERROR(NOT_ENOUGH_ARGUMENTS_FOR_MACRO_N_EXPECTED_AT_LEAST_N_GOT_N,
             "Not enough arguments for macro %0. Expected at least %1 got %2", Underline<0>);

CREATE_ERROR(TOO_MANY_ARGUMENTS_FOR_MACRO_N_EXPECTED_N_GOT_N, "Too many arguments for macro %0. Expected %1 got %2",
             Underline<0>, PointAt<3>);

CREATE_ERROR(PP_EXPECTED_N_INSTEAD_OF_N, "Expected %0 instead of %1", PointAt<1>);
CREATE_ERROR(PP_EXPECTED_N_AFTER_N, "Expected %0 after %1", InsertAfter<1, 0>);
CREATE_ERROR(PP_EXPECTED_N, "Expected %0", InsertAfter<1, 0>);
} // namespace pp

} // namespace coral66::errors

// coral66

#undef CREATE_ERROR
#undef CREATE_ERROR_2
#undef CREATE_ERROR_3
#undef CREATE_ERROR_4
#undef CREATE_ERROR_5
#undef CREATE_ERROR_6
#undef CREATE_ERROR_7
#undef CREATE_ERROR_8
#undef CREATE_ERROR_VAARG
#undef CREATE_UNIQUE_ERROR
#undef CREATE_UNIQUE_ERROR_3
#undef CREATE_UNIQUE_ERROR_4
#undef CREATE_UNIQUE_ERROR_5
#undef CREATE_UNIQUE_ERROR_6
#undef CREATE_UNIQUE_ERROR_7
#undef CREATE_UNIQUE_ERROR_8
#undef CREATE_UNIQUE_ERROR_9
#undef CREATE_UNIQUE_ERROR_VAARG
