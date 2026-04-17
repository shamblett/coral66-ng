#
# Parser AST dumper binary
#

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

cc_binary(
    name = "astdump",
    srcs = ["astdump.cpp"],
    deps = [
        "//lib/frontend/parser",
        "//lib/frontend/preprocessor",
        "//lib/support",
        "//tools/parser/astdump/lib:astdump",
    ],
)
