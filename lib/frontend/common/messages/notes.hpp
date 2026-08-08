/*
 * Project: coral66
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

/**
 * Note definitions.
 *
 */

#include "lib/frontend/common/diagnostic.hpp"
#include "lib/support/constexpr.hpp"

#define CREATE_NOTE_2(variableName, format)                                                               \
    namespace detail                                                                                      \
    {                                                                                                     \
    constexpr auto CORAL66_PASTE2(variableName, Text) = ::coral66::Constexpr::basic_fixed_string{format}; \
    }                                                                                                     \
    constexpr auto variableName =                                                                         \
        ::coral66::makeDiagnostic<detail::CORAL66_PASTE2(variableName, Text)>(::coral66::Severity::Note, "")

#define CREATE_NOTE_3(...) CREATE_NOTE_VAARG(__VA_ARGS__)
#define CREATE_NOTE_4(...) CREATE_NOTE_VAARG(__VA_ARGS__)
#define CREATE_NOTE_5(...) CREATE_NOTE_VAARG(__VA_ARGS__)
#define CREATE_NOTE_6(...) CREATE_NOTE_VAARG(__VA_ARGS__)
#define CREATE_NOTE_7(...) CREATE_NOTE_VAARG(__VA_ARGS__)
#define CREATE_NOTE_8(...) CREATE_NOTE_VAARG(__VA_ARGS__)

#define CREATE_NOTE(...) CORAL66_PASTE2(CREATE_NOTE_, CORAL66_MACRO_COUNT_ARGUMENTS(__VA_ARGS__))(__VA_ARGS__)

#define CREATE_NOTE_VAARG(variableName, format, ...)                                                                  \
    namespace detail                                                                                                  \
    {                                                                                                                 \
    constexpr auto CORAL66_PASTE2(variableName, Text) = ::coral66::Constexpr::basic_fixed_string{format};             \
    }                                                                                                                 \
    constexpr auto variableName = ::coral66::makeDiagnostic<detail::CORAL66_PASTE2(variableName, Text), __VA_ARGS__>( \
        ::coral66::Severity::Note, "")

namespace coral66::notes
{

namespace pp
{
CREATE_NOTE(TO_MATCH_N_HERE, "To match %0 here:", PointAt<0>);
CREATE_NOTE(PREVIOUSLY_DECLARED_HERE, "Previously declared here:", Underline<0>);
} // namespace pp
} // namespace coral66::notes

#undef CREATE_NOTE
#undef CREATE_NOTE_2
#undef CREATE_NOTE_3
#undef CREATE_NOTE_4
#undef CREATE_NOTE_5
#undef CREATE_NOTE_6
#undef CREATE_NOTE_7
#undef CREATE_NOTE_8
#undef CREATE_NOTE_VAARG
