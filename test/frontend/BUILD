#
# Test suite frontend library tests
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "frontendtest",
    srcs = [
        "lexer/invalid_tokens_tests.cpp",
        "lexer/valid_tokens_tests.cpp",
        "parser/parser_tests.cpp",
        "preprocessor/macro_tests.cpp",
        "semantics_tests.cpp",
    ],
    visibility = ["//test:__pkg__"],
    deps = [
        "//lib/frontend/common",
        "//lib/frontend/parser",
        "//lib/frontend/preprocessor",
        "//lib/frontend/semantics",
        "//lib/support",
        "//test/common",
        "//test/vendor/catch2v3",
    ],
)

cc_test(
    name = "frontend",
    srcs = [
        "main.cpp",
    ],
    visibility = ["//test:__pkg__"],
    deps = [
        "frontendtest",
    ],
)
