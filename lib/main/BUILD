#
# Main library
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "main",
    srcs = [
        "action_executor.cpp",
        "command_line_processor.cpp",
        "command_line_arguments.cpp",
        "main.cpp",
    ],
    hdrs = [
        "action_executor.hpp",
        "command_line_arguments.hpp",
        "command_line_processor.hpp",
        "main.hpp",
        "version.hpp",
    ],
    visibility = [
        "//bin:__pkg__",
        "//test:__subpackages__",
    ],
    deps = [
        "//lib/backend",
        "//lib/frontend/common",
        "//lib/frontend/preprocessor",
        "//lib/frontend/semantics",
        "//lib/support",
    ],
)
