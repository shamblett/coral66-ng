#
# Frontend lexer library
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "lexer",
    srcs = [
        "lexer.cpp",
    ],
    hdrs = [
        "lexer.hpp",
    ],
    visibility = [
        "//lib/backend:__subpackages__",
        "//lib/frontend:__subpackages__",
        "//lib/main:__pkg__",
        "//test:__subpackages__",
    ],
    deps = [
        "//lib/frontend/common",
        "//lib/support",
    ],
)
