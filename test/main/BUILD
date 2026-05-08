#
# Test suite frontend library tests
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

filegroup(
    name = "data_dir",
    srcs = ["data"],
)

cc_library(
    name = "maintest",
    srcs = [
        "command_line_tests.cpp",
        "main_tests.cpp",
    ],
    visibility = ["//test:__pkg__"],
    deps = [
        "//lib/frontend/common",
        "//lib/main",
        "//lib/support",
        "//test/common",
        "//test/vendor/catch2v3",
    ],
)

cc_test(
    name = "main",
    srcs = [
        "main.cpp",
    ],
    data = [
        ":data_dir",
    ],
    env = {
        "DATA_DIR": "$(rootpath :data_dir)",
    },
    visibility = ["//test:__pkg__"],
    deps = [
        "maintest",
    ],
)
