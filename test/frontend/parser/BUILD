#
# Test suite frontend parser library tests
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "parsertest",
    srcs = [
        "communicator_tests.cpp",
        "declaration_tests.cpp",
        "error_tests.cpp",
        "expression_tests.cpp",
        "place_reference_tests.cpp",
        "procedure_tests.cpp",
        "program_tests.cpp",
        "statement_tests.cpp",
    ],
    hdrs = [
        "common.hpp",
    ],
    visibility = ["//test:__pkg__"],
    deps = [
        "//lib/frontend/common",
        "//lib/frontend/parser",
        "//lib/support",
        "//test/common",
        "//test/vendor/catch2v3",
    ],
)

cc_test(
    name = "parser",
    srcs = [
        "main.cpp",
    ],
    visibility = ["//test:__pkg__"],
    deps = [
        "parsertest",
    ],
)
