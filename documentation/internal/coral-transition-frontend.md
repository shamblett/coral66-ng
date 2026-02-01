# Coral 66 Transition Plan

This document outlines the step-by-step plan to transition the existing C99 compiler frontend (`src/cld/Frontend`) to a Coral 66 compiler, based on the Official Definition of Coral 66.

The transition preserves the underlying architecture (Lexer -> Preprocessor -> Parser -> AST -> Semantics) but replaces the language-specific implementations.

## Phase 1: Lexical Analysis (Lexer)

**Goal:** Replace C99 tokens and lexing rules with Coral 66 specifications.

**Affected Files:**
- `src/cld/Frontend/Compiler/LexerCommon.hpp`
- `src/cld/Frontend/Compiler/Lexer.cpp`
- `src/cld/Frontend/Compiler/Lexer.hpp`

**Step 1.1: Update Token Definitions**
In `LexerCommon.hpp`, modify `enum class TokenType`:
1.  **Remove C Keywords:** `Auto`, `Break`, `Case`, `Char`, `Const`, `Continue`, `Default`, `Do`, `Double`, `Else`, `Enum`, `Extern`, `Float`, `For`, `Goto`, `If`, `Inline`, `Int`, `Long`, `Register`, `Restrict`, `Return`, `Short`, `Signed`, `Sizeof`, `Static`, `Struct`, `Switch`, `Typedef`, `Union`, `Unsigned`, `Void`, `Volatile`, `While`, `Int128`, `Complex`, `Imaginary`.
2.  **Add Coral Keywords:**
    - Block/Structure: `BEGIN`, `END`, `PROCEDURE`, `PROGRAM` (or `SEGMENT`).
    - Control Flow: `IF`, `THEN`, `ELSE`, `FOR`, `STEP`, `UNTIL`, `DO`, `GOTO`, `ANSWER` (Return).
    - Data Types: `INTEGER`, `FLOATING` (Real), `FIXED`, `BITS`.
    - Data Structures: `ARRAY`, `TABLE`, `PRESET`, `OVERLAY`.
    - Storage/Scope: `VALUE`, `LOCATION`, `COMMON`.
    - Misc: `COMMENT`, `DEFINE`.
3.  **Update Operators:**
    - Ensure `.OR.`, `.AND.`, `.NOT.` are supported (if used in specific Coral dialect) or standard logical operators. Coral 66 often uses `OR`, `AND` keywords or specific symbols. Check `Lexer.cpp` for symbol mapping.
    - `<-` (Assignment) if applicable (standard Coral uses `:=`).
    - `NEQ` (Not Equal) etc.

**Step 1.2: Update Tokenization Logic**
In `Lexer.cpp`:
1.  **Update Keyword Map:** Update the hash map or trie used to identify keywords to match the new `TokenType` enum.
2.  **Numeric Literals:**
    - Support `OCTAL` literals (often standard in Coral).
    - Support `FIXED` point literals (e.g., `3.14` might need context or specific suffix if strictly typed, though usually handled in semantics).
3.  **Comments:**
    - Implement `COMMENT` keyword handling. Coral comments start with `COMMENT` and end with `;`. The lexer must consume everything between them and emit a single comment token or skip it.
4.  **Strings:**
    - Ensure string literal parsing matches Coral rules (often single quotes `'string'`).

## Phase 2: Preprocessor Adaptation

**Goal:** Adapt the preprocessor to support Coral 66 `DEFINE` macro facilities.

**Affected Files:**
- `src/cld/Frontend/Preprocessor/Preprocessor.cpp`
- `src/cld/Frontend/Preprocessor/Preprocessor.hpp`

**Step 2.1: Simplify Macro Engine**
Coral 66 `DEFINE` is a text substitution mechanism.
1.  **Macro Definition:** Parse `DEFINE name "string";`. This differs from C's `#define name value`.
2.  **Expansion:** The expansion logic can likely reuse the existing token stream substitution, but the *parsing* of the definition must change.
3.  **Directives:** Remove C directives `#include`, `#ifdef`, `#pragma`, etc., unless a specific Coral extension requires them. Keep a basic inclusion mechanism if "Library" support is needed.

## Phase 3: Abstract Syntax Tree (AST)

**Goal:** Redefine the AST nodes to represent Coral 66 constructs.

**Affected Files:**
- `src/cld/Frontend/Compiler/Syntax.hpp`
- `src/cld/Frontend/Compiler/Syntax.cpp`

**Step 3.1: Top Level Structure**
1.  **`Program` Node:** Replace `TranslationUnit`. A Coral program is a Block (typically).
2.  **`Block` Node:** A list of Declarations followed by a list of Statements (Commands), enclosed in `BEGIN` ... `END`.

**Step 3.2: Declarations**
1.  **Data Declaration:** Create nodes for `NumberDeclaration` (`INTEGER`, `FLOATING`, `FIXED`), `ArrayDeclaration`, and `TableDeclaration`.
    - `TableDeclaration` is complex: it defines a structure-like layout with strict word/bit packing.
2.  **Procedure Declaration:** Replace `FunctionDefinition`. Needs to capture `VALUE` and `LOCATION` parameters explicitly.
3.  **Overlay Declaration:** Create node for `OVERLAY` (aliasing memory).

**Step 3.3: Statements (Commands)**
1.  **`Assignment`:** `Variable := Expression`.
2.  **`GotoStatement`:** `GOTO Label`.
3.  **`IfStatement`:** `IF condition THEN statement [ELSE statement]`.
4.  **`ForStatement`:** `FOR variable := start STEP increment UNTIL end DO statement`.
5.  **`AnswerStatement`:** `ANSWER expression` (Coral's return).
6.  **`CodeStatement`:** For inline machine code `CODE BEGIN ... END`.

**Step 3.4: Expressions**
1.  **Update Precedence:** Ensure AST reflects Coral's operator precedence.
2.  **Part Word Access:** Add nodes for bit-field access (e.g., `BITS [start, width]`).

## Phase 4: Parser Implementation

**Goal:** Rewrite the Recursive Descent Parser for Coral 66 Grammar.

**Affected Files:**
- `src/cld/Frontend/Compiler/Parser.hpp`
- `src/cld/Frontend/Compiler/Parser.cpp`
- `src/cld/Frontend/Compiler/Parser*.cpp` (split files)

**Step 4.1: Structural Parsing**
1.  **`parseProgram`:** Entry point. Expects a `Block` or `Segment`.
2.  **`parseBlock`:**
    - Parse Declarations first (Coral often requires declarations before statements).
    - Parse Statements until `END`.

**Step 4.2: Declaration Parsing**
1.  **`parseDeclaration`:** Dispatch based on keywords (`INTEGER`, `REAL`, `TABLE`, etc.).
2.  **`parseTable`:** Implement specific logic to parse table presets and element layouts.

**Step 4.3: Statement Parsing**
1.  **`parseStatement`:** Switch on keywords (`IF`, `FOR`, `GOTO`). Default to `Assignment` or `ProcedureCall`.
2.  **`parseFor`:** Handle the specific `STEP` `UNTIL` syntax.

**Step 4.4: Expression Parsing**
1.  **`parseExpression`:** Update precedence climbing or recursive methods to match Coral operators.

## Phase 5: Semantic Analysis

**Goal:** Implement Type Checking and Scope Rules.

**Affected Files:**
- `src/cld/Frontend/Compiler/Semantics.hpp`
- `src/cld/Frontend/Compiler/Semantics.cpp`
- `src/cld/Frontend/Compiler/SemanticAnalysis*.cpp`

**Step 5.1: Symbol Table**
1.  **Scope Handling:** Adapt `Context` to handle block-scoped variables. Coral allows nested procedures.

**Step 5.2: Type Checking**
1.  **Numeric Types:** Implement strict checking for `FIXED` scaling (a key feature of Coral). `FIXED (total_bits, fractional_bits)`.
2.  **Table Access:** Validate field access against table definitions.
3.  **Parameter Passing:** Check `VALUE` vs `LOCATION` semantics in procedure calls.

**Step 5.3: Constant Evaluation**
1.  **Presets:** Evaluate `PRESET` values at compile time.

## Summary of Data Structure Changes

| Feature | Old Structure (C99) | New Structure (Coral 66) |
| :--- | :--- | :--- |
| **Root** | `TranslationUnit` | `Program` / `Block` |
| **Funcs** | `FunctionDefinition` | `ProcedureDeclaration` |
| **Types** | `int`, `float`, `struct` | `INTEGER`, `FLOATING`, `FIXED`, `TABLE` |
| **Loops** | `ForStatement` (C style) | `ForStatement` (Step/Until) |
| **Macros** | `#define` (Preprocessor) | `DEFINE` (Lexer/Preprocessor) |
| **Comments**| `//`, `/* */` | `COMMENT ... ;` |
