# Control Flow Pass & Overall Semantics Review 2

This document provides a review of the Control Flow Pass, highlighting any deficiencies or omissions to allow full compliance with the Coral 66 specification. It also identifies gaps in the existing Symbol Resolution and Type Checking passes now that the Control Flow Pass has been implemented.

## 1. Control Flow Pass Status
The newly implemented Control Flow Pass successfully handles:
* **Array & Table Bounds Evaluation**: Emits `INVALID_ARRAY_BOUNDS` and `TABLE_INVALID_DIMENSIONS` by evaluating constant dimension expressions.
* **Array Index Checking**: Enforces `ARRAY_INDEX_OUT_OF_BOUNDS` by statically evaluating indices where possible against evaluated array dimensions.
* **Part-Word & Fixed Scaling Evaluation**: Enforces bounds for `INVALID_BIT_RANGE` and `INVALID_SCALE_FOR_FIXED` by validating numeric parameters at compile time.
* **Switch Label Validations**: Enforces `SWITCH_INDEX_OUT_OF_BOUNDS` based on constant index evaluations (1-based Coral 66 rule).
* **Block Jumps Check**: It was identified during this review that `ILLEGAL_BLOCK_JUMP` was redundant. In Coral 66, labels scoped to inner blocks are invisible to outer blocks. Thus, `SymbolResolutionPass` effectively enforces block jump legality via pre-scanning scopes, correctly emitting `UNDECLARED_IDENTIFIER_N` when attempting to jump into a block. The redundant explicit block jump check in `ControlFlowPass` has been removed.
* **Answer Statement Contexts**: Effectively ensures Typed Procedures contain an Answer statement (`MISSING_ANSWER_STATEMENT`). Context validations for Untyped procedures and global answer statements are cleanly handled by the parser.

**Status:** The Control Flow Pass logic is structurally sound, and all current control flow rules correctly align with the Official Definition.

---

## 2. Identified Deficiencies in Symbol Resolution and Type Checking Passes

With the implementation of the Control Flow Pass and its constant evaluator (`evaluateConstantInteger`), the following deficiencies in the older semantics passes can now be addressed:

### A. Fixed-Point Type Resolution (`SymbolResolutionPass`)
* **Issue:** In `SymbolResolutionPass::createTypeFromToken`, when resolving the `FIXED` keyword, the system currently hardcodes a default scale of `16, 8`:
  ```cpp
  if (type == lexer::TokenType::FixedKeyword)
      return {(FixedType{16, 8})};
  ```
  The parser parses the `TotalBits` and `FractionBits` expressions, and `ControlFlowPass` validates them inside `TypeChangeExpression`, but the base `VariableDeclaration` scale parameters are silently discarded during type creation.
* **Action Required:** Update `VariableDeclaration`, `ArrayDeclaration`, and related parsing structures to store the fixed scale AST nodes. `SymbolResolutionPass` must then utilize `evaluateConstantInteger` to properly construct the `FixedType` representation to enable accurate downstream type operations.

### B. Recursive Procedure Validation (`TypeCheckingPass` / `ControlFlowPass`)
* **Issue:** The Coral 66 Spec (Section 3.5) explicitly states: "If a procedure is defined in a manner which directly or indirectly calls on itself, that procedure is said to be recursive and must be explicitly declared as such."
  The parser correctly parses the `RECURSIVE` keyword flag (`isRecursive`), but no semantic pass checks if a function call expression invokes the current procedure and fails if the `RECURSIVE` keyword is omitted.
* **Action Required:** `TypeCheckingPass` or `ControlFlowPass` should track the `m_currentProcedure` and check all `ProcedureCallExpression` and `FunctionCallExpression` targets. If the target matches the current procedure, the pass must verify `m_currentProcedure->isRecursive()`. If false, it must emit a new diagnostic `RECURSIVE_CALL_ON_NON_RECURSIVE_PROCEDURE`.

## Next Steps
The next iteration of semantics work should resolve these 2 specific deficiencies (Fixed-Point Scale Retention and Recursive Call Validation) across the AST, Parser, Symbol Resolution Pass, and Type Checking Pass.