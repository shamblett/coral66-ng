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
        "//bin/coral66:__pkg__",
        "//test:__subpackages__",
    ],
    deps = [
        "//lib/backend",
        "//lib/frontend",
        "//lib/support",
    ],
)
