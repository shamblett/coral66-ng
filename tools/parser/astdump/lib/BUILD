#
# AST Dump library
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "astdump",
    srcs = [
        "ast_dump.cpp",
    ],
    hdrs = [
        "ast_dump.hpp",
    ],
    visibility = [
        "//test:__subpackages__",
    ],
    deps = [
        "//lib/frontend/common",
        "//lib/frontend/parser",
    ],
)
