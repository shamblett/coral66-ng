#
# LLVM versions should have their own lib in here
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "llvm-21",
    linkopts = [
        "-L /usr/lib64/llvm21/lib64/ -lLLVM",
    ],
    visibility = ["//support:__pkg__"],
)
