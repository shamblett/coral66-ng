# Coral 66 Lexer Code Changes

This report outlines the specific architectural and code-level changes required to update the current C/C++ derived lexer in `lib/frontend/lexer` to fully comply with the Coral 66 language specification.

## 1. Source Normalization Simplification (`lexer.cpp`)

The current `tokenize` function performs C-specific source transformations that must be removed. This removal will also yield a massive performance improvement, as the source mapping step can be virtually eliminated.

* **Remove Trigraphs & Continuations:** Delete the entire logic block inside `tokenize` that scans for `?` and `\` to process trigraphs (e.g., `??=`) and backslash-newline line continuations. 
* **Eliminate `IntervalMap` Overhead:** Because trigraphs and continuations are not part of Coral 66, the normalized source string space is perfectly mapped 1:1 with the original source space (excluding the UTF-8 BOM and `\r\n` standardizations). The `IntervalMap` (`characterToSourceSpace`) logic can be heavily simplified or completely removed, directly emitting tokens with their original source offsets.

## 2. State Machine Pruning (`lexer_support.hpp` & `lexer_support.cpp`)

Several states in the variant-based `StateMachine` are exclusively for C/C++ and should be removed.

* **Remove Wide Characters:** Delete the `L` struct and its `advance` method. Remove `L` from the `StateMachine` variant.
* **Remove Universal Characters:** Delete the `MaybeUC` and `UniversalCharacter` structs and their `advance` methods. Remove them from the `StateMachine` variant.
* **Remove Extraneous Punctuators:** In `Start::advance`, remove the `case '{':` and `case '}':` branches. If encountered, these should now fall through to the default branch and be handled as `TokenType::Miscellaneous` (triggering an error).
* **Fix Numeric Parsing (`PreprocessingNumber`):** In `PreprocessingNumber::advance`, remove the logic that specifically looks for exponential notation (`e`, `E`, `p`, `P`) and sign characters (`+`, `-`) inside the number. Coral 66 handles scale via the literal `10` operator at the language grammar level, so the lexer should only emit basic integer and real (decimal) token strings.

## 3. Implementing Coral 66 Comment Parsing

Comments in Coral 66 are indivisible and must be ignored safely by the lexer to prevent internal syntax from triggering spurious tokens.

* **`CommentSentence` State:**
  * When the `Text` or `StroppedKeyword` state resolves the `COMMENT` keyword, the state machine should immediately transition into a new `CommentSentence` state.
  * `CommentSentence::advance` should consume all characters indiscriminately until it encounters a `;`. Once the semicolon is reached, it should drop the buffer, optionally emit a `CommentKeyword` token for fidelity, and return to the `Start` state.
* **`BracketedComment` State:**
  * Modify the `Context` or state machine to remember if the *last emitted token* was a `SemiColon`.
  * If the last token was a `SemiColon` and `Start::advance` encounters `(`, transition to a `BracketedComment` state instead of emitting an `OpenParentheses`.
  * `BracketedComment::advance` must maintain a nesting counter, incrementing on `(` and decrementing on `)`. When the counter reaches zero, the comment ends, the text is discarded, and it returns to `Start`.
* **`EndComment` State:**
  * When the `END` keyword is resolved, transition to a new `EndComment` state.
  * This state should consume whitespace. If the next sequence of characters forms an identifier, consume it entirely and discard it. If it forms any other token (like a `;`), put the character back and transition to `Start`.

## 4. Implementing Keyword Stropping (Dialect Support)

The provided test code (`restore.cor`) uses single quotes to delimit keywords (e.g., `'COMMENT'`, `'DEFINE'`). Currently, single quotes trigger a `StringLiteral`.

* **New `StroppedKeyword` State:** 
  * In `Start::advance`, change `case '\'':` to transition to a `StroppedKeyword` state.
  * `StroppedKeyword::advance` will accumulate characters until the closing `'`.
  * Once closed, verify the accumulated string using `isKeyword(buffer)`. If it is a valid keyword, emit the corresponding keyword token (e.g., `TokenType::CommentKeyword`). If not, handle it as an error or an identifier, depending on strict dialect rules.
* **Update `StringLiteral` State:**
  * `StringLiteral` should only be triggered by double quotes (`"`).

## 5. String and Literal Adjustments

* **Simplify String Parsing (`lexer.cpp`):**
  * Coral 66 strings do not support C-style escape sequences (like `\n` or `\t`).
  * The `parseStringLiteral` function should be vastly simplified. It no longer needs to decode escape sequences. It simply needs to extract the exact text between the matching `"`, ensuring nested quotes or specific rules (if the dialect requires them) are handled gracefully.
* **`OCTAL` and `LITERAL` Integration:**
  * Ensure the `OctalKeyword` and `LiteralKeyword` are correctly emitted by the lexer.
  * Depending on parser complexity, it may be beneficial to add a lexer pass (or handle it in `toCTokens`) that merges an `OctalKeyword`, `OpenParentheses`, `PPNumber`, and `CloseParentheses` into a single, unified integer token to simplify the syntactic analysis phase.

## 6. Token Definition Cleanups (`lexer_common.hpp`)

* Remove `TokenType::OpenBrace` and `TokenType::CloseBrace`.
* Ensure new states (`CommentSentence`, `BracketedComment`, etc.) do not accidentally leak tokens into the macro preprocessor unless strictly intended. Preprocessor tracking should ignore these correctly.