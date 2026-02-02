#
# Frontend common library
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "common",
    srcs = [
        "ConstantValue.cpp",
        "Diagnostic.cpp",
        "LanguageOptions.cpp",
        "Message.cpp",
        "SourceObject.cpp",
        "targets/X86TargetFeatures.cpp",
    ],
    hdrs = [
        "Attributes.hpp",
        "Builtins.def",
        "ConstantValue.hpp",
        "CustomDiagnostics.hpp",
        "Diagnostic.hpp",
        "DiagnosticUtilities.hpp",
        "ErrorMessages.hpp",
        "LanguageOptions.hpp",
        "LexerCommon.hpp",
        "Limits.hpp",
        "Message.hpp",
        "SemanticsCommon.hpp",
        "SourceInterface.hpp",
        "SourceObject.hpp",
        "targets/X86Features.def",
        "targets/X86TargetFeatures.hpp",
    ],
    visibility = [
        "//lib/backend:__subpackages__",
        "//lib/frontend:__subpackages__",
        "//lib/main:__pkg__",
        "//test:__subpackages__",
    ],
    deps = [
        "//lib/support",
    ],
)
