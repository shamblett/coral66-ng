# Preprocessor Documentation

This document describes the main data structures, flow of control, and specific algorithms used in the `preprocessor` component of the Coral66 compiler.

## Main Data Structures

1. **Syntax Tree Nodes (`preprocessor_syntax.hpp`)**
   The preprocessor parses the raw token stream into a lightweight abstract syntax tree (AST) to distinguish between macro definitions and normal code text.
   - **`File`**: The root node representing the entire source unit being preprocessed. It holds a sequence of `Group`s.
   - **`Group`**: A structural container holding a sequence of `GroupPart`s.
   - **`GroupPart`**: A `std::variant` representing a segment of the source. It can be a `TextBlock` (code to be evaluated and macro-expanded), a `ControlLine` (a macro directive).
   - **`TextBlock`**: Represents a contiguous span of `PPToken`s that are not part of a preprocessor directive.
   - **`ControlLine`**: Represents a macro directive, holding variants like `DefineDirective`.
   - **`DefineDirective`**: Captures a Coral66 `DEFINE` statement. It stores the identifier token and the sequence of tokens forming the replacement body.

2. **Preprocessor Environment (`preprocessor.hpp`)**
   - **`Preprocessor` Class**: The main execution engine for preprocessing, implementing the `PPSourceInterface`.
   - **`Macro`**: An internal struct representing an active macro definition. It holds:
     - `identifierPos`: The token representing the macro's name.
     - `argumentList`: An optional list of formal parameters for function-like macros.
     - `replacement`: The sequence of tokens that will replace the macro invocation.
   - **`m_defines`**: A symbol table (`std::unordered_map`) mapping macro identifier strings to a stack (vector) of `Macro` definitions, managing scoping and definitions.

3. **`Context` (`preprocessor_parser.hpp`)**
   - A helper class instantiated during the parsing phase. It tracks the source interface, accumulates error counts, and routes diagnostic messages.

## Flow of Control

Unlike the C preprocessor which is strictly line-oriented and uses `#`, the Coral66 preprocessor relies on specific inline language keywords (`DEFINE` and `DELETE`). Processing occurs in two logical phases:

1. **Parsing Phase (`preprocessor_parser.cpp`)**
   - The stream of Preprocessor Tokens (`PPToken`) emitted by the lexer is fed into `parseMacro`.
   - `parseMacro` orchestrates the token stream into `Group`s by calling `parseGroup`.
   - `parseGroup` iterates sequentially over the tokens. It buffers standard tokens into `TextBlock`s. When it encounters the `DEFINE` or `DELETE` keywords, it flushes the current `TextBlock` and delegates to `parseControlLine`.
   - `parseControlLine` parses the Coral66 specific macro definition syntax (which terminates with a semicolon) and builds a `ControlLine` variant holding the directive's data.

2. **Execution and Expansion Phase (`preprocessor.cpp`)**
   - The `Preprocessor` iterates over the AST (`File` -> `Group` -> `GroupPart`).
   - **Processing Directives**: When a `ControlLine` containing a `DefineDirective` is evaluated, the preprocessor validates the definition (`parseDefineDirective`), resolves any parameter lists, and registers the new `Macro` in the `m_defines` map.
   - **Macro Expansion**: When evaluating a `TextBlock`, the preprocessor scans the sequence of tokens using `macroSubstitute`.
     - It looks for `Identifier` tokens.
     - If an identifier matches an entry in `m_defines`, it initiates substitution.
     - For function-like macros, it parses the actual arguments from the token stream at the invocation site.
     - The arguments are mapped to the parameters, and the `replacement` tokens are injected into the output stream.

## Special Algorithms & Techniques

- **Keyword-Triggered Directives**: The preprocessor parsing logic explicitly looks for `DefineKeyword` and `DeleteKeyword` to isolate macro instructions from normal code, tracking until a terminating `SemiColon` is found.
- **Variant-Based AST**: Much like the lexer, the preprocessor syntax nodes heavily use `std::variant` (e.g., `GroupPart`, `ControlLine`). This provides a safe, polymorphic representation of file structures without virtual method overhead.
- **Substitution Provenance (`m_substitutions`)**: When a macro is expanded, the preprocessor creates a `Source::Substitution` record and assigns a new `MacroID` to the replacement tokens. This precise provenance tracking allows the compiler to generate accurate diagnostic messages that point both to the expanded token's location and the original macro definition site.
- **Two-Pass Separation**: By clearly separating the identification of macro boundaries (Parsing Phase) from the actual token replacement (Execution Phase), the preprocessor simplifies handling complex scenarios like nested definitions or function-like macro arguments that span multiple tokens.
