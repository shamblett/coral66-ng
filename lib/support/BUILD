#
# Support library
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "support",
    srcs = [
        "Filesystem.cpp",
        "Text.cpp",
        "Triple.cpp",
        "Utilities.cpp",
    ],
    hdrs = [
        "AbstractIntrusiveVariant.hpp",
        "Constexpr.hpp",
        "Expected.hpp",
        "Filesystem.hpp",
        "FunctionRef.hpp",
        "IntrusiveVariantAllocator.hpp",
        "IntrusiveVariantValue.hpp",
        "MaxVector.hpp",
        "ScopeExit.hpp",
        "Text.hpp",
        "Triple.hpp",
        "Utilities.hpp",
        "ValueReset.hpp",
        "vendor/tsl/ordered_hash.h",
        "vendor/tsl/ordered_map.h",
    ],
    visibility = [
        "//frontend:__subpackages__",
        "//backend:__subpackages__",
        "//main:__pkg__",
        "//test:__subpackages__",
    ],
    deps = ["//llvm:llvm-21"],
)
