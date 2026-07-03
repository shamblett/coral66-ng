# Coral 66 Lexer Compliance Review

This document provides a compliance review of the Coral 66 lexer implementation (`lib/frontend/lexer`) against the provided Coral 66 specification documents (`documentation/specification/coral66-grammar.txt` and `documentation/specification/Official Definition of Coral 66.txt`).

## 1. Mandated Functionality Not Supported by the Lexer (Deficiencies)

These are areas where the lexer diverges from or fails to implement requirements explicitly stated in the Coral 66 Official Definition.

*   **Global Space Insensitivity (Layout Characters):**
    *   *Specification:* The Official Definition mandates that layout characters (such as spaces) are completely ignored by the compiler outside of strings. This implies spaces can legally appear *inside* numbers (e.g., `1 0 . 5`), identifiers, and keywords without breaking tokenization.
    *   *Lexer Implementation:* The lexer relies on whitespace as standard token delimiters. It will fragment `1 0 . 5` into multiple separate number tokens. *(Note: The `variations.txt` document explicitly overrides this for identifiers stating "Identifiers cnnot comprise of spaces", but does not explicitly exempt numbers or keywords).*
*   **Multiline Strings:**
    *   *Specification:* Section 10.4 states that a string is "any succession of characters (printing or layout) enclosed in quotation marks". Because newlines and carriage returns are layout characters, strings are theoretically allowed to span multiple lines.
    *   *Lexer Implementation:* The lexer strictly forbids multiline strings. Encountering a `\n` or `\r` while in the `StringLiteral` state immediately triggers an `UNTERMINATED_STRING_LITERAL` error.
*   **Nested Strings via Distinguishable Quotes:**
    *   *Specification:* Section 10.4 notes that "Assuming that the hardware representations of the opening and closing quote symbols are distinguishable, occur[rences] of strings can be nested."
    *   *Lexer Implementation:* The lexer assumes standard ASCII `"` for both opening and closing quotes. It supports internal quotes by escaping them sequentially (e.g., `""`), but does not implement state tracking for nested strings using distinguishable hardware opening/closing quotes.
*   **The `10` Exponent Symbol:**
    *   *Specification:* The grammar explicitly mandates `10` as the exponent symbol for REAL numbers (e.g., `Digitlist 10 Signedinteger`).
    *   *Lexer Implementation:* The lexer tokenizes `10` strictly as an integer literal. It does not group sequences like `2.5 10 3` into a single floating-point token. Instead, exponents are handled using the modern transliterated prime symbol (`'`) as specified in the local `real.txt` documentation.

## 2. Extra Functionality Implemented Not Mandated by Specifications (Extensions)

These are areas where the lexer is more permissive or introduces modernizations not defined in the strict original Coral 66 grammar.

*   **Leading and Trailing Decimal Points:**
    *   *Lexer Implementation:* The lexer successfully tokenizes `.5` and `3.` as valid `Floating` point literals.
    *   *Specification:* The grammar dictates that a Real number decimal must have digits on *both* sides: `Real ::= Digitlist . Digitlist`. Values like `.5` and `3.` are technically invalid according to the strict grammar and should require a leading/trailing zero (e.g., `0.5`, `3.0`).
*   **Stropped Keywords:**
    *   *Lexer Implementation:* The lexer includes a dedicated `StroppedKeyword` state (and `Prime` state) to support quoted keywords like `'BEGIN'`.
    *   *Specification:* The Official Definition states that words in uppercase are treated as keywords and lowercase as identifiers. It does not mandate "stropping" (enclosing keywords in quotes) as part of the core language syntax. This is an implementation-specific, Algol-inspired extension designed to make parsing easier on modern ASCII systems without relying purely on casing.
*   **Reserved Uppercase Enforcement:**
    *   *Lexer Implementation:* The lexer checks for keywords strictly against uppercase strings (e.g., `BEGIN`). Any other case variation (e.g., `Begin`, `begin`) is cleanly tokenized as an `Identifier`.
    *   *Specification:* The spec mentions uppercase is used for symbols (keywords) and lowercase is "reserved for use in identifiers". By implementing this as strict case sensitivity, the lexer seamlessly accommodates the 1970s typographic specification inside a functional modern ASCII environment, effectively allowing both uppercase reserved words and mixed/lowercase variables side by side.
