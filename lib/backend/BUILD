#
# Backend library
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "backend",
    srcs = [
        "code_generation.cpp",
        "code_generator.cpp",
        "code_generator_declarations.cpp",
        "code_generator_expressions.cpp",
        "code_generator_statements.cpp",
        "numeric_support.cpp",
    ],
    hdrs = [
        "code_generation.hpp",
        "code_generator.hpp",
        "type_identity.hpp",
    ],
    visibility = [
        "//lib/main:__pkg__",
        "//test:__subpackages__",
    ],
    deps = [
        "//lib/frontend/common",
        "//lib/frontend/semantics",
        "//lib/support",
    ],
)
