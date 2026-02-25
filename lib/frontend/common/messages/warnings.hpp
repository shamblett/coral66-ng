/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

/**
 * Warning message definitions for Coral 66
 *
 */

#include "lib/frontend/common/diagnostic.hpp"
#include "lib/support/constexpr.hpp"

#define CREATE_WARNING_3(variableName, cliName, format)                                                        \
    namespace detail                                                                                           \
    {                                                                                                          \
    constexpr auto P99_PASTE2(variableName, Text) = ::coral66::Constexpr::basic_fixed_string{format};          \
    inline auto P99_PASTE2(variableName, Register) = ::coral66::detail::Diagnostic::WarningRegistrar(cliName); \
    }                                                                                                          \
    constexpr auto variableName =                                                                              \
        ::coral66::makeDiagnostic<detail::P99_PASTE2(variableName, Text)>(::coral66::Severity::Warning, cliName)

#define CREATE_WARNING_4(...) CREATE_WARNING_VAARG(__VA_ARGS__)
#define CREATE_WARNING_5(...) CREATE_WARNING_VAARG(__VA_ARGS__)
#define CREATE_WARNING_6(...) CREATE_WARNING_VAARG(__VA_ARGS__)
#define CREATE_WARNING_7(...) CREATE_WARNING_VAARG(__VA_ARGS__)
#define CREATE_WARNING_8(...) CREATE_WARNING_VAARG(__VA_ARGS__)
#define CREATE_WARNING_9(...) CREATE_WARNING_VAARG(__VA_ARGS__)

#define CREATE_WARNING(...) P99_PASTE2(CREATE_WARNING_, CORAL66_MACRO_COUNT_ARGUMENTS(__VA_ARGS__))(__VA_ARGS__)

#define CREATE_WARNING_VAARG(variableName, cliName, format, ...)                                                  \
    namespace detail                                                                                              \
    {                                                                                                             \
    constexpr auto P99_PASTE2(variableName, Text) = ::coral66::Constexpr::basic_fixed_string{format};             \
    inline auto P99_PASTE2(variableName, Register) = ::coral66::detail::Diagnostic::WarningRegistrar(cliName);    \
    }                                                                                                             \
    constexpr auto variableName = ::coral66::makeDiagnostic<detail::P99_PASTE2(variableName, Text), __VA_ARGS__>( \
        ::coral66::Severity::Warning, cliName)

namespace coral66::Warnings
{
namespace Semantics
{
CREATE_WARNING(UNUSED_VARIABLE_N, "unused-variable", "Unused variable %0", PointAt<0>);
CREATE_WARNING(POSSIBLE_PRECISION_LOSS, "precision-loss", "Possible loss of precision in fixed point operation",
               Underline<0>);
} // namespace Semantics
namespace CLI
{
CREATE_WARNING(CANNOT_COMPILE_TO_OBJECT_FILE_AND_ASSEMBLY_AT_THE_SAME_TIME, "command-line-argument-combination",
               "Cannot compile to object file and assembly at the same time - ignoring option");
CREATE_WARNING(CANNOT_COMPILE_TO_OBJECT_FILE_AND_PREPROCESS_AT_THE_SAME_TIME, "command-line-argument-combination",
               "Cannot compile to object file and preprocess at the same time - ignoring option");
CREATE_WARNING(CANNOT_COMPILE_TO_ASSEMBLY_AND_PREPROCESS_AT_THE_SAME_TIME, "command-line-argument-combination",
               "Cannot compile to assembly and preprocess at the same time - ignoring option");
CREATE_WARNING(NO_ACTION_SPEFIFIED, "command-line-noaction", "No action specified - command line checked");
} // namespace CLI

namespace pp
{
CREATE_WARNING(N_REDEFINED, "macro-redefined", "%0 redefined", Underline<0>);

CREATE_WARNING(TOKEN_CONCATENATION_RESULTING_IN_AN_INVALID_TOKEN_IS_UB, "token-concat",
               "Token concatenation resulting in an invalid token is undefined behaviour", Underline<0>, PointAt<1>,
               Underline<2>);

CREATE_WARNING(MACRO_EXPANSION_PRODUCING_DEFINED_IS_NOT_PORTABLE, "expansion-to-defined",
               "Macro expansion producing 'defined' is not portable", Underline<0>);
} // namespace pp
} // namespace coral66::Warnings

#undef CREATE_WARNING
#undef CREATE_WARNING_3
#undef CREATE_WARNING_4
#undef CREATE_WARNING_5
#undef CREATE_WARNING_6
#undef CREATE_WARNING_7
#undef CREATE_WARNING_8
#undef CREATE_WARNING_9
#undef CREATE_WARNING_VAARG