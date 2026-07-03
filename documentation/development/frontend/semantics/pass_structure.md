✦ I have successfully set up the file structure and skeletal framework necessary to support Scheme 1 (Multi-Pass Architecture).

Here is a summary of the additions:

1. symbol_table.hpp / .cpp
    * Provides a skeletal SymbolTable class that manages stacked/nested scopes to correctly resolve variables and procedures within their respective blocks.

2. semantic_context.hpp / .cpp
    * Acts as the central context payload passed to each semantic pass. It currently wraps the source_interface, diagnostics reporter, error flags, and an instance
      of the SymbolTable.

3. symbol_resolution_pass.hpp / .cpp
    * Skeletal structure for the first pass. This pass will walk the Syntax::Program, establish scopes in the symbol table, and resolve/bind identifiers to their
      declarations without worrying about expression type rules.

4. type_checking_pass.hpp / .cpp
    * Skeletal structure for the second pass. This pass expects the symbol table to be fully resolved. It will infer expression types, validate function calls
      (matching arguments), check type conversions, etc.

5. control_flow_pass.hpp / .cpp
    * Skeletal structure for the third pass. This checks for valid goto label boundaries, unreachable code, or broken loop semantics independent of types.

6. BUILD Updates
    * Updated the cc_library for lib/frontend/semantics/semantics to include all of the new C++ classes.
    * Note: I also fixed a missing #include <vector> in source_interface.hpp that was uncovered by building these fresh files.

The codebase builds seamlessly and passes all existing tests. This gives you a clear pathway to begin migrating SemanticAnalysis::visit(...) into decoupled visitors
living inside each specific pass!

