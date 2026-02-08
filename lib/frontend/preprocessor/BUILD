#
# Frontend preprocessor library
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "preprocessor",
    srcs = [
        "preprocessor.cpp",
        "preprocessor_parser.cpp",
        "preprocessor_syntax.cpp",
    ],
    hdrs = [
        "preprocessor.hpp",
        "preprocessor_parser.hpp",
        "preprocessor_syntax.hpp",
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
        "//lib/frontend/parser",
        "//lib/frontend/syntax",
        "//lib/support",
    ],
)
