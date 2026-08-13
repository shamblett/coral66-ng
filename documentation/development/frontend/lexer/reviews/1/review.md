# Coral 66 Lexer Review

An analysis of the lexer code in `lib/frontend/lexer` against the Coral 66 language specification reveals that while it
has been significantly adapted from a C compiler codebase (e.g., handling Coral 66 keywords and operators like `<>` and
`:=`), it still retains several C-specific features that should be removed. Furthermore, critical Coral 66 lexical
features are currently missing.

## 1. Functionality Not Needed for Coral 66

The following features are remnants of the C/C++ lexer and should be removed as they are not part of the Coral 66
language specification:

* **Trigraph Processing:** C-style trigraphs (e.g., `??=`, `??/`, `??(`) are processed during the source normalization
  phase in `lexer.cpp`. Coral 66 does not use trigraphs, and retaining this could cause conflicts if these character
  sequences appear inside strings or comments.
* **Backslash-Newline Continuations:** The normalization phase processes `\` followed by a newline to merge lines. This
  is a C preprocessor concept and does not exist in Coral 66.
* **Universal Character Names:** The `MaybeUC` and `UniversalCharacter` states explicitly parse C++98/C99 style `\uXXXX`
  and `\UXXXXXXXX` Unicode escapes.
* **Wide Character and String Literals:** The `L` state processes `L"..."` and `L'...'` prefixes for wide strings, which
  are entirely absent from Coral 66.
* **C-Style Preprocessing Numbers:** The `PreprocessingNumber` state tracks C-style numbers (including `e`/`E` and `p`/
  `P` exponents). Coral 66 numeric structures differ significantly.
* **Extraneous Punctuators:** The lexer specifically tokenizes C operators that are unused in Coral 66, such as `{`
  (`TokenType::OpenBrace`) and `}` (`TokenType::CloseBrace`). Coral 66 strictly uses `BEGIN` and `END`.

## 2. Functionality Needed for Coral 66 (Missing)

The following features are required by the Coral 66 specification but are either missing or incorrectly implemented in
the current lexer:

* **Indivisible Comment Handling:**
  In Coral 66, comments must be treated as indivisible entities. Currently, the lexer attempts to tokenize the internal
  text of comments as standard code, which will lead to lexical errors if a comment contains unprintable characters,
  stray punctuation, or unmatched string quotes. The lexer needs to explicitly consume and ignore:
    * **Comment sentences:** Initiated by `COMMENT` and terminated by `;`. The lexer currently emits a `CommentKeyword`
      token but then erroneously tokenizes everything inside the comment up to the semicolon.
    * **Bracketed comments:** Any text enclosed in matching round brackets `( ... )` immediately following a semicolon.
      The lexer currently emits `OpenParentheses` and tokenizes the inner contents.
    * **END comments:** Any single identifier immediately following the `END` keyword is an annotation and should be
      consumed as part of the `END` token or explicitly ignored.
* **Coral 66 Numeric Constants:**
    * **Scientific Notation:** Coral 66 uses the `10` keyword/operator as the exponent marker instead of `E` or `e`
      (e.g., `12.3 10 -4` means $12.3 \times 10^{-4}$). The lexer currently defaults to standard C number parsing.
    * **Octal Literals:** Defined structurally as `OCTAL ( Octallist )` rather than using C-style `0` prefixes.
    * **Character Literals:** Defined structurally as `LITERAL ( character )` rather than using C-style `'c'` syntax.
* **Keyword Stropping (Dialect Support):**
  While the specification technically relies on typographical layout (uppercase vs lowercase), many real-world Coral 66
  codebases (such as the provided `../../../../code-exampes/examples/restore.cor`) enclose keywords in single quotes to
  distinguish them from identifiers (e.g., `'COMMENT'`, `'DEFINE'`). Currently, the lexer parses these as standard
  string literals.
* **String Literal Restrictions:**
  Coral 66 string literals are exclusively enclosed in double quotes (`"..."`). The current lexer allows strings and
  characters to be enclosed in single quotes (`'...'`), and it implicitly handles C-style escape sequences that are not
  specified in Coral 66.