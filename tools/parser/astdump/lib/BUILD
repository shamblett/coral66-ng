#
# AST Dump library
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "astdump",
    srcs = [
        "ast_dump.cpp",
        "ast_dump_communicators.cpp",
        "ast_dump_declarations.cpp",
        "ast_dump_expressions.cpp",
        "ast_dump_statements.cpp",
    ],
    hdrs = [
        "ast_dump.hpp",
    ],
    visibility = [
        "//test:__subpackages__",
        "//tools/parser/astdump/bin:__pkg__",
    ],
    deps = [
        "//lib/frontend/common",
        "//lib/frontend/parser",
    ],
)
