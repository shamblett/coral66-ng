#
# Frontend parser library
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "parser",
    srcs = [
        "parser.cpp",
        "parser_communicators.cpp",
        "parser_declarations.cpp",
        "parser_expressions.cpp",
        "parser_statements.cpp",
        "parser_utilities.cpp",
    ],
    hdrs = [
        "parser.hpp",
        "parser_utilities.hpp",
    ],
    visibility = [
        "//lib/backend:__subpackages__",
        "//lib/frontend:__subpackages__",
        "//lib/main:__pkg__",
        "//test:__subpackages__",
    ],
    deps = [
        "//lib/frontend/common",
        "//lib/frontend/syntax",
        "//lib/support",
    ],
)
