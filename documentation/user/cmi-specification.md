# Coral Module Interface (CMI) Specification

This document defines the layout, data sourcing, and behavioural expectations for Coral Module Interface (`.cmi`) files. The `.cmi` file serves as the bridge between pre-compiled Coral 66 libraries and the consuming programs, encapsulating a library's public API.

## 1. Data Source and Generation

The `.cmi` file is an artifact generated from the **Semantic Analysis** phase when compiling a Coral 66 library (`SEGMENT`).

1. **Symbol Table & Export Collection:** When `coral66` compiles a source file, the `SymbolResolutionPass` in the semantic analyzer constructs the Symbol Table. Any declarations inside a `COMMON` communicator have their communicator type set to `Common` and are recorded in the semantic context's `exportedSymbols` list.
2. **Filtering via `COMMON`:** Only symbols declared or specified within the `COMMON` communicator are added to `exportedSymbols`. Internal library variables, macros, and unexported procedures remain in the local module scope and are excluded from the `.cmi` file.
3. **Serialization:** When the source file is a `SEGMENT`, `ActionExecutor` invokes `CMIProcessor::generateCMIFile` after compilation. The exported semantic declarations are serialized into a formatted JSON document (using RapidJSON) and written to disk as `<output_stem>.cmi` alongside the generated `.o` object file.

## 2. CMI File Layout

The `.cmi` file is a UTF-8 encoded JSON document. It is designed to be human-readable, easily inspected by external tooling, and quickly deserialized by the `coral66` compiler.

The JSON schema consists of two top-level properties: `header` and `symbols`.

### 2.1 Header Block

The `header` object provides metadata identifying the library and schema version:

* **`library_name`** (`String`): The canonical name of the library (matching the identifier after the `SEGMENT` keyword).
* **`cmi_version`** (`String`): A semantic version string indicating the `.cmi` schema version (currently `"1.0.0"`, defined by `CMIProcessor::CURRENT_CMI_VERSION`). During consumption, the parser verifies that `cmi_version` matches the compiler's supported version.

### 2.2 Symbols Block

The `symbols` array contains the serialized representations of all exported declarations. Every object in this array shares three common fields:

* **`name`** (`String`): The Coral 66 identifier of the symbol (unmangled).
* **`communicator_type`** (`String`): The communicator origin of the symbol (`"Common"`, `"External"`, `"Library"`, `"Absolute"`, or `"None"`). For symbols exported via a library's `COMMON` block, this is `"Common"` (defaults to `"Library"` during parsing if omitted).
* **`symbol_category`** (`String`): The declaration category (`"data"`, `"procedure"`, `"array"`, `"table"`, `"table_element"`, or `"overlay"`).

#### Primitive Type Encodings

Primitive Coral 66 data types are represented in `.cmi` JSON fields using the following strings:
* `"INTEGER"` — Coral 66 `INTEGER` (and `FIXED`)
* `"REAL"` — Coral 66 `FLOATING`
* `"BITS"` — Coral 66 `BITS` (and part-word types)
* `"POINTER"` — Coral 66 `POINTER`
* `"NONE"` — Untyped return type (used for untyped `PROCEDURE` declarations)

#### Category-Specific Fields

* **Data (`"data"`)**:
    * **`data_type`** (`String`): The primitive type of the scalar variable (`"INTEGER"`, `"REAL"`, `"BITS"`, or `"POINTER"`).
* **Procedures (`"procedure"`)**:
    * **`return_type`** (`String`): The return type (`"INTEGER"`, `"REAL"`, `"BITS"`, `"POINTER"`, or `"NONE"` for untyped procedures).
    * **`parameters`** (`Array` of `Object`): Ordered list of formal parameters. Each parameter object contains:
        * **`name`** (`String`): The parameter identifier, or `""` (empty string) when the parameter is unnamed in a `COMMON` procedure specification.
        * **`pass_by`** (`String`): The parameter passing convention (`"VALUE"` or `"LOCATION"`).
        * **`data_type`** (`String`): The parameter data type (`"INTEGER"`, `"REAL"`, `"BITS"`, or `"POINTER"`).
* **Arrays (`"array"`)**:
    * **`data_type`** (`String`): The primitive element type (`"INTEGER"`, `"REAL"`, `"BITS"`, or `"POINTER"`).
    * **`dimensions`** (`Array` of `UInt64`): The size of each array dimension.
* **Tables (`"table"`)**:
    * **`table_name`** (`String`): The name of the table.
    * **`table_id`** (`UInt64`): Internal table identifier.
    * **`width`** (`UInt64`): Table width in words per entry.
    * **`length`** (`UInt64`): Number of entries in the table.
    * **`fields`** (`Array` of `Object`): Array of inline table field definitions (if present on the `TableDefinition`), where each object specifies `name` (`String`) and `data_type` (`String`). Note that named table element declarations are also emitted as separate `"table_element"` symbol entries.
* **Table Elements (`"table_element"`)**:
    * **`data_type`** (`String`): Element data type (e.g., `"INTEGER"`, `"BITS"`).
    * **`is_unsigned`** (`Boolean`): `true` if declared `UNSIGNED`, `false` if signed.
    * **`word_position`** (`UInt32`, optional): Word offset within a table entry.
    * **`bit_position`** (`UInt32`, optional): Bit offset for part-word table elements.
    * **`parent_table`** (`String`, optional): The `name` of the parent `"table"` symbol in the same `.cmi` file. During deserialization, this links the `TableElementDeclaration` back to its parent `TableDeclaration`.
* **Overlays (`"overlay"`)**:
    * **`overlay_name`** (`String`): The name of the overlay.
    * **`overlay_id`** (`UInt64`): Internal overlay identifier.
    * **`fields`** (`Array` of `Object`): Array of field objects, each containing `name` (`String`) and `data_type` (`String`).

**Ignored Entities:**
While the `COMMON` communicator in monolithic, single-pass Coral 66 compilation supports specifying global `SWITCH` labels (`SwitchDeclaration`) and `GOTO` labels (`LabelDeclaration`), these entities **are explicitly skipped** during CMI generation. Jumping to an arbitrary memory address or switch table across pre-compiled object-file boundaries violates calling conventions, stack frame management, and modern linker execution models, so they cannot form part of a pre-compiled library's public API.

## 3. Examples

The following examples demonstrate creating a Coral 66 library, inspecting the generated `.cmi` file, and importing it into a main program.

### 3.1 The Library Source (`myMaths.cor`)

Note that floating-point types in Coral 66 source code use the `FLOATING` keyword (which maps to `"REAL"` in the `.cmi` file):

```coral66
SEGMENT myMaths
COMMENT This is the library source ;

COMMON (
    FLOATING PROCEDURE myMathsSin(VALUE FLOATING);
    FLOATING pi := 3.14159
)

BEGIN
    FLOATING PROCEDURE myMathsSin(VALUE FLOATING x);
    BEGIN
        COMMENT Internal Taylor series implementation ... ;
        ANSWER x
    END;
END
```

### 3.2 The Generated CMI File (`myMaths.cmi`)

When the library above is compiled to an object file (`coral66 -c myMaths.cor -o myMaths.o` or `./scripts/compile.sh myMaths.cor -c`), the compiler generates `myMaths.o` and the following `myMaths.cmi` JSON file:

```json
{
    "header": {
        "library_name": "myMaths",
        "cmi_version": "1.0.0"
    },
    "symbols": [
        {
            "name": "myMathsSin",
            "communicator_type": "Common",
            "symbol_category": "procedure",
            "return_type": "REAL",
            "parameters": [
                {
                    "name": "",
                    "pass_by": "VALUE",
                    "data_type": "REAL"
                }
            ]
        },
        {
            "name": "pi",
            "communicator_type": "Common",
            "symbol_category": "data",
            "data_type": "REAL"
        }
    ]
}
```
*(Note: Because the formal parameter in the `COMMON` communicator specification `(VALUE FLOATING)` does not specify a parameter name, `CMIProcessor` records `"name": ""`. The formal parameter identifier `x` is local to the procedure definition.)*

### 3.3 Table and Table Element Example (from `support/library/time.cmi`)

When a `TABLE` and its elements are declared in a `COMMON` block (such as `TABLE tm [14, 1] [ tmsec INTEGER 0; ... ]` in `support/library/time.cor`), the `.cmi` file serializes the table followed by its `table_element` declarations:

```json
{
    "header": {
        "library_name": "time",
        "cmi_version": "1.0.0"
    },
    "symbols": [
        {
            "name": "tm",
            "communicator_type": "Common",
            "symbol_category": "table",
            "table_name": "tm",
            "table_id": 0,
            "width": 14,
            "length": 1,
            "fields": []
        },
        {
            "name": "tmsec",
            "communicator_type": "Common",
            "symbol_category": "table_element",
            "data_type": "INTEGER",
            "is_unsigned": false,
            "word_position": 0,
            "parent_table": "tm"
        }
    ]
}
```

### 3.4 The Consuming Program (`main.cor`)

When a consuming program declares `LIBRARY (myMaths)`, the compiler locates `myMaths.cmi` in the library search path (`-L`), parses the JSON, and injects `myMathsSin` and `pi` into the program's global symbol table.

```coral66
PROGRAM main

LIBRARY (myMaths)

BEGIN
    FLOATING result;
    result := myMathsSin(pi);
END
```

## 4. Separation of Interface and Implementation (Linkage)

A common question when viewing a `.cmi` file is: *"If a preset variable like `FLOATING pi := 3.14159` is exported, why isn't the value `3.14159` in the JSON?"*

The `.cmi` file serves strictly as an **interface definition**—analogous to a C/C++ header file containing `extern float pi;`. It provides the consuming compiler with the type and communicator metadata required to enforce type safety and emit external LLVM Intermediate Representation (IR) declarations.

1. **The Compiler's Role:** When `main.cor` is compiled, the compiler reads `pi` from `myMaths.cmi` with `communicator_type: "Common"` and emits an external global declaration using its mangled name (`@_c66_common_pi = external global float`; see [Name Mangling Specification](name-mangling-specification.md)). It does not need to know the initializer value to compile `main.cor`.
2. **The Object File's Role:** When `myMaths.cor` was compiled, the initial value `3.14159` was emitted into the data section of `myMaths.o` under the symbol `_c66_common_pi` (or implemented in an external C wrapper object).
3. **The Linker's Role:** During the final link step, the system linker (`gcc`, invoked by `ActionExecutor::linkExecutable`) links `main.o` and `myMaths.o` together, resolving the external reference in `main.o` to the definition in `myMaths.o`.

The same principle applies to procedures: the `.cmi` file provides the signature so the compiler can validate call-site arguments and types, while the executable machine code resides in the linked `.o` file.

## 5. Deserialization and Consumption

When `SymbolResolutionPass` encounters a `LIBRARY (<Name>, ...)` communicator:

1. **Resolution:** For each library identifier `<Name>`, the semantic analyzer constructs the path `<libraryPath>/<Name>.cmi` using the directory provided via the `-L` command line option (defaulting to the current directory if `-L` is omitted).
2. **Validation:** `CMIProcessor::parseCMIFile` parses the JSON document and checks that `header` and `symbols` exist, `header.cmi_version` equals `CURRENT_CMI_VERSION` (`"1.0.0"`), and `symbols` is a valid JSON array. If reading or validation fails, the compiler reports the `FAILED_TO_READ_CMI_FILE_N` semantic error.
3. **Injection:** Iterating through the `symbols` array, `CMIProcessor` instantiates the corresponding semantic `Declaration` objects (`VariableDeclaration`, `ProcedureDeclaration`, `ArrayDeclaration`, `TableDeclaration`, `TableElementDeclaration`, or `OverlayDeclaration`) backed by synthetic identifier tokens and restores their `CommunicatorType`. `SymbolResolutionPass` then inserts each declaration into the global scope of the current translation unit's symbol table (reporting `REDEFINITION_OF_SYMBOL_N` on collisions), allowing the rest of semantic analysis and LLVM code generation to reference them as external communicator symbols.
