#
# Frontend compiler library
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "compiler",
    srcs = [
        "program.cpp",
        "program_interface.cpp",
        "semantic_analysis.cpp",
        "semantic_analysis_attributes.cpp",
        "semantic_analysis_builtins.cpp",
        "semantic_analysis_declarations.cpp",
        "semantic_analysis_expressions.cpp",
        "semantic_analysis_statements.cpp",
        "semantic_analysis_types.cpp",
        "semantics.cpp",
    ],
    hdrs = [
        "program.hpp",
        "program_interface.hpp",
        "semantic_analysis.hpp",
        "semantic_utilities.hpp",
        "semantics.hpp",
    ],
    visibility = [
        "//lib/backend:__subpackages__",
        "//lib/main:__pkg__",
        "//test:__subpackages__",
    ],
    deps = [
        "//lib/frontend/common",
        "//lib/frontend/lexer",
        "//lib/frontend/parser",
        "//lib/frontend/preprocessor",
        "//lib/frontend/syntax",
        "//lib/support",
    ],
)
