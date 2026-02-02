#
# Test suite frontend library tests
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "frontendtestlib",
    srcs = [
        "LexerTests.cpp",
        "ParserTests.cpp",
        "PreprocessorTests.cpp",
        "SemanticsTests.cpp",
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
        "main.cpp",
    ],
    visibility = ["//test:__pkg__"],
    deps = [
        "frontendtestlib",
    ],
)
