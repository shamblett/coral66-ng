#
# Test suite frontend semantics library tests
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "semanticstest",
    srcs = [
        "semantics_symbol_resolution_tests.cpp",
        "semantics_type_check_tests.cpp",
        "symbol_table_tests.cpp",
    ],
    visibility = ["//test:__pkg__"],
    deps = [
        "//lib/frontend/common",
        "//lib/frontend/parser",
        "//lib/frontend/semantics",
        "//lib/support",
        "//test/common",
        "//test/frontend/parser:parsertest",
        "//test/vendor/catch2v3",
    ],
)

cc_test(
    name = "semantics",
    srcs = [
        "main.cpp",
    ],
    visibility = ["//test:__pkg__"],
    deps = [
        "semanticstest",
    ],
)
