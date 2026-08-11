#
# Test suite frontend common library
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "common",
    hdrs = [
        "common.hpp",
    ],
    visibility = ["//test:__subpackages__"],
    deps = [
        "//lib/frontend/common",
        "//lib/frontend/preprocessor",
        "//lib/frontend/semantics",
        "//lib/support",
        "//test/vendor/catch2v3",
    ],
)

cc_library(
    name = "commontest",
    srcs = [
        "message_tests.cpp",
    ],
    visibility = ["//test:__pkg__"],
    deps = [
        "//lib/frontend/common",
        "//test/vendor/catch2v3",
    ],
)

cc_test(
    name = "common_test",
    srcs = [
        "main.cpp",
    ],
    visibility = ["//test:__pkg__"],
    deps = [
        "commontest",
        "//test/common",
    ],
)
