#
# Compliance tests
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "compliancetests",
    srcs = [
        "section_1_tests.cpp",
        "section_2_tests.cpp",
        "section_3_tests.cpp",
        "section_4_tests.cpp",
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
    name = "compliance",
    srcs = [
        "main.cpp",
    ],
    data = [
        ":data_dir",
    ],
    env = {
        "COMPLIANCE_DATA_DIR": "test/compliance/data",
    },
    visibility = ["//test:__pkg__"],
    deps = [
        "compliancetests",
    ],
)
