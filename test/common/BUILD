#
# Test suite common library
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "common",
    srcs = [
        "test_config.cpp",
    ],
    hdrs = [
        "test_config.hpp",
        "test_targets.hpp",
    ],
    visibility = ["//test:__subpackages__"],
    deps = [
        "//lib/frontend/common",
        "//lib/frontend/compiler",
        "//lib/support",
        "//test/vendor/catch2v3",
    ],
)
