Analysis of Name Mangling for Coral 66 Libraries: Synthesizing analysis on the advantages, impact, and overall worth of implementing name mangling in the semantics phase.


✦ Based on an analysis of the codebase (specifically the SymbolTable, CMIProcessor, and the backend CodeGenerator), here is a breakdown of how implementing name mangling would affect the
coral66-ng compiler and its library ecosystem.

1. What advantage would there be to implement name mangling?

Currently, because the raw source identifier is used as both the semantic symbol table key and the LLVM backend symbol name, library developers are constrained by the flat C namespace.

Implementing a mangling scheme (e.g., prefixing all COMMON and LIBRARY exports with _c66_) provides several strong advantages:
* Linker Isolation: It entirely eliminates collisions between Coral 66 procedures and the C standard library. A Coral 66 module could safely export a COMMON procedure named time or read
  without the linker confusing it for libc's time() or read().
* API Ergonomics: Library writers would no longer be forced to invent contorted, capitalized aliases (like Time, Clock, or timep) to sidestep collisions. Consumers of the library could
  use the natural, expected names in their Coral 66 code.
* Decoupled Namespaces (if applied to the Semantic Symbol Table): If the semantic SymbolTable keys incorporate the communicator type (e.g., distinguishing between EXTERNAL time and
  COMMON time), a single module could internally declare an EXTERNAL binding to a C function and a COMMON wrapper using the exact same base identifier.

2. What impact would name mangling have on the code base?

Introducing name mangling touches several architectural boundaries:

* Semantics (lib/frontend/semantics/): The SymbolTable would need to hash and store mangled names (or composite keys) rather than just raw std::string_view source names.
* Module Interfaces (cmi_processor.cpp): The CMI serialization logic would need to store either the mangled name, or enough metadata (like the communicator context) so the importer can
  deterministically reconstruct the correct linkage name.
* Code Generation (lib/backend/): When calling llvm::Module::getOrInsertFunction or creating GlobalVariables, the backend must use the mangled name. Crucially, EXTERNAL declarations must
  bypass mangling to preserve their ability to link against raw C ABI symbols.
* Diagnostics & Tooling: Error reporting in semantic_context and debugging tools (like astdump) would require a demangling step so that developers see time rather than _c66_common_time
  in their compiler errors.
* Tests & Libraries: Any LLVM IR golden tests or CMI structural tests in test/ will break and require updates. Additionally, existing standard libraries (support/library/*.cor) and their
  wrappers (support/external/*.c) would need a refactor to take advantage of the new naming freedom.

3. Would it be worth the benefit?

Yes. While it introduces a moderate amount of refactoring work across the compiler, a simple mangling scheme is highly worthwhile for the long-term health of the language.

Without mangling, every future library module added to coral66-ng will suffer from the same C-namespace friction, artificially shifting the burden of collision avoidance onto the users
and library authors. By isolating the Coral 66 ABI via a straightforward prefix strategy (avoiding complex C++-style type encoding, which is unnecessary for Coral 66), you dramatically
improve the developer experience and the safety of the linking phase for a relatively low architectural cost.
