#
# Compliance tests common
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "compliancecommon",
    srcs = [
        "common.cpp",
    ],
    hdrs = [
        "common.hpp",
    ],
    visibility = ["//test:__subpackages__"],
    deps = [
        "//lib/backend",
        "//lib/frontend/common",
        "//lib/support",
        "//test/common",
    ],
)
