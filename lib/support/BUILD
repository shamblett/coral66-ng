#
# Support library
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "support",
    srcs = [
        "filesystem.cpp",
        "text.cpp",
        "triple.cpp",
        "utilities.cpp",
    ],
    hdrs = [
        "abstract_intrusive_variant.hpp",
        "constexpr.hpp",
        "filesystem.hpp",
        "function_ref.hpp",
        "intrusive_variant_allocator.hpp",
        "intrusive_variant_value.hpp",
        "max_vector.hpp",
        "scope_exit.hpp",
        "text.hpp",
        "triple.hpp",
        "utilities.hpp",
        "value_reset.hpp",
        "vendor/ctrack/include/ctrack.hpp",
        "vendor/magic_enum/magic_enum.hpp",
        "vendor/tsl/ordered_hash.h",
        "vendor/tsl/ordered_map.h",
    ] + glob(["vendor/rapidjson/**/*.h"]),
    visibility = [
        "//lib/backend:__subpackages__",
        "//lib/frontend:__subpackages__",
        "//lib/main:__pkg__",
        "//test:__subpackages__",
        "//tools:__subpackages__",
    ],
    deps = ["//lib/llvm:llvm-22"],
)
