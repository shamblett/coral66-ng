#
# Main library
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "main",
    srcs = [
        "command_line.cpp",
        "main.cpp",
    ],
    hdrs = [
        "command_line.hpp",
        "main.hpp",
    ],
    visibility = [
        "//bin:__pkg__",
        "//test:__subpackages__",
    ],
    deps = [
        "//lib/backend",
        "//lib/frontend/common",
        "//lib/frontend/preprocessor",
        "//lib/frontend/semantics",
        "//lib/support",
    ],
)
