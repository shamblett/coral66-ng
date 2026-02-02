#
# Frontend common library
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "common",
    srcs = [
        "ConstValue.cpp",
        "Diagnostic.cpp",
        "LanguageOptions.cpp",
        "Message.cpp",
        "targets/x86TargetFeatures.cpp",
    ],
    hdrs = [
        "Attributes.hpp",
        "Builtins.def",
        "Constvalue.hpp",
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
        "target/x86TargetFeatures.hpp",
        "traget/X86features.def",
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
