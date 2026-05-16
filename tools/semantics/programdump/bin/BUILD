#
# Semantics program dumper binary
#

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

cc_binary(
    name = "programdump",
    srcs = ["programdump.cpp"],
    deps = [
        "//lib/frontend/parser",
        "//lib/frontend/preprocessor",
        "//lib/support",
        "//tools/semantics/programdump/lib:programdump",
    ],
)
