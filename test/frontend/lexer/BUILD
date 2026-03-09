#
# Test suite frontend lexer library tests
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "lexertest",
    srcs = [
        "invalid_tokens_tests.cpp",
        "valid_tokens_tests.cpp",
    ],
    visibility = ["//test:__pkg__"],
    deps = [
        "//lib/frontend/common",
        "//lib/frontend/lexer",
        "//lib/support",
        "//test/common",
        "//test/vendor/catch2v3",
    ],
)

cc_test(
    name = "lexer",
    srcs = [
        "main.cpp",
    ],
    visibility = ["//test:__pkg__"],
    deps = [
        "lexertest",
    ],
)
