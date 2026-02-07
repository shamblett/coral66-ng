#
# Frontend common library
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "common",
    srcs = [
        "constant_value.cpp",
        "diagnostic.cpp",
        "language_options.cpp",
        "message.cpp",
        "source_object.cpp",
        "targets/x86_target_features.cpp",
    ],
    hdrs = [
        "attributes.hpp",
        "builtins.def",
        "constant_value.hpp",
        "custom_diagnostics.hpp",
        "diagnostic.hpp",
        "diagnostic_utilities.hpp",
        "error_messages.hpp",
        "language_options.hpp",
        "lexer_common.hpp",
        "limits.hpp",
        "message.hpp",
        "semantics_common.hpp",
        "source_interface.hpp",
        "source_object.hpp",
        "targets/x86_features.def",
        "targets/x86_target_features.hpp",
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
