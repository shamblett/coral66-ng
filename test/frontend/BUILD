#
# Test suite frontend library tests
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "frontendtestlib",
    srcs = [
        "LexerTests.cpp",
        "Parser.cpp",
        "PreprocessorTests",
        "SemanticsTests",
    ],
    visibility = ["//test:__pkg__"],
    deps = [
        "//lib/support",
        "//test/common",
        "//test/vendor/catch2v3",
    ],
)

cc_test(
    name = "frontend",
    srcs = [
        "main.cc",
    ],
    visibility = ["//test:__pkg__"],
    deps = [
        "frontendtestlib",
    ],
)
