# Name Mangling Specification

This specification defines the deterministic process by which Coral 66 source identifiers are transformed into ABI-compatible symbol names for the LLVM backend. The goal is to provide linker isolation from the C Standard Library while maintaining a predictable and easily demangled naming scheme.

## 1. General Principles
*   **Base Prefix:** All mangled symbols specific to the Coral 66 compiler will begin with the prefix `_c66_`.
*   **No Type Encoding:** Unlike C++, Coral 66 does not support function overloading based on argument types. The mangled name depends *only* on the identifier and its communicator/scope context.
*   **Linker Visibility:** While identifiers that are visible outside the current translation unit (i.e., in communicators) strictly *require* mangling for the linker, module-level internal globals will also be mangled for clarity in the intermediate representation (IR) and debugging.

## 2. Communicator Specific Mangling

### 2.1 EXTERNAL Communicator
Variables and procedures declared within an `EXTERNAL` communicator represent direct bindings to the C ABI.
*   **Rule:** NO MANGLING.
*   **Format:** `<identifier>`
*   **Linkage:** External
*   **Example:** `EXTERNAL printf` → `printf`
*   **Rationale:** Must perfectly match the existing symbols in external C libraries (like `libc`).

### 2.2 COMMON Communicator
The `COMMON` communicator is used to share variables and procedures between Coral 66 modules.
*   **Rule:** Prefix with `_c66_common_`.
*   **Format:** `_c66_common_<identifier>`
*   **Linkage:** External (Common / Weak / Global depending on definition vs declaration)
*   **Example:** `COMMON memcpy` → `_c66_common_memcpy`
*   **Rationale:** Isolates Coral 66 common blocks from C libraries. This allows a module to define a `COMMON memcpy` without colliding with `libc`'s `memcpy` during the linking phase.

### 2.3 LIBRARY Communicator
The `LIBRARY` communicator defines exports intended for a Coral 66 library module.
*   **Rule:** Prefix with `_c66_lib_`.
*   **Format:** `_c66_lib_<identifier>`
*   **Linkage:** External
*   **Example:** `COMMON time` → `_c66_lib_time`
*   **Rationale:** Logically distinguishes standard library exports from user-defined `COMMON` exports, preventing collisions between user code and the Coral 66 standard library.

### 2.4 ABSOLUTE Communicator
The `ABSOLUTE` communicator maps variables to specific hardware memory addresses.
*   **Rule:** No linker symbol generated.
*   **Format:** N/A (Handled via direct memory address generation in LLVM IR)
*   **Rationale:** These do not participate in the linking phase.

## 3. Non-Communicator Symbols

### 3.1 Module-Level Global Variables & Procedures
Variables and procedures declared outside of any communicator (and therefore internal to the module).
*   **Rule:** Prefix with `_c66_global_`.
*   **Format:** `_c66_global_<identifier>`
*   **Linkage:** Internal (Private to the LLVM module)
*   **Example:** `INTEGER my_counter` → `_c66_global_my_counter`
*   **Rationale:** Prevents collisions with any C macro names if headers were somehow involved in intermediate processing, and provides a clear namespace in the LLVM IR and debugging tools.

### 3.2 Local Variables
Variables declared inside procedures or blocks.
*   **Rule:** No ABI mangling required. Left as local LLVM IR registers/names (`%<identifier>`).
*   **Format:** `<identifier>`
*   **Rationale:** Local variables are not visible to the linker.


## 4. Demangling (For Diagnostics)
The compiler's error reporting (e.g., in `semantic_context` or `astdump`) and debugging output should implement a demangling utility to ensure users see original Coral 66 names.
*   If a string starts with `_c66_common_`, strip it and report as `COMMON <name>`.
*   If a string starts with `_c66_lib_`, strip it and report as `LIBRARY <name>`.
*   If a string starts with `_c66_global_`, strip it and report as `<name>`.

## External mapping implementation

Here is the corrected breakdown of how the name mangling affects the wrappers, using the correct Coral 66 syntax:

Option 1: Direct Implementation via Mangled Names in C
If the C wrappers only exist to adapt types (like casting double to float), you implement the _c66_common_ symbol directly in C.

You would update time_wrappers.c to directly implement the mangled Coral 66 symbol:

    1 #include <time.h>
    2
    3 // Implements the Coral 66 "COMMON clock" export directly
    4 int _c66_common_clock(void) {
    5     return (int)clock(); 
    6 }
    7
    8 // Implements the Coral 66 "COMMON time" export directly
    9 time_t _c66_common_time(time_t* arg) {
10     return time(arg);
11 }
Impact: You can delete the EXTERNAL declarations and the wrapper BEGIN ... END implementations from time.cor. The Coral 66 file becomes just the interface definition
that generates the .cmi:

```
1 SEGMENT time
2
3 COMMON (
4     INTEGER PROCEDURE clock;
5     POINTER PROCEDURE time(LOCATION INTEGER);
6     ! ...
7 )
```