#
# Frontend syntax library
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "syntax",
    srcs = [
        "syntax.cpp",
    ],
    hdrs = [
        "syntax.hpp",
    ],
    visibility = [
        "//lib/backend:__subpackages__",
        "//lib/frontend:__subpackages__",
        "//lib/main:__pkg__",
        "//test:__subpackages__",
    ],
    deps = [
        "//lib/frontend/common",
        "//lib/frontend/lexer",
        "//lib/support",
    ],
)
