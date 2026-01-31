# Coral 66 Backend Transition Plan

This document outlines the step-by-step plan to transition the existing LLVM Backend (`src/cld/LLVMBackend`) to support the Coral 66 language. This follows the frontend transition plan and adapts the code generation logic to match the new AST and Coral 66 semantics.

## Phase 1: Type System Adaptation

**Goal:** Update the backend type mapping and identity logic to support Coral 66 types (`FIXED`, `TABLE`, `OVERLAY`).

**Affected Files:**
- `src/cld/LLVMBackend/TypeIdentity.hpp`
- `src/cld/LLVMBackend/CodeGeneration.hpp`
- `src/cld/LLVMBackend/CodeGenerator.hpp`

**Step 1.1: Update Type Identity Hashing**
In `TypeIdentity.hpp`, update `TypeHasher` and `TypeEqual` specializations:
1.  **Remove C Types:** Remove `StructType`, `UnionType` (unless reused for `TABLE`), `EnumType`.
2.  **Add Coral Types:**
    -   `Semantics::TableType`: Hash based on layout signature.
    -   `Semantics::FixedType`: Hash based on total bits and fractional bits.
    -   `Semantics::OverlayType`: Treat similarly to `Union` or `Pointer`.

**Step 1.2: Type Mapping (LLVM Types)**
In `CodeGenerator.hpp` (method `visit(const Semantics::Type& type)`):
1.  **`FIXED` Type:** Map to standard `llvm::IntegerType` of sufficient width. Scaling operations will be explicit in the IR (shifted).
2.  **`TABLE` Type:** Map to `llvm::StructType` with `llvm::Packed` attribute if required to match the bit-packed nature, or a single large `Integer`/`Array` if the table is purely for bit-field definitions.
3.  **`BITS` Type:** Map to `llvm::IntegerType` (often `i32` or machine word).

## Phase 2: Program Structure & Entry Point

**Goal:** Adapt the entry point to generate code for a Coral `Program` or `Segment` instead of a C `TranslationUnit`.

**Affected Files:**
- `src/cld/LLVMBackend/Codegen.cpp`
- `src/cld/LLVMBackend/CodeGeneration.hpp`

**Step 2.1: Entry Point Modification**
1.  Update `generateLLVM(..., const Semantics::Program& program, ...)` signature if the root AST node type changes.
2.  **Global Initialization:** Coral programs may have global data with `PRESET` values. Ensure `visit(const Semantics::Program&)` iterates over global declarations first.
3.  **Program Block:** Generate the `main` entry point for the `PROGRAM` block. Coral segments might compile to functions/modules.

## Phase 3: Statement Generation

**Goal:** Implement visitors for Coral 66 control flow constructs.

**Affected Files:**
- `src/cld/LLVMBackend/CodeGeneratorStatements.cpp`
- `src/cld/LLVMBackend/CodeGenerator.hpp`

**Step 3.1: Control Flow**
1.  **`ForStatement`:** Rewrite `visit(const Semantics::ForStatement&)` to handle `STEP` and `UNTIL`.
    -   Generate loop header with `start` assignment.
    -   Generate condition check `variable < end` (or `>` if step is negative).
    -   Generate latch block adding `step` to `variable`.
2.  **`GotoStatement`:** Adapt to use Coral label resolution. (Likely reuses existing `m_labels` map).
3.  **`AnswerStatement`:** Implement `visit(const Semantics::AnswerStatement&)`:
    -   Equivalent to `return expression` in C.
    -   Check `m_currentFunction` return type.

**Step 3.2: Inline Code**
1.  **`CodeStatement`:** Implement `visit(const Semantics::CodeStatement&)` to support inline assembly or direct machine code injection (using `llvm::InlineAsm`).

## Phase 4: Expression Generation

**Goal:** Implement evaluation logic for Coral expressions, specifically bit manipulation.

**Affected Files:**
- `src/cld/LLVMBackend/CodeGeneratorExpressions.cpp`
- `src/cld/LLVMBackend/CodeGenerator.hpp`

**Step 4.1: Part Word Access (Tables/BITS)**
1.  **Bit Field Access:** Implement logic to load/store from `TABLE` fields.
    -   **Load:** Load the containing word -> Shift Right `(word >> offset)` -> Mask `(val & mask)`.
    -   **Store:** Load word -> Mask out field `(word & ~mask)` -> OR in new value `((val & val_mask) << offset)`.
2.  **`BITS` Pseudo-operator:** Implement similarly to table fields but for arbitrary variables.

**Step 4.2: Fixed Point Arithmetic**
1.  **Scaling:** If Semantics does not lower `FIXED` arithmetic to integer ops, the backend must inject shifts.
    -   `ADD/SUB`: Align binary points (shift smaller scale).
    -   `MUL`: Multiply integers -> Result has sum of fractional bits -> Shift right to normalize.
    -   `DIV`: Shift numerator left -> Divide -> Result has difference of fractional bits.

## Phase 5: ABI & Function Calls

**Goal:** Ensure parameter passing conventions match Coral 66 rules (`VALUE` vs `LOCATION`).

**Affected Files:**
- `src/cld/LLVMBackend/ABIImplementation.hpp`
- `src/cld/LLVMBackend/X64ABI.cpp`
- `src/cld/LLVMBackend/CodeGeneratorExpressions.cpp` (Call handling)

**Step 5.1: Parameter Passing**
1.  **`VALUE`:** Pass by value (standard C behavior).
2.  **`LOCATION`:** Pass by reference (pointer). The backend must ensure `llvm::Value*` passed is a pointer to the storage, not the loaded value.

**Step 5.2: Recursion**
Coral 66 officially requires `RECURSIVE` keyword for recursion, though many compilers allow it by default. Ensure the backend doesn't rely on `nocapture` or `norecurse` attributes unless verified.

## Summary of Backend Changes

| Feature | Old Implementation (C99) | New Implementation (Coral 66) |
| :--- | :--- | :--- |
| **Types** | `int`, `float`, `struct` | `INTEGER` (i32), `FLOATING` (float), `TABLE` (packed struct) |
| **Structs** | `LLVM StructType` (padded) | `LLVM StructType` (packed) + Bitwise Shift/Mask Logic |
| **Fixed Point**| N/A (or float) | Integer ops + Bit Shifts |
| **Loops** | `for(init; cond; inc)` | `FOR v := start STEP s UNTIL end` |
| **Return** | `return` | `ANSWER` |
| **ABI** | By Value/Pointer (Explicit) | `VALUE` (ByVal), `LOCATION` (ByRef) |
