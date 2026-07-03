# Lexer Documentation

This document describes the main data structures, flow of control, and specific algorithms used in the `lexer` component of the Coral66 compiler.

## Main Data Structures

1. **Tokens (`TokenBase`, `PPToken`, `CToken`)**
   - **`TokenBase`**: Base class that stores information about a token, such as its `TokenType` (defined in `lexer_common.hpp`), position (offset, length, fileID), and a macro ID if it was inserted as part of a macro expansion.
   - **`PPToken`**: Preprocessor Token. These are the initial tokens emitted by the lexer before macro expansions and other preprocessing steps have been executed.
   - **`CToken`**: Coral66 Token. Produced after preprocessing, these are the tokens fed into the Coral66 parser.

2. **Source Objects (`PPSourceObject`, `CSourceObject`)**
   - These are specializations of a template class `SourceObject<T>` that hold vectors of tokens and provide an interface for iterating and querying them.

3. **`Context`**
   - A helper class instantiated during tokenization. It accumulates tokens, stores error states, reports errors, and maintains mappings between the normalized token offsets and the actual physical source file offsets (handling trigraphs and line continuations).

4. **`StateMachine` (std::variant)**
   - The lexer implements a finite state machine using a C++ `std::variant`. The variant holds one of several states representing the current execution context of the lexer:
     - `Start`: The initial state waiting for the start of a new token.
     - `Text`: Parsing keywords, identifiers, or other textual blocks.
     - `CharacterLiteral` / `StringLiteral`: Parsing string/character literal values.
     - `PreprocessingNumber`: Parsing numbers.
     - `UniversalCharacter` / `MaybeUC`: Handling universal character names (e.g., Unicode escape sequences).
     - `Dot`, `Punctuation`, `L`: Intermediate states for handling specific operator and symbol sequences.

## Flow of Control

The primary entry point for lexing is the `tokenize` function (located in `lexer.cpp`), which follows a well-defined pipeline:

1. **Source Normalization Phase**
   - The lexer first performs a cleanup pass over the raw source string:
     - Detects and removes the UTF-8 Byte Order Mark (BOM) if present.
     - Normalizes line endings (replaces `\r\n` with `\n`).
     - Performs trigraph replacement and processes line continuations (a backslash followed by a newline).
   - *Algorithm Detail*: To preserve the ability to report accurate diagnostic positions, it builds an `IntervalMap` (`characterToSourceSpace`). This map keeps track of the delta between the "normalized character space" and the original "source character space" so that errors and token locations are reported correctly.

2. **State Machine Execution Phase**
   - A loop iterates through the normalized string space character by character (or by UTF-32 codepoints, depending on the current state's requirements).
   - In each iteration, a polymorphic visitor accesses the active state object in the `StateMachine` variant and invokes its `advance` method.
   - The `advance` method consumes the character, alters internal state, and optionally pushes new tokens to the `Context` object. It returns the next state of the state machine (or itself).
   - If a character signifies the end of a token in a given state, the token is recorded in `Context` and the state transitions back to `Start`. Sometimes, the character is not consumed and is "put back" to be immediately evaluated under the newly transitioned state.

3. **Token Conversion Phase**
   - The lexer generates a `PPSourceObject` containing `PPToken`s.
   - The function `toCTokens` is later used to convert the `PPSourceObject` into a `CSourceObject` (containing `CToken`s), which incorporates handling of macro logic, ensuring that the token stream is ready for the parser.

## Special Algorithms & Techniques

- **Interval Mapping for Source Locations:** Rather than complicating the state machine with line continuation and trigraph logic, the lexer pre-processes these out into a contiguous sequence while mapping intervals using binary searches (`std::lower_bound`). This allows the main state machine to be clean and fast while maintaining perfect source provenance mapping.
- **Variant-Based State Machine:** The lexer leverages modern C++ techniques (`std::variant` and `std::visit`-like dispatch) to create a type-safe finite state machine. This replaces the traditional switch-case monster loop seen in older C-style lexers, isolating the logic for each state into dedicated structs with explicit memory layout.
- **UTF-8 to UTF-32 Conversion:** Depending on the token being parsed, the lexer automatically determines via type traits if the state's `advance` function requires a `char` or a `std::uint32_t` (UTF-32 codepoint) and seamlessly decodes UTF-8 sequences when necessary.