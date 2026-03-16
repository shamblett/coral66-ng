# Coral 66 Lexer Conformance Review

Based on the official Coral 66 specification and the grammar rules provided in `documentation/specification`, an analysis of the lexer implementation in `lib/frontend/lexer` has been conducted. The following areas highlight discrepancies where the code either fails to support mandated functionality or includes superfluous features not part of the standard.

## 1. Functionality Not Fully Supported (Non-conformant / Missing)

### Nested Quotes in Strings and Macro Bodies
*   **Specification:** Section 11.2 and Appendix 1 define `String` and `Macrobody` as any "sequence of characters with quotes matched" or "in which string quotes are matched". This mandates support for balanced/nested quote characters within the string or macro definition.
*   **Implementation:** The `StringLiteral::advance` state machine simply terminates upon encountering the first unescaped double-quote (`"`). It lacks the logic to count or match nested quotes, meaning valid Coral 66 macro bodies containing matched quotes will be incorrectly truncated, leading to syntax errors or malformed tokens.

### Proper Floating-Point Exponents & Token Dropping Bug
*   **Specification:** Coral 66 defines real numbers using the `10` keyword/operator for exponents (e.g., `Digitlist 10 Signedinteger`). 
*   **Implementation:** The lexer does not natively group the `10` operator and its subsequent signed integer into a single numeric token, deferring this to the parser. However, the lexer attempts to parse C-style scientific notation (`e` or `E`) in `processNumber`. 
*   **Resulting Bug:** Because the `PreprocessingNumber` state machine terminates on `+` or `-`, a sequence like `1.0e-3` is split into three raw tokens: `1.0e`, `-`, and `3`. When `processNumber` attempts to evaluate `1.0e` using `llvm::APFloat::convertFromString`, it fails and returns an empty `std::optional`. The `toCTokens` function silently ignores empty results, dropping the float token from the output stream entirely without emitting an error diagnostic.

## 2. Superfluous Functionality (Not Mandated)

### C-Style Floating-Point Detection (`e` / `E`)
*   **Implementation:** The `processNumber` function explicitly checks for C-style exponents (`text.find('.') != std::string::npos || text.find('e') != std::string::npos || text.find('E') != std::string::npos`) to classify a token as a floating-point number.
*   **Analysis:** This is an extension inherited from modern C-family languages. Coral 66 strictly dictates the use of `10` for scale factors. This logic is not mandated by the specification and, as noted above, is currently broken when negative exponents are involved.

### Hexadecimal Support (`hexToValue`)
*   **Implementation:** The file `lexer_support.cpp` contains a `hexToValue` function that relies on `std::from_chars` with a base of 16.
*   **Analysis:** Coral 66 natively supports only decimal and octal numbers (the latter specifically via the `OCTAL` keyword). There is no provision for hexadecimal literals in the language. The `hexToValue` function is completely unused within the lexer and can be safely removed.

### C-Preprocessor Legacy Infrastructure (Trigraphs & Line Splicing)
*   **Implementation:** The `PPToken` class documentation explicitly states that its `m_charSpaceLength` field represents the token length after "trigraphs and Backslash Newline pairs" have been removed. Auxiliary template functions like `findUTF8StartByte` exist to support complex character space mappings.
*   **Analysis:** Line splicing (escaping newlines with backslashes) and trigraphs are features of the C/C++ preprocessor. Coral 66 does not possess these lexical constructs; formatting is largely free-form, and newlines do not require escaping. Maintaining infrastructure to strip backslash-newlines or trigraphs adds superfluous complexity.
