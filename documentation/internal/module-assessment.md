# Module Assessment: C++23 Modules for `coral66-ng`

The following is an assessment of modularising the coral66-ng codebase. It uses the lexer subsystem as an example but 
the results apply globally.

Based on an examination of the `lexer_support.hpp` and `lexer_support.cpp` files in `lib/frontend/lexer/`, as well as the project's build setup (Bazel) and tooling context (VSCode, ccache), here are the pros and cons of converting `lexer_support` into a C++23 module.

### Pros

1. **Strict Encapsulation & Macro Isolation**
   By defining a module (e.g., `export module coral66.lexer_support;`), you can explicitly `export` only the interfaces needed by the lexer (such as `Context`, `StateMachine`, `isKeyword`, etc.). Any internal helper functions (like `findUTF8StartByte`, `getNumUTF8ForUTF32`) or macros from included headers (like LLVM headers or standard libraries) are completely hidden and will not leak into `lexer.cpp`.
2. **Cleaner Interface Design**
   Modules enforce a strict separation of what is visible to the consumer versus what is internal. This forces a well-defined API boundary at the language level rather than relying on `#pragma once`, namespaces, or relying on developers not to use internal helpers.
3. **Modernization & Standardization**
   It embraces C++23 features, moving away from legacy text-based `#include` directives. This modernizes the codebase and can set a precedent for modularizing other components in the project.

### Cons

1. **Minimal Compile-Time Benefit (in this specific case)**
   A primary benefit of modules is that the compiler parses the interface once into a Built Module Interface (BMI), which is quickly loaded by multiple importers. However, `lexer_support.hpp` is currently only included by a single file (`lexer.cpp`). Therefore, you won't see any compilation speedup; in fact, the overhead of generating and loading the BMI might make the build slightly slower.
2. **Build System (Bazel) Complexity**
   The project uses Bazel (`rules_cc`), which currently has limited and experimental support for C++20/C++23 modules. Converting this code to a module would likely require complex workarounds, custom Starlark rules, or bleeding-edge Bazel features. Additionally, the project uses `ccache` (as seen in `.bazelrc`), which historically struggles to correctly cache C++ modules.
3. **Tooling and IDE Degradation**
   The project uses VSCode (evidenced by the `.vscode` directory). Language servers like `clangd`—which power autocomplete, linting, and go-to-definition—still have incomplete or buggy support for C++ modules. Moving to modules could lead to a significantly degraded developer experience (e.g., false-positive errors, broken intellisense).
4. **Interoperability with Legacy Includes**
   The `lexer_support` code relies heavily on non-modular headers (e.g., `<llvm/Support/Format.h>`, `<charconv>`, and internal project headers like `lib/frontend/common/messages/errors.hpp`). These must be placed in the global module fragment (`module; #include ...`). If any of these headers are not "module-friendly", it can lead to fragile code and difficult-to-diagnose One Definition Rule (ODR) violations.
5. **Refactoring Overhead**
   The effort required to restructure the `.hpp` and `.cpp` files into a module interface (e.g., `lexer_support.cppm`), meticulously define the `export` blocks, manage the global module fragment, and update the `BUILD` files likely outweighs the architectural benefits for a localized support file.

### Summary
While making `lexer_support` a C++23 module would improve encapsulation and modernize the code, **the cons heavily outweigh the pros in this specific scenario.** Because it is only included in one place, you gain no build speed improvements, and you invite significant friction with Bazel, `ccache`, and IDE tooling.
