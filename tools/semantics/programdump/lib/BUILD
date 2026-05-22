#
# Semantics program Dump library
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "programdump",
    srcs = [
        "program_dump.cpp",
        "program_dump_support.cpp",
    ],
    hdrs = [
        "program_dump.hpp",
    ],
    visibility = [
        "//test:__subpackages__",
        "//tools/semantics/programdump/bin:__pkg__",
    ],
    deps = [
        "//lib/frontend/common",
        "//lib/frontend/parser",
        "//lib/frontend/semantics",
    ],
)
