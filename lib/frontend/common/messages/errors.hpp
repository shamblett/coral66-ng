/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

/**
 * Error message definitions.
 *
 */

#include "lib/frontend/common/diagnostic.hpp"
#include "lib/support/constexpr.hpp"

#define CREATE_ERROR_2(variableName, format)                                                              \
    namespace detail                                                                                      \
    {                                                                                                     \
    constexpr auto CORAL66_PASTE2(variableName, Text) = ::coral66::Constexpr::basic_fixed_string{format}; \
    }                                                                                                     \
    constexpr auto variableName = ::coral66::makeDiagnostic<detail::CORAL66_PASTE2(variableName, Text)>(  \
        ::coral66::Severity::Error, #variableName)

#define CREATE_ERROR_3(...) CREATE_ERROR_VAARG(__VA_ARGS__)
#define CREATE_ERROR_4(...) CREATE_ERROR_VAARG(__VA_ARGS__)
#define CREATE_ERROR_5(...) CREATE_ERROR_VAARG(__VA_ARGS__)
#define CREATE_ERROR_6(...) CREATE_ERROR_VAARG(__VA_ARGS__)
#define CREATE_ERROR_7(...) CREATE_ERROR_VAARG(__VA_ARGS__)
#define CREATE_ERROR_8(...) CREATE_ERROR_VAARG(__VA_ARGS__)

#define CREATE_ERROR(...) CORAL66_PASTE2(CREATE_ERROR_, CORAL66_MACRO_COUNT_ARGUMENTS(__VA_ARGS__))(__VA_ARGS__)

#define CREATE_ERROR_VAARG(variableName, format, ...)                                                                 \
    namespace detail                                                                                                  \
    {                                                                                                                 \
    constexpr auto CORAL66_PASTE2(variableName, Text) = ::coral66::Constexpr::basic_fixed_string{format};             \
    }                                                                                                                 \
    constexpr auto variableName = ::coral66::makeDiagnostic<detail::CORAL66_PASTE2(variableName, Text), __VA_ARGS__>( \
        ::coral66::Severity::Error, #variableName)

#define CREATE_UNIQUE_ERROR_3(variableName, name, format)                                                 \
    namespace detail                                                                                      \
    {                                                                                                     \
    constexpr auto CORAL66_PASTE2(variableName, Text) = ::coral66::Constexpr::basic_fixed_string{format}; \
    }                                                                                                     \
    constexpr auto variableName =                                                                         \
        ::coral66::makeDiagnostic<detail::CORAL66_PASTE2(variableName, Text)>(::coral66::Severity::Error, name)

namespace coral66::errors
{
namespace parser
{
CREATE_ERROR(EXPECTED_N, "Expected %0", InsertAfter<1, 0>);
CREATE_ERROR(EXPECTED_CONDITION, "Expected Condition %0", InsertAfter<1, 0>);
CREATE_ERROR(EXPECTED_BEGIN, "Expected 'BEGIN' keyword", InsertAfter<0>);
CREATE_ERROR(EXPECTED_PROGRAM_SEGMENT, "Expected 'PROGRAM' or 'SEGMENT' keyword", InsertAfter<0>);
CREATE_ERROR(EXPECTED_END, "Expected 'END' keyword", InsertAfter<0>);
CREATE_ERROR(EXPECTED_SEMICOLON, "Expected semicolon", InsertAfter<0>);
CREATE_ERROR(EXPECTED_IDENTIFIER, "Expected identifier", InsertAfter<0>);
CREATE_ERROR(EXPECTED_LABELS, "Expected at least one label", InsertAfter<0>);
CREATE_ERROR(EXPECTED_EXPRESSION, "Expected expression", InsertAfter<0>);
CREATE_ERROR(EXPECTED_SIMPLE_EXPRESSION, "Expected simple expression", InsertAfter<0>);
CREATE_ERROR(EXPECTED_TERM, "Expected term", InsertAfter<0>);
CREATE_ERROR(EXPECTED_FACTOR, "Expected factor", InsertAfter<0>);
CREATE_ERROR(EXPECTED_CLOSE_PARENTHESIS, "Expected close parenthesis", InsertAfter<0>);
CREATE_ERROR(EXPECTED_OPEN_PARENTHESIS, "Expected open parenthesis", InsertAfter<0>);
CREATE_ERROR(EXPECTED_OPEN_SQUARE_BRACKET, "Expected open square bracket", InsertAfter<0>);
CREATE_ERROR(EXPECTED_CLOSE_SQUARE_BRACKET, "Expected close square bracket", InsertAfter<0>);
CREATE_ERROR(EXPECTED_TYPED_PRIMARY, "Expected typed primary", InsertAfter<0>);
CREATE_ERROR(EXPECTED_WORD_REFERENCE, "Expected word reference", InsertAfter<0>);
CREATE_ERROR(EXPECTED_VARIABLE, "Expected variable", InsertAfter<0>);
CREATE_ERROR(EXPECTED_PROCEDURE_CALL, "Expected procedure call", InsertAfter<0>);
CREATE_ERROR(EXPECTED_FRACTION_BITS, "Expected fraction bits", InsertAfter<0>);
CREATE_ERROR(EXPECTED_UNION, "Expected union", InsertAfter<0>);
CREATE_ERROR(EXPECTED_MASK, "Expected mask", InsertAfter<0>);
CREATE_ERROR(EXPECTED_STATEMENT, "Expected statement", InsertAfter<0>);
CREATE_ERROR(EXPECTED_SIMPLE_STATEMENT, "Expected simple statement", InsertAfter<0>);
CREATE_ERROR(EXPECTED_STRING_LITERAL, "Expected string literal", InsertAfter<0>);
CREATE_ERROR(EXPECTED_COLON, "Expected colon", InsertAfter<0>);
CREATE_ERROR(EXPECTED_TYPE, "Expected type", InsertAfter<0>);
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
CREATE_ERROR(ANSWER_OUTSIDE_PROCEDURE, "Answer statement used outside procedure body", InsertAfter<0>);
CREATE_ERROR(ANSWER_IN_UNTYPED_PROCEDURE, "Answer statement used inside untyped procedure", InsertAfter<0>);
CREATE_ERROR(MISSING_ANSWER_STATEMENT, "Typed procedure must contain an answer statement", InsertAfter<0>);
CREATE_ERROR(MALFORMED_COMMUNICATOR, "Invalid format for communicator", Underline<0>);
CREATE_ERROR(EXPECTED_N_AFTER_N, "Expected %0 after %1", InsertAfter<1, 0>);
} // namespace parser

namespace semantics
{
CREATE_ERROR(UNDECLARED_IDENTIFIER_N, "Undeclared identifier %0", Underline<0>);
CREATE_ERROR(REDEFINITION_OF_SYMBOL_N, "Redefinition of symbol %0", Underline<0>);
CREATE_ERROR(TYPE_MISMATCH, "Type mismatch in expression", Underline<0>);
CREATE_ERROR(EXPECTED_INTEGER_TYPE, "Expected INTEGER type", Underline<0>);
CREATE_ERROR(EXPECTED_FLOATING_TYPE, "Expected FLOATING type", Underline<0>);
CREATE_ERROR(EXPECTED_NUMERIC_TYPE, "Expected numeric type", Underline<0>);
CREATE_ERROR(INCORRECT_NUMBER_OF_INDICES, "Incorrect number of indices for array access", Underline<0>);
CREATE_ERROR(ARRAY_INDEX_OUT_OF_BOUNDS, "Array index out of bounds", Underline<0>);
CREATE_ERROR(INVALID_ARRAY_BOUNDS, "Invalid array bounds: lower bound must be <= upper bound", Underline<0>);
CREATE_ERROR(SWITCH_INDEX_OUT_OF_BOUNDS, "Switch index out of bounds", Underline<0>);
CREATE_ERROR(INCORRECT_PRESET_COUNT, "Incorrect number of presets for declaration", Underline<0>);
CREATE_ERROR(TABLE_INVALID_DIMENSIONS, "Table width and length must be strictly positive", Underline<0>);
CREATE_ERROR(CANNOT_ASSIGN_TO_CONST, "Cannot assign to constant or value parameter", Underline<0>);
CREATE_ERROR(TABLE_ELEMENT_NOT_FOUND_N, "Element %0 not found in table", Underline<0>);
CREATE_ERROR(INVALID_BIT_RANGE, "Invalid bit range specified", Underline<0>);
CREATE_ERROR(LABEL_NOT_FOUND_N, "Label %0 not found", Underline<0>);
CREATE_ERROR(PROCEDURE_ARGUMENT_COUNT_MISMATCH, "Incorrect number of arguments for procedure call", Underline<0>);
CREATE_ERROR(PROCEDURE_ARGUMENT_TYPE_MISMATCH, "Argument type mismatch", Underline<0>);
CREATE_ERROR(LOCATION_PARAMETER_REQUIRES_LVALUE, "LOCATION parameter requires a variable (L-value)", Underline<0>);
CREATE_ERROR(INVALID_SCALE_FOR_FIXED, "Invalid scale for FIXED point operation", Underline<0>);
CREATE_ERROR(RECURSIVE_CALL_ON_NON_RECURSIVE_PROCEDURE, "Procedure calls itself but is not declared RECURSIVE",
             Underline<0>);
CREATE_ERROR(PRESET_TYPE_MISMATCH, "Preset value type does not match declaration type", Underline<0>);
CREATE_ERROR(PRESET_FIXED_POINT_OVERFLOW, "Preset value exceeds fixed point scale bounds", Underline<0>);
} // namespace semantics

namespace CLI
{
CREATE_ERROR(FAILED_TO_OPEN_SOURCE_FILE, "Failed to open source file '%0'");
CREATE_ERROR(NO_SOURCE_FILES_SPECIFIED, "No source files specified");
CREATE_ERROR(SOURCE_FILE_EXTENSION_NOT_COR, "The source file extension is not .cor '%0'");
CREATE_ERROR(FAILED_TO_OPEN_FILE_FOR_OUTPUT, "Failed to open file '%0' for output");
CREATE_ERROR(FAILED_TO_GENERATE_TARGET_MACHINE, "Failed to generate LLVM target machine");
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

CREATE_ERROR(REDEFINITION_OF_MACRO_PARAMETER_N, "Redefinition of macro parameter %0", Underline<0>);

CREATE_ERROR(WHITESPACE_REQUIRED_AFTER_OBJECT_MACRO_DEFINITION, "Whitespace required after object macro definition",
             Underline<0>);

CREATE_ERROR(INFINITE_MACRO_RECURSION_DETECTED_N, "Infinite macro recursion detected for macro %0", Underline<0>);

CREATE_ERROR(NOT_ENOUGH_ARGUMENTS_FOR_MACRO_N_EXPECTED_N_GOT_N, "Not enough arguments for macro %0. Expected %1 got %2",
             Underline<0>);

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
