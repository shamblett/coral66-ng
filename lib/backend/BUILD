#
# Frontend compiler library
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "backend",
    srcs = [
        "abi_implementation.cpp",
        "code_generation.cpp",
        "code_generator.cpp",
        "code_generator_declarations.cpp",
        "code_generator_expressions.cpp",
        "code_generator_statements.cpp",
        "x64_abi.cpp",
    ],
    hdrs = [
        "abi_implementation.hpp",
        "code_generation.hpp",
        "code_generator.hpp",
        "common_abi_implementation.hpp",
        "type_identity.hpp",
        "x64_abi.hpp",
    ],
    visibility = [
        "//lib/main:__pkg__",
        "//test:__subpackages__",
    ],
    deps = [
        "//lib/frontend/common",
        "//lib/frontend/compiler",
        "//lib/support",
    ],
)
