# Coral Module Interface (CMI) Specification

This document defines the layout, data sourcing, and behavioral expectations for Coral Module Interface (`.cmi`) files. The `.cmi` file serves as the bridge between pre-compiled Coral 66 libraries and the consuming programs, encapsulating a library's public API.

## 1. Data Source and Generation

The `.cmi` file is an artifact generated during the **Semantic Analysis** phase of library compilation. 

1. **Symbol Table Extraction:** When `coral66-ng` compiles a file designated as a library, the Semantic Analyzer constructs a full Symbol Table for the entire source file.
2. **Filtering via `COMMON`:** Once parsing and semantic checks are complete, the CMI Generator filters the Symbol Table. It extracts *only* the symbols that were declared or specified within the `COMMON` communicator.
3. **Serialization:** These extracted semantic objects (Data Types, Procedure Signatures, Labels, etc.) are mapped into a standardized JSON schema and written to disk as the `.cmi` file alongside the `.o` (object) file.

Internal library variables, macros, and unexported procedures remain in the semantic analyzer's local scope and are explicitly excluded from the `.cmi` file.

## 2. CMI File Layout

The `.cmi` file is a UTF-8 encoded JSON document. It is designed to be human-readable, easily parsed by external tooling (like IDEs or dependency trackers), and quickly deserialized by the `coral66-ng` compiler.

The JSON schema consists of two primary sections: `header` and `symbols`.

### 2.1 Header Block

The header provides metadata necessary for the compiler to validate compatibility and resolve namespaces.

*   **`library_name`** (`String`): The canonical name of the library (typically matching the `SEGMENT` name).
*   **`cmi_version`** (`String`): A semantic version number indicating the structure version of the `.cmi` format. This allows future versions of `coral66-ng` to parse older `.cmi` files by detecting the version and applying backward compatibility transformations if the schema evolves.

### 2.2 Symbols Block

The `symbols` array contains the serialized representations of exported symbols. Each object in this array contains:

*   **`name`** (`String`): The identifier of the symbol.
*   **`symbol_category`** (`String`): The category of the symbol (e.g., `"data"`, `"procedure"`, `"label"`, `"switch"`).
*   *Category-specific fields*:
    *   **Data**: Requires `data_type` (e.g., `"REAL"`, `"INTEGER"`), and array dimension bounds if applicable.
    *   **Procedures**: Requires `return_type` (e.g., `"REAL"`, `"NONE"` for untyped procedures) and a `parameters` array detailing parameter names, data types, and passing conventions (`"VALUE"`, `"LOCATION"`).

## 3. Examples

The following examples demonstrate a simple library creation, the resulting `.cmi` file, and a consuming program.

### 3.1 The Library Source (`myMaths.cor`)

Notice that we declare `myMaths_Sin` as a `REAL PROCEDURE` to allow it to return a value. 

```coral66
SEGMENT myMaths;
COMMENT This is the library source ;

COMMON (
    REAL PROCEDURE myMaths_Sin(VALUE REAL);
    REAL pi;
)

REAL pi = 3.14159;

REAL PROCEDURE myMaths_Sin(VALUE REAL x);
BEGIN
    COMMENT Internal taylor series implementation ... ;
    ANSWER x;
END;
```

### 3.2 The Generated CMI File (`myMaths.cmi`)

When the above library is compiled (`coral66 --emit-library myMaths.cor -o myMaths`), the compiler generates `myMaths.o` and the following `myMaths.cmi` JSON file:

```json
{
  "header": {
    "library_name": "myMaths",
    "cmi_version": "1.0.0"
  },
  "symbols": [
    {
      "name": "myMaths_Sin",
      "symbol_category": "procedure",
      "return_type": "REAL",
      "parameters": [
        {
          "name": "unnamed_param_0",
          "pass_by": "VALUE",
          "data_type": "REAL"
        }
      ]
    },
    {
      "name": "pi",
      "symbol_category": "data",
      "data_type": "REAL",
      "is_array": false
    }
  ]
}
```
*(Note: Because the parameter in the `COMMON` block specification `(VALUE REAL)` is unnamed in Coral 66 specifications, the generator assigns a placeholder name like `unnamed_param_0`. The actual parameter name `x` is local to the implementation.)*

### 3.3 The Consuming Program (`main.cor`)

When the main program is compiled, it uses the `LIBRARY` communicator. The compiler parses the `.cmi` JSON file above and injects `myMaths_Sin` and `pi` into the main program's semantic symbol table as `External` symbols.

```coral66
PROGRAM main;

LIBRARY myMaths;

BEGIN
    REAL result;
    result := myMaths_Sin(pi);
END;
```

## 4. Separation of Interface and Implementation (Linkage)

A common question when viewing a `.cmi` file is: *"If a preset variable like `REAL pi = 3.14159;` is exported, why isn't the value `3.14159` in the JSON?"*

The `.cmi` file serves strictly as an **interface definition**—much like a C/C++ header file containing `extern float pi;`. It provides the consuming compiler with enough information to enforce type safety and generate valid LLVM Intermediate Representation (IR). 

1.  **The Compiler's Role:** When `main.cor` is compiled, the compiler sees `pi` in the `.cmi` file and emits an LLVM instruction declaring `pi` as an external global variable (e.g., `@pi = external global float`). It does not need to know the value to compile the code.
2.  **The Object File's Role:** When the library `myMaths.cor` was originally compiled, the actual value `3.14159` was embedded into the initialized data segment of the generated object file (`myMaths.o`).
3.  **The Linker's Role:** During the final step of building the executable, the system linker (invoked by `action_executor.cpp`) takes `main.o` and `myMaths.o` and wires them together. It resolves the external reference in `main` to the actual memory address inside `myMaths` where `3.14159` is stored.

The same principle applies to procedure bodies. The `.cmi` file contains the signature so the compiler can verify argument types, but the actual executable machine code lives in the `.o` file.

## 5. Deserialization and Consumption

When a consuming program encounters `LIBRARY <Name>;`:

1.  **Resolution:** The compiler driver searches the library paths (e.g., provided via `-L`) for `<Name>.cmi`.
2.  **Validation:** The parser reads the JSON, checking `header.cmi_version` against the compiler's supported versions, and verifying `header.library_name` matches the requested library.
3.  **Injection:** Iterating through the `symbols` array, the compiler instantiates in-memory semantic objects (e.g., `ProcedureSymbol`, `VariableSymbol`) and inserts them into the global scope of the current translation unit. These symbols are flagged with an `External` linkage attribute so the code generator emits LLVM `declare` statements instead of definitions.