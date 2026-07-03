# Coral 66 Parser Review

## Overview
This document provides a review of the Coral 66 parser located in `lib/frontend/parser/` for its adherence to the official Coral 66 specification and its readiness to produce an AST for semantic processing. 

## Conformance to Specification
Overall, the parser is highly robust, closely mapping to the Coral 66 formal grammar. It correctly handles the complex nuances of the language:

*   **Expressions:** Accurately parses standard numeric expressions, conditional expressions (`IF ... THEN ... ELSE`), and bitwise operations (`DIFFER`, `UNION`, `MASK`). It also properly parses word references, procedure calls, and type casting.
*   **Statements:** Comprehensively covers all major statement types:
    *   Conditional `IF...THEN...ELSE` statements.
    *   `GOTO` statements and label assignments.
    *   `ANSWER` statements, including contextual checks (e.g., throwing a diagnostic if `ANSWER` is placed inside an untyped procedure).
    *   `FOR` loops, accommodating Coral 66's specific elements (`STEP...UNTIL` and `WHILE` components) gracefully through the `Syntax::ForElement` structure.
    *   `CODE` blocks inline assembly constructs.
*   **Declarations:** Effectively parses standard declarations (Variables, Arrays, `TABLE` definitions, `OVERLAY` structures, and `PRESET` assignments).
*   **Communicators:** Parses the `COMMON` communicator properly (`parser_communicators.cpp`). As requested, the `LIBRARY`, `ABSOLUTE`, and `EXTERNAL` communicators are correctly omitted from the parsing phase for now.
*   **Keyword Validation:** The lexer defines the comprehensive list of Coral 66 keywords, and the parser successfully integrates them to guide its recursive descent logic.

## Areas of Non-Conformance
*   **Missing `RECURSIVE` Keyword Support:** The Coral 66 grammar dictates that a procedure declaration may be marked as recursive (e.g., `Answerspec RECURSIVE Procedureheading ; Statement`). While `TokenType::RecursiveKeyword` is properly defined by the lexer, `parser_procedures.cpp` does not consume or support this keyword during procedure parsing. Attempting to parse a `RECURSIVE PROCEDURE` will currently result in a syntax error.

## Areas for Improvement
*   **Switch Declaration Categorization:** The language specification treats `SWITCH` definitions as `Switchdec` within the broader declaration list. The current parser extracts them specifically as `Syntax::SwitchPlaceReference` and stores them in a separate `placeReferences` list within the `Syntax::Block` node. While semantically sound, unifying them under the general `Declaration` hierarchy may bring the AST closer to the specification's mental model.
*   **Error Recovery Enhancements:** The current error recovery inside `parseBlock` relies on aggressively consuming tokens until a `SemiColon` or `EndKeyword` is found. This sync strategy is adequate but might skip significant portions of valid code if a minor syntax error occurs in tightly nested blocks or long variable lists.

## Compliance Level Assessment
**Assessment: HIGH**

The codebase demonstrates a meticulous adherence to the Coral 66 specification. The AST objects defined in `lib/frontend/syntax/syntax.hpp` map cleanly to Coral 66 features, making it extremely well-prepared for downstream semantic processing. Once the `RECURSIVE` keyword logic is implemented, the syntactic compliance of the implemented features will be effectively complete.
