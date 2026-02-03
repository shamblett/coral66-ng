load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//test:__subpackages__"])

cc_library(
    name = "catch2v3",
    srcs = ["catch_amalgamated.cpp"],
    hdrs = ["catch_amalgamated.hpp"],
    defines = ["CATCH_AMALGAMATED_CUSTOM_MAIN"],
    visibility = ["//test:__subpackages__"],
)
