# Frontend Data Flow Analysis

This document traces the data flow through the frontend stages of the `cld` compiler, from raw source code to the semantic abstract syntax tree (AST).

## Overview

The compilation process is orchestrated by `cld::compileCFile` in `src/cld/cldMain/cldMain.cpp`. It pipelines the source code through a series of transformations: Lexing (PP) -> Preprocessing -> Lexing (C) -> Parsing -> Semantic Analysis.

## Stage 1: Lexing (Preprocessor Tokens)

The first stage converts the raw source code string into a stream of Preprocessor Tokens.

*   **Initiating Function**: `cld::Lexer::tokenize(...)`
*   **Input**:
    *   `std::string` (Source Code)
    *   `cld::LanguageOptions` (C standard version, extensions, etc.)
*   **Output**: `cld::PPSourceObject`
    *   Contains `std::vector<cld::Lexer::PPToken>`
*   **Explanation**: This stage handles low-level lexing tasks such as trigraph replacement, line splicing (backslash-newline), and grouping characters into basic tokens (identifiers, numbers, punctuators) without interpreting them as C-specific constructs yet.

## Stage 2: Preprocessing

The preprocessor consumes the initial token stream, handles directives (macros, includes), and produces a clean stream of tokens ready for C analysis.

*   **Initiating Function**: `cld::PP::preprocess(...)`
*   **Input**:
    *   `cld::PPSourceObject` (from Stage 1)
    *   `cld::PP::Options` (Include paths, macro definitions)
*   **Output**: `cld::PPSourceObject`
    *   A new `PPSourceObject` containing the expanded stream of `PPToken`s.
*   **Explanation**: Resolves `#include` directives, expands macros, handles `#ifdef`/`#ifndef` conditionals, and removes comments.

## Stage 3: Lexing (C Tokens)

The preprocessed tokens are converted into C-specific tokens.

*   **Initiating Function**: `cld::Lexer::toCTokens(...)`
*   **Input**: `cld::PPSourceObject` (from Stage 2)
*   **Output**: `cld::CSourceObject`
    *   Contains `std::vector<cld::Lexer::CToken>`
*   **Explanation**: This step bridges the gap between the preprocessor and the C language. It resolves keywords (which might have been concealed by macros), converts literals to their typed representations (e.g., `APSInt`, `APFloat`), and prepares the token stream for the parser.

## Stage 4: Parsing (Syntax Analysis)

The parser consumes the C tokens and builds a raw syntactic AST that mirrors the C grammar.

*   **Initiating Function**: `cld::Parser::buildTree(...)`
*   **Input**: `cld::CSourceObject` (from Stage 3)
*   **Output**: `std::optional<cld::Syntax::TranslationUnit>`
*   **Explanation**: A recursive descent parser that constructs a tree based on the EBNF grammar of C99. The resulting `Syntax::TranslationUnit` and its children (defined in `Syntax.hpp`) represent the code structure (e.g., `IfStatement`, `BinaryExpression`) but lack semantic information like types or symbol resolution.

## Stage 5: Semantic Analysis

The final frontend stage validates the raw AST and transforms it into a fully typed and resolved semantic AST.

*   **Initiating Function**: `cld::Semantics::analyse(...)`
*   **Input**:
    *   `cld::Syntax::TranslationUnit` (from Stage 4)
    *   `cld::CSourceObject` (Source context)
*   **Output**: `cld::Semantics::Program`
    *   Contains `cld::Semantics::TranslationUnit`
*   **Explanation**:
    *   **Validation**: Checks types, scopes, variable lifetimes, and adherence to C99 constraints.
    *   **Transformation**: Converts `Syntax::Node`s into `Semantics` objects (defined in `Semantics.hpp`).
    *   **Result**: The `Semantics::TranslationUnit` is a list of `Useable` globals (Functions, Variables). Nodes in this tree are fully typed (e.g., expressions have a `Type` and `ValueCategory`). This "Semantic AST" is the input for the LLVM Backend code generation.
