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
    name = "generated_dir",
    srcs = ["data/generated"],
)

cc_library(
    name = "semanticstest",
    srcs = [
        "semantics_benchmark_tests.cpp",
        "semantics_control_flow_tests.cpp",
        "semantics_generated_tests.cpp",
        "semantics_symbol_resolution_tests.cpp",
        "semantics_type_check_tests.cpp",
        "symbol_table_tests.cpp",
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
    ],
    env = {
        "BENCHMARK_DIR": "$(rootpath :benchmark_dir)",
        "GENERATED_DIR": "$(rootpath :generated_dir)",
    },
    visibility = ["//test:__pkg__"],
    deps = [
        "semanticstest",
    ],
)
