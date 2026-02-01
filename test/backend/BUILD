#
# Test suite backend library tests
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "backendtestlib",
    srcs = ["CodeGeneratorTests,cpp",
            "Common.cpp",
    ],
    hdrs = ["Common.hpp", 
    ]
    visibility = ["//test:__pkg__"],
    deps = [
         "//test/vendor/catch2v3",
         "//test/common",
         "//support",
    ],
)

cc_test(
    name = "frontend",
    srcs = [
        "main.cpp",
    ]
    visibility = ["//test:__pkg__"],
    deps = [
        "backendtestlib",

    ],
)
