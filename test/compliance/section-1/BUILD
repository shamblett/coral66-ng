#
# Compliance tests section 1
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "section1test",
    srcs = [
        "section_1_tests.cpp",
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
    name = "section1",
    srcs = [
        "main.cpp",
    ],
    data = [
        ":data_dir",
    ],
    env = {
        "COMPLIANCE_SECTION_1_DIR": "test/compliance/section-1/data",
    },
    visibility = ["//test:__pkg__"],
    deps = [
        "section1test",
    ],
)
