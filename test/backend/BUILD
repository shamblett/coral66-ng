#
# Test suite backend library tests
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "backendtest",
    srcs = [
        "code_generator_tests.cpp",
        "common.cpp",
    ],
    hdrs = [
        "common.hpp",
    ],
    visibility = ["//test:__pkg__"],
    deps = [
        "//lib/backend",
        "//lib/frontend/common",
        "//lib/support",
        "//test/common",
        "//test/vendor/catch2v3",
    ],
)

filegroup(
    name = "data_dir",
    srcs = glob(["data/**"]),
)

cc_test(
    name = "backend",
    srcs = [
        "main.cpp",
    ],
    data = [
        ":data_dir",
    ],
    env = {
        "BACKEND_DIR": "test/backend/data",
    },
    visibility = ["//test:__pkg__"],
    deps = [
        "backendtest",
    ],
)
