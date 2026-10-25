/*
 * Project: coral66-ng
 * Author : Copyright S. Hamblett <steve.hamblett@linux.com>
 * Date   : 31/01/2026
 */

#pragma once

/**
 * Warning message definitions.
 *
 */

#include "lib/frontend/common/diagnostic.hpp"
#include "lib/support/constexpr.hpp"

namespace coral66::warnings::CLI
{
namespace detail
{
inline auto CANNOT_COMPILE_TO_OBJECT_FILE_AND_ASSEMBLY_AT_THE_SAME_TIMERegister =
    coral66::detail::Diagnostic::WarningRegistrar("command-line-argument-combination");
}
constexpr auto CANNOT_COMPILE_TO_OBJECT_FILE_AND_ASSEMBLY_AT_THE_SAME_TIME =
    coral66::makeDiagnostic<"Cannot compile to object file and assembly at the same time - ignoring option">(
        Severity::Warning, "command-line-argument-combination");

namespace detail
{
inline auto CANNOT_COMPILE_TO_OBJECT_FILE_AND_PREPROCESS_AT_THE_SAME_TIMERegister =
    coral66::detail::Diagnostic::WarningRegistrar("command-line-argument-combination");
}
constexpr auto CANNOT_COMPILE_TO_OBJECT_FILE_AND_PREPROCESS_AT_THE_SAME_TIME =
    coral66::makeDiagnostic<"Cannot compile to object file and preprocess at the same time - ignoring option">(
        Severity::Warning, "command-line-argument-combination");

namespace detail
{
inline auto CANNOT_COMPILE_TO_ASSEMBLY_AND_PREPROCESS_AT_THE_SAME_TIMERegister =
    coral66::detail::Diagnostic::WarningRegistrar("command-line-argument-combination");
}
constexpr auto CANNOT_COMPILE_TO_ASSEMBLY_AND_PREPROCESS_AT_THE_SAME_TIME =
    coral66::makeDiagnostic<"Cannot compile to assembly and preprocess at the same time - ignoring option">(
        Severity::Warning, "command-line-argument-combination");

namespace detail
{
inline auto NO_ACTION_SPEFIFIEDRegister = coral66::detail::Diagnostic::WarningRegistrar("command-line-noaction");
}
constexpr auto NO_ACTION_SPEFIFIED =
    coral66::makeDiagnostic<"No action specified - command line checked">(Severity::Warning, "command-line-noaction");
} // namespace coral66::warnings::CLI
