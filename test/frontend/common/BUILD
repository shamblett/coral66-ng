#
# Test suite frontend common library
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")

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
