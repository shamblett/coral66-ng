#
# Frontend semantics library
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "semantics",
    srcs = [
        "control_flow_pass.cpp",
        "program.cpp",
        "program_interface.cpp",
        "semantic_analysis.cpp",
        "semantic_context.cpp",
        "semantics.cpp",
        "symbol_resolution_pass.cpp",
        "symbol_table.cpp",
        "type_checking_pass.cpp",
    ],
    hdrs = [
        "control_flow_pass.hpp",
        "program.hpp",
        "program_interface.hpp",
        "semantic_analysis.hpp",
        "semantic_context.hpp",
        "semantic_support.hpp",
        "semantic_utilities.hpp",
        "semantics.hpp",
        "symbol_resolution_pass.hpp",
        "symbol_table.hpp",
        "type_checking_pass.hpp",
    ],
    visibility = [
        "//lib/backend:__subpackages__",
        "//lib/frontend:__subpackages__",
        "//lib/main:__pkg__",
        "//test:__subpackages__",
    ],
    deps = [
        "//lib/frontend/common",
        "//lib/frontend/lexer",
        "//lib/frontend/syntax",
        "//lib/support",
    ],
)
