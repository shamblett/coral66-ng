#
# Main library
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "main",
    srcs = [
        "CommandLIne.cpp",
        "Main.cpp",
    ],
    hdrs = [
        "CommandLine.hpp",
        "Main.hpp",
    ],
    visibility = [
        "//bin:__pkg__",
        "//test:__subpackages__",
    ],
    deps = [
        "//lib/backend",
        "//lib/frontend/common",
        "//lib/frontend/compiler",
        "//lib/support",
    ],
)
