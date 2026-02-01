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
        "SemanticAnalysisAttributes.cpp",
        "SemanticAnalysis.cpp",
        "SemanticAnalysisBuiltins.cpp",
        "SemanticAnalysisDeclarations.cpp",
        "SemanticAnalysisExpressions.cpp",
        "SemanticAnalysisStatements.cpp",
        "SemanticAnalysisTypes.cpp",
        "Semantics.cpp",
        "SourceObject.cpp",
        "Syntax.cpp"
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
        "Semantics.hpp",
        "SemanticUtilities.hpp",
        "SourceObject.hpp",
        "Syntax.hpp",
    ],
    visibility = [
        "//main:__pkg__",
        "//test:__subpackages__",
    ],
    deps = [
        "//frontend/common",
        "//support",
    ],
)
