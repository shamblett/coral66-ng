#
# Frontend compiler library
#

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "compiler",
    srcs = [
        "Lexer.cpp",
        "Parser.cpp",
        "ParserDeclarations.cpp",
        "ParserExpressions.cpp",
        "ParserUtilities.cpp",
        "Preprocessor.cpp",
        "PreprocessorParser.cpp",
        "PreprocessorSyntax.cpp",
        "Program.cpp",
        "ProgramInterface.cpp",
        "SemanticAnalysis.cpp",
        "SemanticAnalysisAttributes.cpp",
        "SemanticAnalysisBuiltins.cpp",
        "SemanticAnalysisDeclarations.cpp",
        "SemanticAnalysisExpressions.cpp",
        "SemanticAnalysisStatements.cpp",
        "SemanticAnalysisTypes.cpp",
        "Semantics.cpp",
        "Syntax.cpp",
    ],
    hdrs = [
        "Lexer.hpp",
        "Parser.hpp",
        "ParserUtilities.hpp",
        "Preprocessor.hpp",
        "PreprocessorParser.hpp",
        "PreprocessorSyntax.hpp",
        "Program.hpp",
        "ProgramInterface.hpp",
        "SemanticAnalysis.hpp",
        "SemanticUtilities.hpp",
        "Semantics.hpp",
        "Syntax.hpp",
    ],
    visibility = [
        "//lib/backend:__pkg__",
        "//lib/main:__pkg__",
        "//test:__subpackages__",
    ],
    deps = [
        "//lib/frontend/common",
        "//lib/support",
    ],
)
