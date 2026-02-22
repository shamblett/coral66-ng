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
        ::coral66::makeDiagnostic<detail::P99_PASTE2(variableName, Text)>(::coral66::Severity::Error, "")

#define CREATE_ERROR_3(...) CREATE_ERROR_VAARG(__VA_ARGS__)
#define CREATE_ERROR_4(...) CREATE_ERROR_VAARG(__VA_ARGS__)
#define CREATE_ERROR_5(...) CREATE_ERROR_VAARG(__VA_ARGS__)
#define CREATE_ERROR_6(...) CREATE_ERROR_VAARG(__VA_ARGS__)
#define CREATE_ERROR_7(...) CREATE_ERROR_VAARG(__VA_ARGS__)
#define CREATE_ERROR_8(...) CREATE_ERROR_VAARG(__VA_ARGS__)

#define CREATE_ERROR(...) P99_PASTE2(CREATE_ERROR_, CORAL66_MACRO_COUNT_ARGUMENTS(__VA_ARGS__))(__VA_ARGS__)

#define CREATE_ERROR_VAARG(variableName, format, ...)                                                 \
    namespace detail                                                                                  \
    {                                                                                                 \
    constexpr auto P99_PASTE2(variableName, Text) = ::coral66::Constexpr::basic_fixed_string{format}; \
    }                                                                                                 \
    constexpr auto variableName =                                                                     \
        ::coral66::makeDiagnostic<detail::P99_PASTE2(variableName, Text), __VA_ARGS__>(::coral66::Severity::Error, "")

namespace coral66::Errors
{
namespace Parser
{
CREATE_ERROR(EXPECTED_N, "Expected %0", InsertAfter<1, 0>);
CREATE_ERROR(EXPECTED_BEGIN, "Expected 'BEGIN' keyword", InsertAfter<0>);
CREATE_ERROR(EXPECTED_END, "Expected 'END' keyword", InsertAfter<0>);
CREATE_ERROR(EXPECTED_IDENTIFIER, "Expected identifier", InsertAfter<0>);
CREATE_ERROR(EXPECTED_EXPRESSION, "Expected expression", InsertAfter<0>);
CREATE_ERROR(EXPECTED_STATEMENT, "Expected statement", InsertAfter<0>);
CREATE_ERROR(EXPECTED_DECLARATION, "Expected declaration", InsertAfter<0>);
CREATE_ERROR(EXPECTED_ASSIGNMENT, "Expected assignment ':='", InsertAfter<0>);
CREATE_ERROR(EXPECTED_THEN, "Expected 'THEN' in IF statement", InsertAfter<0>);
CREATE_ERROR(EXPECTED_DO, "Expected 'DO' in FOR statement", InsertAfter<0>);
CREATE_ERROR(EXPECTED_UNTIL, "Expected 'UNTIL' in FOR statement", InsertAfter<0>);
CREATE_ERROR(EXPECTED_STEP, "Expected 'STEP' in FOR statement", InsertAfter<0>);
CREATE_ERROR(INVALID_FIXED_FORMAT, "Invalid format for FIXED type. Expected (total, fraction)", Underline<0>);
CREATE_ERROR(INVALID_BITS_FORMAT, "Invalid format for BITS type", Underline<0>);
CREATE_ERROR(EXPECTED_N_INSTEAD_OF_N, "Expected %0 instead of %1", PointAt<1>);
CREATE_ERROR(EXPECTED_N_AFTER_N, "Expected %0 after %1", InsertAfter<1, 0>);
} // namespace Parser

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

namespace Lexer
{
// Adding some Lexer errors that might be used by Lexer.cpp
CREATE_ERROR(INVALID_OCTAL_CHARACTER, "Invalid octal character '%0'", PointAt<1>);
CREATE_ERROR(CHARACTER_TOO_LARGE_FOR_LITERAL_TYPE, "Character too large for literal type", Underline<0>);
CREATE_ERROR(INVALID_ESCAPE_SEQUENCE_N, "Invalid escape sequence '%0'", PointAt<1>);
CREATE_ERROR(EXPECTED_CHARACTER_AFTER_BACKSLASH, "Expected character after backslash", PointAt<0>);
CREATE_ERROR(NEWLINE_IN_CHARACTER_LITERAL_USE_BACKLASH_N, "Newline in character literal. Use \\n instead", PointAt<0>);
CREATE_ERROR(NEWLINE_IN_STRING_LITERAL_USE_BACKLASH_N, "Newline in string literal. Use \\n instead", PointAt<0>);
CREATE_ERROR(INVALID_UTF8_SEQUENCE, "Invalid UTF-8 Sequence", PointAt<0>);
CREATE_ERROR(INVALID_UC_VALUE_MUSTNT_BE_LESS_THAN_A0, "Invalid universal character. Value mustn't be less than 0x00A0",
             Underline<0>);
CREATE_ERROR(INVALID_UC_VALUE_MUST_FIT_IN_UTF32, "Invalid universal character. Value must fit in UTF32", Underline<0>);
CREATE_ERROR(INVALID_UC_VALUE_MUSTNT_BE_IN_RANGE,
             "Invalid universal character. Value mustn't be in range of 0xD800 to 0xDFFF", Underline<0>);
CREATE_ERROR(INVALID_UC_EXPECTED_N_MORE_DIGITS, "Invalid universal character. Expected %0 more hex digit%s0",
             Underline<1>);
CREATE_ERROR(AT_LEAST_ONE_HEXADECIMAL_DIGIT_REQUIRED, "At least one hexadecimal digit required", PointAt<0>);
CREATE_ERROR(UNEXPECTED_CHARACTER, "Unexpected character", PointAt<0>);
CREATE_ERROR(NON_PRINTABLE_CHARACTER_N, "Non printable character '%0'", PointAt<1>);
CREATE_ERROR(STRAY_N_IN_PROGRAM, "Stray %tokenType0 in program", PointAt<0>);
CREATE_ERROR(UNTERMINATED_STRING_LITERAL, "Unterminated string literal", Underline<0>);
CREATE_ERROR(UNTERMINATED_CHARACTER_LITERAL, "Unterminated character literal", Underline<0>);
} // namespace Lexer

} // namespace coral66::Errors

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
