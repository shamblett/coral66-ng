#
# Compliance tests section 1
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "section3test",
    srcs = [
        "section_3_tests.cpp",
    ],
    visibility = ["//test:__pkg__"],
    deps = [
        "//lib/backend",
        "//lib/frontend/common",
        "//lib/support",
        "//test/common",
        "//test/compliance/common",
        "//test/vendor/catch2v3",
    ],
)

filegroup(
    name = "data_dir",
    srcs = glob(["data/**"]),
)

cc_test(
    name = "section3",
    srcs = [
        "main.cpp",
    ],
    data = [
        ":data_dir",
    ],
    env = {
        "COMPLIANCE_SECTION_3_DIR": "test/compliance/section-3/data",
    },
    visibility = ["//test:__pkg__"],
    deps = [
        "section3test",
    ],
)
