#
# Test suite support library tests
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "supporttestlib",
    srcs = [
        "IntrusiveVariantAllocatorTests.cpp",
        "IntrusiveVariantValueTests.cpp",
    ],
    visibility = ["//test:__pkg__"],
    deps = [
        "//support",
        "//test/vendor/catch2v3",
    ],
)

cc_test(
    name = "support",
    srcs = [
        "main.cc",
    ],
    visibility = ["//test:__pkg__"],
    deps = [
        "supporttestlib",
        "//support",
        "//test/common",
    ],
)
