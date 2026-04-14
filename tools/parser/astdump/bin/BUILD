#
# Parser AST dumper binary
#

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

cc_binary(
    name = "astdump",
    srcs = ["astdump.cpp"],
    deps = [
        "//tools/parser/astdump/lib:astdump",
    ],
)
