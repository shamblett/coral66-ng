#
# Test suite frontend semantics library tests
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

filegroup(
    name = "benchmark_dir",
    srcs = ["data/benchmark"],
)

filegroup(
    name = "support_dir",
    srcs = ["data/support"],
)

filegroup(
    name = "generated_dir",
    srcs = ["data/generated"],
)

filegroup(
    name = "golden_dir",
    srcs = ["data/golden"],
)

cc_library(
    name = "semanticstest",
    srcs = [
        "benchmark_tests.cpp",
        "cmi_processing_tests.cpp",
        "communicator_tests.cpp",
        "control_flow_tests.cpp",
        "generated_tests.cpp",
        "golden_tests.cpp",
        "support_tests.cpp",
        "symbol_resolution_tests.cpp",
        "symbol_table_tests.cpp",
        "type_check_tests.cpp",
    ],
    visibility = ["//test:__pkg__"],
    deps = [
        "//lib/frontend/common",
        "//lib/frontend/parser",
        "//lib/frontend/semantics",
        "//lib/support",
        "//test/common",
        "//test/frontend/common",
        "//test/vendor/catch2v3",
        "//tools/semantics/programdump/lib:programdump",
    ],
)

cc_test(
    name = "semantics",
    srcs = [
        "main.cpp",
    ],
    data = [
        ":benchmark_dir",
        ":generated_dir",
        ":golden_dir",
        ":support_dir",
    ],
    env = {
        "BENCHMARK_DIR": "$(rootpath :benchmark_dir)",
        "GENERATED_DIR": "$(rootpath :generated_dir)",
        "GOLDEN_DIR": "$(rootpath :golden_dir)",
        "SUPPORT_DIR": "$(rootpath :support_dir)",
    },
    visibility = ["//test:__pkg__"],
    deps = [
        "semanticstest",
    ],
)
