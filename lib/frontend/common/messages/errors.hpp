/*
 * Project: coral66-ng
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

namespace coral66::errors
{
namespace parser
{
constexpr auto EXPECTED_N = makeDiagnostic<"Expected %0", InsertAfter<1, 0>>(Severity::Error, "EXPECTED_N");
constexpr auto EXPECTED_CONDITION =
    makeDiagnostic<"Expected Condition %0", InsertAfter<1, 0>>(Severity::Error, "EXPECTED_CONDITION");
constexpr auto EXPECTED_BEGIN =
    makeDiagnostic<"Expected 'BEGIN' keyword", InsertAfter<0>>(Severity::Error, "EXPECTED_BEGIN");
constexpr auto EXPECTED_PROGRAM_SEGMENT = makeDiagnostic<"Expected 'PROGRAM' or 'SEGMENT' keyword", InsertAfter<0>>(
    Severity::Error, "EXPECTED_PROGRAM_SEGMENT");
constexpr auto EXPECTED_END = makeDiagnostic<"Expected 'END' keyword", InsertAfter<0>>(Severity::Error, "EXPECTED_END");
constexpr auto MISSING_END_FOR_PROCEDURE =
    makeDiagnostic<"Procedure has a begin statement but no end is found before the next procedure is declared",
                   InsertAfter<0>>(Severity::Error, "MISSING_END_FOR_PROCEDURE");
constexpr auto MISSING_BEGIN_FOR_PROCEDURE =
    makeDiagnostic<"Procedure has an end statement without a preceeding begin statement", InsertAfter<0>>(
        Severity::Error, "MISSING_BEGIN_FOR_PROCEDURE");
constexpr auto EXPECTED_SEMICOLON =
    makeDiagnostic<"Expected semicolon", InsertAfter<0>>(Severity::Error, "EXPECTED_SEMICOLON");
constexpr auto EXPECTED_IDENTIFIER =
    makeDiagnostic<"Expected identifier", InsertAfter<0>>(Severity::Error, "EXPECTED_IDENTIFIER");
constexpr auto EXPECTED_LABELS =
    makeDiagnostic<"Expected at least one label", InsertAfter<0>>(Severity::Error, "EXPECTED_LABELS");
constexpr auto EXPECTED_EXPRESSION =
    makeDiagnostic<"Expected expression", InsertAfter<0>>(Severity::Error, "EXPECTED_EXPRESSION");
constexpr auto EXPECTED_SIMPLE_EXPRESSION =
    makeDiagnostic<"Expected simple expression", InsertAfter<0>>(Severity::Error, "EXPECTED_SIMPLE_EXPRESSION");
constexpr auto EXPECTED_TERM = makeDiagnostic<"Expected term", InsertAfter<0>>(Severity::Error, "EXPECTED_TERM");
constexpr auto EXPECTED_FACTOR = makeDiagnostic<"Expected factor", InsertAfter<0>>(Severity::Error, "EXPECTED_FACTOR");
constexpr auto EXPECTED_CLOSE_PARENTHESIS =
    makeDiagnostic<"Expected close parenthesis", InsertAfter<0>>(Severity::Error, "EXPECTED_CLOSE_PARENTHESIS");
constexpr auto EXPECTED_OPEN_PARENTHESIS =
    makeDiagnostic<"Expected open parenthesis", InsertAfter<0>>(Severity::Error, "EXPECTED_OPEN_PARENTHESIS");
constexpr auto EXPECTED_OPEN_SQUARE_BRACKET =
    makeDiagnostic<"Expected open square bracket", InsertAfter<0>>(Severity::Error, "EXPECTED_OPEN_SQUARE_BRACKET");
constexpr auto EXPECTED_CLOSE_SQUARE_BRACKET =
    makeDiagnostic<"Expected close square bracket", InsertAfter<0>>(Severity::Error, "EXPECTED_CLOSE_SQUARE_BRACKET");
constexpr auto EXPECTED_TYPED_PRIMARY =
    makeDiagnostic<"Expected typed primary", InsertAfter<0>>(Severity::Error, "EXPECTED_TYPED_PRIMARY");
constexpr auto EXPECTED_WORD_REFERENCE =
    makeDiagnostic<"Expected word reference", InsertAfter<0>>(Severity::Error, "EXPECTED_WORD_REFERENCE");
constexpr auto EXPECTED_VARIABLE =
    makeDiagnostic<"Expected variable", InsertAfter<0>>(Severity::Error, "EXPECTED_VARIABLE");
constexpr auto EXPECTED_PROCEDURE_CALL =
    makeDiagnostic<"Expected procedure call", InsertAfter<0>>(Severity::Error, "EXPECTED_PROCEDURE_CALL");
constexpr auto EXPECTED_FRACTION_BITS =
    makeDiagnostic<"Expected fraction bits", InsertAfter<0>>(Severity::Error, "EXPECTED_FRACTION_BITS");
constexpr auto EXPECTED_UNION = makeDiagnostic<"Expected union", InsertAfter<0>>(Severity::Error, "EXPECTED_UNION");
constexpr auto EXPECTED_MASK = makeDiagnostic<"Expected mask", InsertAfter<0>>(Severity::Error, "EXPECTED_MASK");
constexpr auto EXPECTED_STATEMENT =
    makeDiagnostic<"Expected statement", InsertAfter<0>>(Severity::Error, "EXPECTED_STATEMENT");
constexpr auto EXPECTED_SIMPLE_STATEMENT =
    makeDiagnostic<"Expected simple statement", InsertAfter<0>>(Severity::Error, "EXPECTED_SIMPLE_STATEMENT");
constexpr auto EXPECTED_STRING_LITERAL =
    makeDiagnostic<"Expected string literal", InsertAfter<0>>(Severity::Error, "EXPECTED_STRING_LITERAL");
constexpr auto EXPECTED_COLON = makeDiagnostic<"Expected colon", InsertAfter<0>>(Severity::Error, "EXPECTED_COLON");
constexpr auto EXPECTED_TYPE = makeDiagnostic<"Expected type", InsertAfter<0>>(Severity::Error, "EXPECTED_TYPE");
constexpr auto EXPECTED_DECLARATION =
    makeDiagnostic<"Expected declaration", InsertAfter<0>>(Severity::Error, "EXPECTED_DECLARATION");
constexpr auto EXPECTED_COMPARATOR =
    makeDiagnostic<"Expected comparator", InsertAfter<0>>(Severity::Error, "EXPECTED_COMPARATOR");
constexpr auto EXPECTED_ASSIGNMENT =
    makeDiagnostic<"Expected assignment ':='", InsertAfter<0>>(Severity::Error, "EXPECTED_ASSIGNMENT");
constexpr auto EXPECTED_THEN =
    makeDiagnostic<"Expected 'THEN' in IF statement", InsertAfter<0>>(Severity::Error, "EXPECTED_THEN");
constexpr auto EXPECTED_ELSE =
    makeDiagnostic<"Expected 'ELSE' in IF statement", InsertAfter<0>>(Severity::Error, "EXPECTED_ELSE");
constexpr auto EXPECTED_DO =
    makeDiagnostic<"Expected 'DO' in FOR statement", InsertAfter<0>>(Severity::Error, "EXPECTED_DO");
constexpr auto EXPECTED_UNTIL =
    makeDiagnostic<"Expected 'UNTIL' in FOR statement", InsertAfter<0>>(Severity::Error, "EXPECTED_UNTIL");
constexpr auto EXPECTED_STEP =
    makeDiagnostic<"Expected 'STEP' in FOR statement", InsertAfter<0>>(Severity::Error, "EXPECTED_STEP");
constexpr auto INVALID_FIXED_FORMAT =
    makeDiagnostic<"Invalid format for FIXED type. Expected (total, fraction)", Underline<0>>(Severity::Error,
                                                                                              "INVALID_FIXED_FORMAT");
constexpr auto INVALID_BITS_FORMAT =
    makeDiagnostic<"Invalid format for BITS type", Underline<0>>(Severity::Error, "INVALID_BITS_FORMAT");
constexpr auto EXPECTED_N_INSTEAD_OF_N =
    makeDiagnostic<"Expected %0 instead of %1", PointAt<1>>(Severity::Error, "EXPECTED_N_INSTEAD_OF_N");
constexpr auto ANSWER_OUTSIDE_PROCEDURE =
    makeDiagnostic<"Answer statement used outside procedure body", InsertAfter<0>>(Severity::Error,
                                                                                   "ANSWER_OUTSIDE_PROCEDURE");
constexpr auto ANSWER_IN_UNTYPED_PROCEDURE =
    makeDiagnostic<"Answer statement used inside untyped procedure", InsertAfter<0>>(Severity::Error,
                                                                                     "ANSWER_IN_UNTYPED_PROCEDURE");
constexpr auto MISSING_ANSWER_STATEMENT =
    makeDiagnostic<"Typed procedure must contain an answer statement", InsertAfter<0>>(Severity::Error,
                                                                                       "MISSING_ANSWER_STATEMENT");
constexpr auto MALFORMED_COMMUNICATOR =
    makeDiagnostic<"Invalid format for communicator", Underline<0>>(Severity::Error, "MALFORMED_COMMUNICATOR");
} // namespace parser

namespace semantics
{
constexpr auto UNDECLARED_IDENTIFIER_N =
    makeDiagnostic<"Undeclared identifier %0", Underline<0>>(Severity::Error, "UNDECLARED_IDENTIFIER_N");
constexpr auto REDEFINITION_OF_SYMBOL_N =
    makeDiagnostic<"Redefinition of symbol %0", Underline<0>>(Severity::Error, "REDEFINITION_OF_SYMBOL_N");
constexpr auto TYPE_MISMATCH =
    makeDiagnostic<"Type mismatch in expression", Underline<0>>(Severity::Error, "TYPE_MISMATCH");
constexpr auto EXPECTED_INTEGER_TYPE =
    makeDiagnostic<"Expected INTEGER type", Underline<0>>(Severity::Error, "EXPECTED_INTEGER_TYPE");
constexpr auto EXPECTED_NUMERIC_TYPE =
    makeDiagnostic<"Expected numeric type", Underline<0>>(Severity::Error, "EXPECTED_NUMERIC_TYPE");
constexpr auto EXPECTED_CONSTANT_EXPRESSION =
    makeDiagnostic<"Expected constant expression", Underline<0>>(Severity::Error, "EXPECTED_CONSTANT_EXPRESSION");
constexpr auto INCORRECT_NUMBER_OF_INDICES =
    makeDiagnostic<"Incorrect number of indices for array access", Underline<0>>(Severity::Error,
                                                                                 "INCORRECT_NUMBER_OF_INDICES");
constexpr auto ARRAY_INDEX_OUT_OF_BOUNDS =
    makeDiagnostic<"Array index out of bounds", Underline<0>>(Severity::Error, "ARRAY_INDEX_OUT_OF_BOUNDS");
constexpr auto INVALID_ARRAY_BOUNDS =
    makeDiagnostic<"Invalid array bounds: lower bound must be <= upper bound", Underline<0>>(Severity::Error,
                                                                                             "INVALID_ARRAY_BOUNDS");
constexpr auto SWITCH_INDEX_OUT_OF_BOUNDS =
    makeDiagnostic<"Switch index out of bounds", Underline<0>>(Severity::Error, "SWITCH_INDEX_OUT_OF_BOUNDS");
constexpr auto INCORRECT_PRESET_COUNT = makeDiagnostic<"Incorrect number of presets for declaration", Underline<0>>(
    Severity::Error, "INCORRECT_PRESET_COUNT");
constexpr auto TABLE_INVALID_DIMENSIONS =
    makeDiagnostic<"Table width and length must be strictly positive", Underline<0>>(Severity::Error,
                                                                                     "TABLE_INVALID_DIMENSIONS");
constexpr auto CANNOT_ASSIGN_TO_CONST = makeDiagnostic<"Cannot assign to constant or value parameter", Underline<0>>(
    Severity::Error, "CANNOT_ASSIGN_TO_CONST");
constexpr auto CANNOT_ASSIGN_TO_SWITCH =
    makeDiagnostic<"Cannot assign to a switch", Underline<0>>(Severity::Error, "CANNOT_ASSIGN_TO_SWITCH");
constexpr auto INVALID_BIT_RANGE =
    makeDiagnostic<"Invalid bit range specified", Underline<0>>(Severity::Error, "INVALID_BIT_RANGE");
constexpr auto LABEL_NOT_FOUND_N =
    makeDiagnostic<"Label %0 not found", Underline<0>>(Severity::Error, "LABEL_NOT_FOUND_N");
constexpr auto FAILED_TO_READ_CMI_FILE_N =
    makeDiagnostic<"Failed to read or parse CMI file: %1", Underline<0>>(Severity::Error, "FAILED_TO_READ_CMI_FILE_N");
constexpr auto PROCEDURE_ARGUMENT_COUNT_MISMATCH =
    makeDiagnostic<"Incorrect number of arguments for procedure call", Underline<0>>(
        Severity::Error, "PROCEDURE_ARGUMENT_COUNT_MISMATCH");
constexpr auto PROCEDURE_ARGUMENT_TYPE_MISMATCH =
    makeDiagnostic<"Argument type mismatch", Underline<0>>(Severity::Error, "PROCEDURE_ARGUMENT_TYPE_MISMATCH");
constexpr auto LOCATION_PARAMETER_REQUIRES_LVALUE =
    makeDiagnostic<"LOCATION parameter requires a variable (L-value)", Underline<0>>(
        Severity::Error, "LOCATION_PARAMETER_REQUIRES_LVALUE");
constexpr auto INVALID_SCALE_FOR_FIXED =
    makeDiagnostic<"Invalid scale for FIXED point operation", Underline<0>>(Severity::Error, "INVALID_SCALE_FOR_FIXED");
constexpr auto RECURSIVE_CALL_ON_NON_RECURSIVE_PROCEDURE =
    makeDiagnostic<"Procedure calls itself but is not declared RECURSIVE", Underline<0>>(
        Severity::Error, "RECURSIVE_CALL_ON_NON_RECURSIVE_PROCEDURE");
constexpr auto PRESET_IN_RECURSIVE_PROCEDURE =
    makeDiagnostic<"Presetting is not allowed in the body of a recursive procedure", Underline<0>>(
        Severity::Error, "PRESET_IN_RECURSIVE_PROCEDURE");
constexpr auto PRESET_IN_INNER_BLOCK = makeDiagnostic<"Presetting is not allowed in an inner block", Underline<0>>(
    Severity::Error, "PRESET_IN_INNER_BLOCK");
constexpr auto PRESET_TYPE_MISMATCH = makeDiagnostic<"Preset value type does not match declaration type", Underline<0>>(
    Severity::Error, "PRESET_TYPE_MISMATCH");
constexpr auto PRESET_FIXED_POINT_OVERFLOW =
    makeDiagnostic<"Preset value exceeds fixed point scale bounds", Underline<0>>(Severity::Error,
                                                                                  "PRESET_FIXED_POINT_OVERFLOW");
} // namespace semantics

namespace CLI
{
constexpr auto FAILED_TO_OPEN_SOURCE_FILE =
    makeDiagnostic<"Failed to open source file '%0'">(Severity::Error, "FAILED_TO_OPEN_SOURCE_FILE");
constexpr auto NO_SOURCE_FILES_SPECIFIED =
    makeDiagnostic<"No source files specified">(Severity::Error, "NO_SOURCE_FILES_SPECIFIED");
constexpr auto SOURCE_FILE_EXTENSION_NOT_COR =
    makeDiagnostic<"The source file extension is not .cor '%0'">(Severity::Error, "SOURCE_FILE_EXTENSION_NOT_COR");
constexpr auto FAILED_TO_OPEN_FILE_FOR_OUTPUT =
    makeDiagnostic<"Failed to open file '%0' for output">(Severity::Error, "FAILED_TO_OPEN_FILE_FOR_OUTPUT");
constexpr auto FAILED_TO_GENERATE_TARGET_MACHINE =
    makeDiagnostic<"Failed to generate LLVM target machine">(Severity::Error, "FAILED_TO_GENERATE_TARGET_MACHINE");
constexpr auto FAILED_TO_LINK_EXECUTABLE =
    makeDiagnostic<"Failed to link executable">(Severity::Error, "FAILED_TO_LINK_EXECUTABLE");
} // namespace CLI

namespace lexer
{
// Adding some Lexer errors that might be used by Lexer.cpp
constexpr auto INVALID_UTF8_SEQUENCE =
    makeDiagnostic<"Invalid UTF-8 Sequence", PointAt<0>>(Severity::Error, "INVALID_UTF8_SEQUENCE");
constexpr auto UNEXPECTED_CHARACTER =
    makeDiagnostic<"Unexpected character", PointAt<0>>(Severity::Error, "UNEXPECTED_CHARACTER");
constexpr auto UNTERMINATED_STRING_LITERAL =
    makeDiagnostic<"Unterminated string literal", Underline<0>>(Severity::Error, "UNTERMINATED_STRING_LITERAL");
constexpr auto UNTERMINATED_BRACKETED_COMMENT =
    makeDiagnostic<"Unterminated bracketed comment", Underline<0>>(Severity::Error, "UNTERMINATED_BRACKETED_COMMENT");
constexpr auto UNTERMINATED_STROPPED_KEYWORD =
    makeDiagnostic<"Unterminated stropped keyword", Underline<0>>(Severity::Error, "UNTERMINATED_STROPPED_KEYWORD");
constexpr auto INVALID_STROPPED_KEYWORD_N =
    makeDiagnostic<"Invalid stropped keyword '%0'", PointAt<1>>(Severity::Error, "INVALID_STROPPED_KEYWORD_N");
constexpr auto UNTERMINATED_COMMENT_KEYWORD_SEQUENCE =
    makeDiagnostic<"Unterminated COMMENT keyword sequence", Underline<0>>(Severity::Error,
                                                                          "UNTERMINATED_COMMENT_KEYWORD_SEQUENCE");
constexpr auto MALFORMED_END_KEYWORD_SEQUENCE =
    makeDiagnostic<"Malformed END keyword sequence", Underline<0>>(Severity::Error, "MALFORMED_END_KEYWORD_SEQUENCE");
constexpr auto MALFORMED_NUMBER_LITERAL =
    makeDiagnostic<"Malformed number literal", Underline<0>>(Severity::Error, "MALFORMED_NUMBER_LITERAL");
} // namespace lexer

namespace pp
{

constexpr auto REDEFINITION_OF_MACRO_PARAMETER_N = makeDiagnostic<"Redefinition of macro parameter %0", Underline<0>>(
    Severity::Error, "REDEFINITION_OF_MACRO_PARAMETER_N");

constexpr auto WHITESPACE_REQUIRED_AFTER_OBJECT_MACRO_DEFINITION =
    makeDiagnostic<"Whitespace required after object macro definition", Underline<0>>(
        Severity::Error, "WHITESPACE_REQUIRED_AFTER_OBJECT_MACRO_DEFINITION");

constexpr auto INFINITE_MACRO_RECURSION_DETECTED_N =
    makeDiagnostic<"Infinite macro recursion detected for macro %0", Underline<0>>(
        Severity::Error, "INFINITE_MACRO_RECURSION_DETECTED_N");

constexpr auto NOT_ENOUGH_ARGUMENTS_FOR_MACRO_N_EXPECTED_N_GOT_N =
    makeDiagnostic<"Not enough arguments for macro %0. Expected %1 got %2", Underline<0>>(
        Severity::Error, "NOT_ENOUGH_ARGUMENTS_FOR_MACRO_N_EXPECTED_N_GOT_N");

constexpr auto TOO_MANY_ARGUMENTS_FOR_MACRO_N_EXPECTED_N_GOT_N =
    makeDiagnostic<"Too many arguments for macro %0. Expected %1 got %2", Underline<0>, PointAt<3>>(
        Severity::Error, "TOO_MANY_ARGUMENTS_FOR_MACRO_N_EXPECTED_N_GOT_N");

constexpr auto PP_EXPECTED_N_INSTEAD_OF_N =
    makeDiagnostic<"Expected %0 instead of %1", PointAt<1>>(Severity::Error, "PP_EXPECTED_N_INSTEAD_OF_N");
constexpr auto PP_EXPECTED_N_AFTER_N =
    makeDiagnostic<"Expected %0 after %1", InsertAfter<1, 0>>(Severity::Error, "PP_EXPECTED_N_AFTER_N");
constexpr auto PP_EXPECTED_N = makeDiagnostic<"Expected %0", InsertAfter<1, 0>>(Severity::Error, "PP_EXPECTED_N");
} // namespace pp

} // namespace coral66::errors
