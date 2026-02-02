#
# Frontend compiler library
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "backend",
    srcs = [
        "ABIImplementation.cpp",
        "CodeGeneration.cpp",
        "CodeGenerator.cpp",
        "CodeGeneratorDeclarations.cpp",
        "CodeGeneratorExpressions.cpp",
        "CodeGeneratorStatements.cpp",
        "X64ABI.cpp",
    ],
    hdrs = [
        "ABIImplementation.hpp",
        "CodeGeneration.hpp",
        "CodeGenerator.hpp",
        "CommonABIImplementation.hpp",
        "TypeIdentity.hpp",
        "X64ABI.hpp",
    ],
    visibility = [
        "//lib/main:__pkg__",
        "//test:__subpackages__",
    ],
    deps = [
        "//lib/frontend/common",
        "//lib/frontend/compiler",
        "//lib/support",
    ],
)
