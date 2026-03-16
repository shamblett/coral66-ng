# Coral 66 Lexer Conformance Review

This report provides a review of the lexer implementation located in `lib/frontend/lexer` for conformance against the official Coral 66 language specification documents (`Official Definition of Coral 66.txt` and `coral66-grammar.txt`).

## 1. Functionality Mandated by the Specification but NOT Supported by the Lexer

### 1.1 Multi-Line String Literals
* **Specification:** Section 10.4 defines a string as *"any sequence of characters enclosed in quotes"*. The specification does not restrict strings to a single line; newlines are valid characters within a sequence.
* **Lexer Implementation:** In `lexer_support.cpp`, the `StringLiteral::advance` state machine explicitly checks for newline characters (`\n` or `\r`). If one is encountered, the lexer immediately emits an `UNTERMINATED_STRING_LITERAL` error and ends the string. This incorrectly prevents strings from spanning multiple lines.

### 1.2 Context-Aware Parsing of the Exponent Symbol (`10`)
* **Specification:** In Coral 66, the symbol `10` is defined as a representation for the exponent symbol ("times ten to the power of") in floating-point literals, e.g., `2.5 10 -3` (Section 10.2 & Appendix 2).
* **Lexer Implementation:** The lexer (in `lexer.cpp`'s `toCTokens` function) attempts to implement this by greedily combining the integer literal `10` with a subsequent sign and integer into a `Floating` token. Because this logic is executed at the lexer level rather than the parser level, it incorrectly captures valid integer arithmetic expressions. For example, a valid expression like `10 - 2` will be erroneously consumed by the lexer and converted into `10^-2` (a floating-point value of `0.01`), breaking standard arithmetic. Resolving the ambiguity between the integer `10` and the exponent `10` requires syntactic context and should not be aggressively resolved by the lexer.

### 1.3 The Explicit Assignment Symbol (`<-`)
* **Specification:** Section 7.1 and Appendix 2 define the left arrow `<-` as the primary assignment symbol. While `:=` is permitted as an alternate representation when the hardware alphabet is restricted, `<-` is the official, mandated symbol.
* **Lexer Implementation:** The lexer only recognizes `:=` as the `Assignment` token (handling `:` followed by `=`). If the sequence `<-` is present in the source, it is parsed as two distinct tokens (`LessThan` and `Minus`).

## 2. Functionality in the Lexer Code NOT Mandated by the Specification

### 2.1 C/C++ Style Trigraphs
* **Lexer Implementation:** The `normalizeSpelling` function in `lexer.cpp` implements C-style trigraph substitution (e.g., converting `??=` to `#`, `??(` to `[`, `??/` to `\`, etc.). 
* **Specification:** Coral 66 has no concept of trigraphs. This is an anachronistic addition borrowed from the C/C++ preprocessor.

### 2.2 Backslash-Newline Line Splicing
* **Lexer Implementation:** Also within `normalizeSpelling`, the lexer strips sequences of a backslash followed by a newline (`\` + `\n`), effectively splicing two physical lines into a single logical line.
* **Specification:** Line continuation splicing via a backslash is a feature of the C/C++ preprocessor and is entirely absent from the Coral 66 specification.

### 2.3 `PROGRAM` and `SEGMENT` as Reserved Keywords
* **Lexer Implementation:** The lexer defines `PROGRAM` and `SEGMENT` as explicit keywords (`ProgramKeyword`, `SegmentKeyword` in `TokenType`) and tokenizes them as such.
* **Specification:** The official grammar does not define `PROGRAM` or `SEGMENT` as keywords (they do not appear in Appendix 2's comprehensive list of language symbols). Section 2.2 explicitly states: *"The language definition does not specify how the program or its segments shall be named or how the segments are to be separated or terminated."* Thus, reserving these terms as language keywords is an implementation-specific extension.

### 2.4 Unicode Hex Escape Fallbacks
* **Lexer Implementation:** When the lexer encounters a non-printable character, it reports an error and formats the output into a C++ style `\U........` Unicode hexadecimal escape sequence.
* **Specification:** Modern Unicode escape sequences and the handling of non-printable characters via `\U` sequences were not part of the 1970 Coral 66 standard.

### 2.5 UTF-8 BOM and Windows Line Ending (`\r\n`) Normalization
* **Lexer Implementation:** The lexer explicitly checks for and skips a UTF-8 Byte Order Mark (`\xEF\xBB\xBF`) at the beginning of the file. It also preprocesses the source string to strip carriage returns (`\r`), converting Windows-style `\r\n` line endings into Unix-style `\n`.
* **Specification:** These are modern, environment-specific text processing accommodations that are not mandated by the Coral 66 language specification.