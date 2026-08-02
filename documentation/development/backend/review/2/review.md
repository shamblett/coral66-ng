# Backend Code Spec Compliance Review

## Overview
This document reviews the `coral66-ng` backend implementation (`lib/backend/`) against the official Coral 66 specification, assessing its adherence and highlighting missing language features as mandated by the language definition and the project's strategic roadmap (`strategy.md`).

## Implemented Features (Strong Compliance)
The backend achieves robust coverage for the majority of the language's core algorithmic and structural features (Phases 1 through 6):
* **Data Types & Operations:** Broad support for Integer, Floating point, and `FIXED` point arithmetic (with correct scaling and fractional bit handling). `BITS` representations are mapped efficiently to 32-bit integers.
* **Control Flow:** `IF`, `FOR` loops (both `STEP-UNTIL` and `WHILE` variants), and `GOTO` statements are fully supported. 
* **Switches:** `SWITCH` jumps successfully dynamically route runtime control flows via generated LLVM Switch instructions.
* **Complex Data Structures:** `ARRAY` and `TABLE` models are implemented. Critically, part-word table elements—a core requirement of the Coral 66 specification for embedded contexts—are correctly generated using bitmasking, bit-shifting, and integer extraction operations.
* **Procedures:** Calling conventions, typed/untyped routines, and `LOCATION` parameter references (pointers) align with the Coral 66 ABI.
* **Inline Assembly:** Support for target-specific `CODE` statements translates cleanly into LLVM inline assembly generation.

## Implementation Deficiencies
The backend currently has several critical omissions, primarily centered around linkage, external communication, and explicit layout (Phase 7 of the strategy roadmap):

* **Communicators (Missing):** The `CodeGenerator::generate` entry point processes the main program `Block` but completely ignores `m_tree.getCommunicator()`. As a result, all Coral 66 Communicators are unimplemented:
  * **`COMMON` communicators:** Cannot share global variables and data sections between independent compilation segments.
  * **`LIBRARY` and `EXTERNAL` communicators:** Cannot link to external procedures or external variables.
  * **`ABSOLUTE` communicators:** Cannot pin variables to specific hardware memory addresses.
* **`PlaceSpecification` (Ignored):** The AST node `syntax::PlaceSpecification` is unhandled. Without this, specifying hardware locations (like I/O ports or exact memory mappings) or communicating labels externally is unsupported.
* **`ProcedureSpecification` (Ignored):** Forward declarations and external specifications mapping to `syntax::ProcedureSpecification` are structurally bypassed. While internal LLVM IR generation successfully relies on a multi-pass approach to resolve internal globals, the lack of explicit `ProcedureSpecification` translation impacts strictly-compliant external linkage semantics.

## Overall Compliance Level: MEDIUM

### Conclusion
The backend is highly functional for standalone algorithmic segments, achieving excellent compliance with structural logic, memory layouts (like `OVERLAY` and `TABLE`), and arithmetic rules. However, because Coral 66 is fundamentally designed for embedded and systems programming where cross-segment linkage and precise hardware mapping are paramount, the complete absence of Communicators and Place Specifications represents a significant compliance deficiency. Implementing the linkage semantics (Phase 7) will bring the backend to a HIGH compliance level.