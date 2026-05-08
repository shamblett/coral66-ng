#
# Test suite frontend parser library tests
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

filegroup(
    name = "testdata",
    srcs = glob([
        "data/golden/ackermann/*",
        "data/golden/sieve/*",
        "data/golden/generated/*",
    ]),
)

cc_library(
    name = "parsertest",
    srcs = [
        "communicator_tests.cpp",
        "declaration_tests.cpp",
        "error_tests.cpp",
        "expression_tests.cpp",
        "golden_tests.cpp",
        "procedure_tests.cpp",
        "program_tests.cpp",
        "statement_tests.cpp",
    ],
    data = [
        "//test/frontend/parser:testdata",
    ],
    visibility = ["//test:__subpackages__"],
    deps = [
        "//lib/frontend/common",
        "//lib/frontend/parser",
        "//lib/support",
        "//test/common",
        "//test/frontend/common",
        "//test/vendor/catch2v3",
        "//tools/parser/astdump/lib:astdump",
    ],
)

cc_test(
    name = "parser",
    srcs = [
        "main.cpp",
    ],
    visibility = ["//test:__subpackages__"],
    deps = [
        "parsertest",
    ],
)
