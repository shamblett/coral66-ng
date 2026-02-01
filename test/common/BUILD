#
# Test suite common library
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "common",
    srcs = ["TestConfig.cpp"],
    hdrs = [
        "TestConfig.hpp",
        "TestTargets.hpp",
    ],
    visibility = ["//test:__subpackages__"],
    deps = [
        "//support",
        "//frontend",
        "//test/vendor/catch2v3",
    ],
)
