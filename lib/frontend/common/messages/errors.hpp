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

#define CREATE_ERROR(eName, ...)P99_PASTE2(CREATE_ERROR_, CORAL66_MACRO_COUNT_ARGUMENTS(__VA_ARGS__))(__VA_ARGS__)

#define CREATE_ERROR_VAARG(variableName, format, uniquename, ...)                                                 \
    namespace detail                                                                                  \
    {                                                                                                 \
    constexpr auto P99_PASTE2(variableName, Text) = ::coral66::Constexpr::basic_fixed_string{format}; \
    }                                                                                                 \
    constexpr auto variableName =                                                                     \
        ::coral66::makeDiagnostic<detail::P99_PASTE2(variableName, Text), __VA_ARGS__>(::coral66::Severity::Error, uniquename)

namespace coral66::errors
{
namespace Parser
{
CREATE_ERROR("gg", EXPECTED_N, "Expected %0", "Parser-01", InsertAfter<1, 0>);
CREATE_ERROR(EXPECTED_CONDITION, "Expected Condition %0", "Parser-02", InsertAfter<1, 0>);
CREATE_ERROR(EXPECTED_BEGIN, "Expected 'BEGIN' keyword", "Parser-03", InsertAfter<0>);
CREATE_ERROR(EXPECTED_END, "Expected 'END' keyword", "Parser-04", InsertAfter<0>);
CREATE_ERROR(EXPECTED_IDENTIFIER, "Expected identifier", "Parser-05", InsertAfter<0>);
CREATE_ERROR(EXPECTED_EXPRESSION, "Expected expression", "Parser-06", InsertAfter<0>);
CREATE_ERROR(EXPECTED_SIMPLE_EXPRESSION, "Expected simple expression", "Parser-07", InsertAfter<0>);
CREATE_ERROR(EXPECTED_TERM, "Expected term", "Parser-08", InsertAfter<0>);
CREATE_ERROR(EXPECTED_FACTOR, "Expected factor", "Parser-09", InsertAfter<0>);
CREATE_ERROR(EXPECTED_CLOSED_PARENTHESIS, "Expected closed parenthesis", "Parser-10", InsertAfter<0>);
CREATE_ERROR(EXPECTED_OPEN_PARENTHESIS, "Expected open parenthesis", "Parser-11", InsertAfter<0>);
CREATE_ERROR(EXPECTED_OPEN_SQUARE_BRACKET, "Expected open square backet", "Parser-12", InsertAfter<0>);
CREATE_ERROR(EXPECTED_TYPED_PRIMARY, "Expected typed primary", "Parser-13", InsertAfter<0>);
CREATE_ERROR(EXPECTED_WORD_REFERENCE, "Expected word reference", "Parser-14", InsertAfter<0>);
CREATE_ERROR(EXPECTED_FRACTION_BITS, "Expected fraction bits", "Parser-15", InsertAfter<0>);
CREATE_ERROR(EXPECTED_UNION, "Expected union", "Parser-16", InsertAfter<0>);
CREATE_ERROR(EXPECTED_MASK, "Expected mask", "Parser-17", InsertAfter<0>);
CREATE_ERROR(EXPECTED_STATEMENT, "Expected statement", "Parser-18", InsertAfter<0>);
CREATE_ERROR(EXPECTED_DECLARATION, "Expected declaration", "Parser-19", InsertAfter<0>);
CREATE_ERROR(EXPECTED_COMPARATOR, "Expected comparator", "Parser-20", InsertAfter<0>);
CREATE_ERROR(EXPECTED_ASSIGNMENT, "Expected assignment ':='", "Parser-21", InsertAfter<0>);
CREATE_ERROR(EXPECTED_THEN, "Expected 'THEN' in IF statement", "Parser-22", InsertAfter<0>);
CREATE_ERROR(EXPECTED_ELSE, "Expected 'ELSE' in IF statement", "Parser-23", InsertAfter<0>);
CREATE_ERROR(EXPECTED_DO, "Expected 'DO' in FOR statement", "Parser-024", InsertAfter<0>);
CREATE_ERROR(EXPECTED_UNTIL, "Expected 'UNTIL' in FOR statement", "Parser-25", InsertAfter<0>);
CREATE_ERROR(EXPECTED_STEP, "Expected 'STEP' in FOR statement", "Parser-26", InsertAfter<0>);
CREATE_ERROR(INVALID_FIXED_FORMAT, "Invalid format for FIXED type. Expected (total, fraction)", "Parser-27", Underline<0>);
CREATE_ERROR(INVALID_BITS_FORMAT, "Invalid format for BITS type", "Parser-28", Underline<0>);
CREATE_ERROR(EXPECTED_N_INSTEAD_OF_N, "Expected %0 instead of %1", "Parser-29", PointAt<1>);
CREATE_ERROR(EXPECTED_N_AFTER_N, "Expected %0 after %1", "Parser-30", InsertAfter<1, 0>);
} // namespace Parser

namespace Semantics
{
CREATE_ERROR(UNDECLARED_IDENTIFIER_N, "Undeclared identifier %0", "Semantics-01", Underline<0>);
CREATE_ERROR(REDEFINITION_OF_SYMBOL_N, "Redefinition of symbol %0", "Semantics-02", Underline<0>);
CREATE_ERROR(TYPE_MISMATCH, "Type mismatch in expression", "Semantics-03", Underline<0>);
CREATE_ERROR(EXPECTED_INTEGER_TYPE, "Expected INTEGER type", "Semantics-04", AnnotateExpr<0>);
CREATE_ERROR(EXPECTED_FLOATING_TYPE, "Expected FLOATING type", "Semantics-05", AnnotateExpr<0>);
CREATE_ERROR(CANNOT_ASSIGN_TO_CONST, "Cannot assign to constant or value parameter", "Semantics-06", Underline<0>);
CREATE_ERROR(TABLE_ELEMENT_NOT_FOUND_N, "Element %0 not found in table", "Semantics-07", Underline<0>);
CREATE_ERROR(INVALID_BIT_RANGE, "Invalid bit range specified", "Semantics-08", Underline<0>);
CREATE_ERROR(LABEL_NOT_FOUND_N, "Label %0 not found", "Semantics-09", Underline<0>);
CREATE_ERROR(PROCEDURE_ARGUMENT_COUNT_MISMATCH, "Incorrect number of arguments for procedure call", "Semantics-10", Underline<0>);
CREATE_ERROR(PROCEDURE_ARGUMENT_TYPE_MISMATCH, "Argument type mismatch", "Semantics-11", AnnotateExpr<0>);
CREATE_ERROR(LOCATION_PARAMETER_REQUIRES_LVALUE, "LOCATION parameter requires a variable (L-value)", "Semantics-12", AnnotateExpr<0>);
CREATE_ERROR(INVALID_SCALE_FOR_FIXED, "Invalid scale for FIXED point operation", "Semantics-13", Underline<0>);
} // namespace Semantics

namespace CLI
{
namespace detail { constexpr auto FAILED_TO_OPEN_SOURCE_FILEText = ::coral66::Constexpr::basic_fixed_string{"Failed to open source file '%0'"}; } constexpr auto FAILED_TO_OPEN_SOURCE_FILE = ::coral66::makeDiagnostic<detail::FAILED_TO_OPEN_SOURCE_FILEText>(::coral66::Severity::Error, "");
CREATE_ERROR(NO_SOURCE_FILES_SPECIFIED, "No source files specified");
CREATE_ERROR(SOURCE_FILE_EXTENSION_NOT_COR, "The source file extension is not .cor '%0'");
CREATE_ERROR(FAILED_TO_OPEN_FILE_FOR_OUTPUT, "Failed to open file '%0' for output");
} // namespace CLI

namespace lexer
{
// Adding some Lexer errors that might be used by Lexer.cpp
CREATE_ERROR(INVALID_OCTAL_CHARACTER, "Invalid octal character '%0'", "Lexer-01", PointAt<1>);
CREATE_ERROR(CHARACTER_TOO_LARGE_FOR_LITERAL_TYPE, "Character too large for literal type", "Lexer-02", Underline<0>);
CREATE_ERROR(INVALID_ESCAPE_SEQUENCE_N, "Invalid escape sequence '%0'", "Lexer-03", PointAt<1>);
CREATE_ERROR(EXPECTED_CHARACTER_AFTER_BACKSLASH, "Expected character after backslash", "Lexer-04", PointAt<0>);
CREATE_ERROR(NEWLINE_IN_CHARACTER_LITERAL_USE_BACKLASH_N, "Newline in character literal. Use \\n instead", "Lexer-05", PointAt<0>);
CREATE_ERROR(NEWLINE_IN_STRING_LITERAL_USE_BACKLASH_N, "Newline in string literal. Use \\n instead", "Lexer-06", PointAt<0>);
CREATE_ERROR(INVALID_UTF8_SEQUENCE, "Invalid UTF-8 Sequence", "Lexer-07", PointAt<0>);
CREATE_ERROR(INVALID_UC_VALUE_MUSTNT_BE_LESS_THAN_A0, "Invalid universal character. Value mustn't be less than 0x00A0",
             "Lexer-08", Underline<0>);
CREATE_ERROR(INVALID_UC_VALUE_MUST_FIT_IN_UTF32, "Invalid universal character. Value must fit in UTF32", "Lexer-09", Underline<0>);
CREATE_ERROR(INVALID_UC_VALUE_MUSTNT_BE_IN_RANGE,
             "Invalid universal character. Value mustn't be in range of 0xD800 to 0xDFFF", "Lexer-10", Underline<0>);
CREATE_ERROR(INVALID_UC_EXPECTED_N_MORE_DIGITS, "Invalid universal character. Expected %0 more hex digit%s0",
             "Lexer-11", Underline<1>);
CREATE_ERROR(AT_LEAST_ONE_HEXADECIMAL_DIGIT_REQUIRED, "At least one hexadecimal digit required", "Lexer-12", PointAt<0>);
CREATE_ERROR(UNEXPECTED_CHARACTER, "Unexpected character", "Lexer-13", PointAt<0>);
CREATE_ERROR(NON_PRINTABLE_CHARACTER_N, "Non printable character '%0'", "Lexer-14", PointAt<1>);
CREATE_ERROR(STRAY_N_IN_PROGRAM, "Stray %tokenType0 in program", "Lexer-15", PointAt<0>);
CREATE_ERROR(UNTERMINATED_STRING_LITERAL, "Unterminated string literal", "Lexer-16", Underline<0>);
CREATE_ERROR(UNTERMINATED_CHARACTER_LITERAL, "Unterminated character literal", "Lexer-17", Underline<0>);
} // namespace lexer

namespace pp
{
CREATE_ERROR(N_IS_AN_INVALID_PREPROCESSOR_DIRECTIVE, "%0 is an invalid preprocessor directive", "PP-01", Underline<0>);

CREATE_ERROR(REDEFINITION_OF_MACRO_PARAMETER_N, "Redefinition of macro parameter %0", "PP-02", Underline<0>);

CREATE_ERROR(WHITESPACE_REQUIRED_AFTER_OBJECT_MACRO_DEFINITION, "Whitespace required after object macro definition",
             "PP-03", Underline<0>);

CREATE_ERROR(DEFINED_CANNOT_BE_USED_AS_MACRO_NAME, "'defined' cannot be used as macro name", "PP-04", Underline<0>);

CREATE_ERROR(REDEFINITION_OF_MACRO_N, "Redefinition of macro %0", "PP-05", Underline<0>);

CREATE_ERROR(VA_ARGS_NOT_ALLOWED_IN_REPLACEMENT_LIST, "__VA_ARGS__ not allowed in replacement list", "PP-06", Underline<0>);

CREATE_ERROR(DEFINING_BUILTIN_MACRO_N_IS_NOT_ALLOWED, "Defining builtin macro %0 is not allowed", "PP-07", Underline<0>);

CREATE_ERROR(UNDEFINING_BUILTIN_MACRO_N_IS_NOT_ALLOWED, "Undefining builtin macro %0 is not allowed", "PP-08", Underline<0>);

CREATE_ERROR(NOT_ENOUGH_ARGUMENTS_FOR_MACRO_N_EXPECTED_N_GOT_N, "Not enough arguments for macro %0. Expected %1 got %2",
             "PP-09", Underline<0>);

CREATE_ERROR(NOT_ENOUGH_ARGUMENTS_FOR_MACRO_N_EXPECTED_AT_LEAST_N_GOT_N,
             "Not enough arguments for macro %0. Expected at least %1 got %2", "PP-10", Underline<0>);

CREATE_ERROR(TOO_MANY_ARGUMENTS_FOR_MACRO_N_EXPECTED_N_GOT_N, "Too many arguments for macro %0. Expected %1 got %2",
             "PP-11", Underline<0>, PointAt<3>);

CREATE_ERROR(EXPECTED_AN_ARGUMENT_AFTER_POUND,
             "Expected an argument after '#' in replacement list of function like macro", "PP-12", PointAt<0>, Underline<1>);

CREATE_ERROR(OPERATOR_DOUBLE_POUND_NOT_ALLOWED_AT_BEGINNING_OF_REPLACEMENT_LIST,
             "Operator '##' not allowed at beginning of replacement list", "PP-13", Underline<0>);

CREATE_ERROR(OPERATOR_DOUBLE_POUND_NOT_ALLOWED_AT_END_OF_REPLACEMENT_LIST,
             "Operator '##' not allowed at end of replacement list", "PP-14", Underline<0>);

CREATE_ERROR(EXPECTED_A_FILENAME_AFTER_INCLUDE, "Expected a <FILENAME> or \"FILENAME\" after #include", "PP-15", InsertAfter<0>);

CREATE_ERROR(EXPECTED_A_FILENAME_AFTER_INCLUDE_2, "Expected a <FILENAME> or \"FILENAME\" after #include", "PP-16", Underline<0>);

CREATE_ERROR(FILE_NOT_FOUND, "'%0' file not found", "PP-17", Underline<1>);

CREATE_ERROR(COULD_NOT_OPEN_FILE, "Could not open '%0'", "PP-18", Underline<1>);

CREATE_ERROR(EXPECTED_A_NUMBER_AFTER_LINE, "Expected a number after '#line'", "PP-19", InsertAfter<0>);

CREATE_ERROR(EXPECTED_END_OF_LINE_OR_STRING_AFTER_NUMBER_IN_LINE,
             "Expected end of line or string after number in '#line' directive", "PP-20", Underline<0>);
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
