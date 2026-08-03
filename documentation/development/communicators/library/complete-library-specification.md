# Complete LIBRARY Communicator Specification

This document details the implementation strategy for the `LIBRARY` communicator in `coral66-ng`. The goal is to provide a modern, encapsulated, and performant module system that remains faithful to the spirit of the Coral 66 specification, while functioning as a standalone compiler requiring no external build systems.

## 1. Architectural Overview: The CMI Approach

Rather than relying on textual inclusion or complex background compilation within the compiler driver, `coral66-ng` utilizes a **Pre-compiled Interface** strategy. 

When a library is compiled, the compiler produces two artifacts:
1. An object file (`.o` or `.a`).
2. A Coral Module Interface (`.cmi`) file. This is a lightweight, serialized binary representation of the symbols explicitly exported by the library.

When an invoking program declares `LIBRARY myMaths`, the compiler does not re-parse the source of `myMaths.cor`. Instead, it locates and deserializes `myMaths.cmi`, injecting the defined symbols into the semantic analyzer's symbol table as `External`.

### 1.1 Encapsulation via `COMMON`
The library code leverages the `COMMON` communicator to explicitly define its public API. Any declarations placed inside the `COMMON` communicator of a library source file are serialized into the `.cmi` file and made available to the invoking program. 

Conversely, any definitions in the library source that are *not* placed in the `COMMON` communicator are local to the library and remain strictly hidden from the invoking program.

### 1.2 Macro Hygiene
Because the importing program only reads the binary `.cmi` file, `DEFINE` macros used internally by the library are completely isolated. They cannot leak into or pollute the importing program's macro namespace, preventing naming collisions.

## 2. Workflows and Command Line Usage

The compiler is designed to be standalone, allowing standard users to both produce and consume libraries effortlessly.

### 2.1 The Library Creator Workflow

Users can create libraries using a dedicated compiler flag (e.g., `--emit-library` or simply using `-c` to denote a compile-only step). 

**Source file (`myMaths.cor`):**
```coral66
COMMENT This is the library source ;
COMMON (
    PROCEDURE myMaths_Sin(VALUE REAL);
    REAL pi;
)

REAL pi = 3.14159;

PROCEDURE myMaths_Sin(VALUE REAL x);
BEGIN
    COMMENT Internal implementation ;
    ...
END;
```

**Command:**
```bash
coral66 --emit-library myMaths.cor -o myMaths
```

**Compiler Action:**
1. Parses and semantically checks `myMaths.cor`.
2. Extracts symbols defined in the `COMMON` block.
3. Generates `myMaths.cmi` containing the serialized interface.
4. Generates `myMaths.o` containing the compiled LLVM IR/object code.

*(Note: The exact flag name, e.g., `--emit-library` vs `-c`, will be finalized in the command-line arguments implementation.)*

### 2.2 The Library Consumer Workflow

The user compiles their main program and specifies the search path for libraries using the `-L` flag.

**Source file (`main.cor`):**
```coral66
PROGRAM main;
LIBRARY myMaths;
BEGIN
    REAL result;
    result := myMaths_Sin(pi);
END;
```

**Command:**
```bash
coral66 main.cor -L /path/to/mylibs -o my_app
```

**Compiler Action:**
1. **Semantic Phase:** Upon encountering `LIBRARY myMaths`, the semantic analyzer searches the paths provided by `-L` (and default system paths) for `myMaths.cmi`.
2. **Symbol Injection:** It deserializes `myMaths.cmi` and registers `myMaths_Sin` and `pi` into the current file's Symbol Table, tagged as `External`. The parser does not need to know the implementation details.
3. **Code Generation:** The compiler generates `main.o`, emitting LLVM IR declarations (`declare ...`) for the external library symbols.
4. **Linking Phase:** The `action_executor.cpp` invokes the system linker, passing `main.o` and appending the matched library object (`/path/to/mylibs/myMaths.o` or utilizing standard linker `-l` flags if archived).

## 3. Advantages of this Strategy

1. **Standalone Capability:** Users do not need CMake, Make, or Bazel to create or share libraries. The `coral66-ng` executable handles both interface generation and linking integration.
2. **Driver Simplicity:** The compiler driver (`action_executor.cpp`) remains clean. It does not need to orchestrate multi-threaded background compilation, check source timestamps, or manage cyclic dependencies.
3. **Compilation Speed:** Reading a binary `.cmi` file is orders of magnitude faster than repeatedly re-preprocessing and parsing Coral 66 source text.
4. **Build System Compatibility:** If a user eventually transitions to a complex build system (like Bazel), the `.cmi` and `.o` artifacts map perfectly to standard DAG dependency tracking.
5. **Memory Efficiency:** The semantic phase does not have to maintain multiple heavy ASTs in memory simultaneously; it only holds the required semantic symbols.
