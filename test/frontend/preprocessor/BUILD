#
# Test suite frontend preprocessor library tests
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "pptest",
    srcs = [
        "error_tests.cpp",
        "macro_tests.cpp",
        "note_tests.cpp",
    ],
    visibility = ["//test:__pkg__"],
    deps = [
        "//lib/frontend/common",
        "//lib/frontend/preprocessor",
        "//lib/support",
        "//test/common",
        "//test/vendor/catch2v3",
    ],
)

cc_test(
    name = "preprocessor",
    srcs = [
        "main.cpp",
    ],
    visibility = ["//test:__pkg__"],
    deps = [
        "pptest",
    ],
)
