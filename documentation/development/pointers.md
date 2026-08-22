# Pointers in Coral 66 Next Generation (coral66-ng)

## Introduction

The `POINTER` type is an extension to the standard Coral 66 language introduced specifically for the `coral66-ng` compiler. It does not exist in the Official Definition of Coral 66. 

It was introduced primarily to solve the impedance mismatch between classic 32-bit Coral 66 semantics and modern 64-bit operating systems and C libraries.

## Why `POINTER` was Introduced

In standard Coral 66, the primary data types are `INTEGER`, `FLOATING`, and `FIXED`. In the LLVM backend of `coral66-ng`, an `INTEGER` is represented as a 32-bit integer (`i32`). 

When interfacing with modern C libraries (such as `<stdio.h>`, `<time.h>`, or `<string.h>`), many functions return or accept 64-bit handles, such as memory addresses (`FILE*`, `char*`, `void*`) or 64-bit opaque types like `time_t` and `clock_t`.

If a Coral 66 program attempted to store a 64-bit C pointer inside a 32-bit `INTEGER`, the memory address would be silently truncated, leading to catastrophic segmentation faults when passed back to C. The `POINTER` type was introduced to safely hold and transport these 64-bit values as opaque handles without loss of data.

## Declarations and Qualifiers

A `POINTER` can be declared in several ways depending on its scope and how it is being passed to procedures.

### Basic Declaration
Variables can be declared as pointers just like any other basic type:

```coral
'POINTER' myFilePtr;
'POINTER' ptr1, ptr2;
```

### Procedure Parameters and Qualifiers
When defining procedures, particularly external C functions, `POINTER` can be qualified to dictate how the data is passed according to the ABI.

*   **`VALUE POINTER`**: Passes the pointer directly by value (e.g., passing a `FILE*` directly to `fread`).
*   **`LOCATION POINTER`**: Passes the memory address of the pointer itself (e.g., passing a `FILE**`).
*   **`POINTER`** (Unqualified): Defaults to standard Coral 66 reference semantics depending on context.

**Example in an EXTERNAL block:**
```coral
'EXTERNAL' (
    'COMMENT' C standard library fopen and fclose ;
    'POINTER' 'PROCEDURE' fopen('VALUE' 'POINTER', 'VALUE' 'POINTER');
    'INTEGER' 'PROCEDURE' fclose('VALUE' 'POINTER');
)
```

## Usage Examples

`POINTER` types are primarily used as opaque handles. You receive them from an external library and pass them back to other external procedures.

### Example: File I/O
```coral
'PROGRAM' FileTest
'LIBRARY' (stdio)
'BEGIN'
    'POINTER' fileHandle;
    'POINTER' filename;
    'POINTER' mode;

    'COMMENT' We assume filename and mode have been mapped to C strings ;
    fileHandle := fopen(filename, mode);

    'IF' fileHandle <> 0 'THEN'
    'BEGIN'
        'COMMENT' Successfully opened, do something, then close it ;
        fclose(fileHandle);
    'END'
'END'
```

## Constraints and Limitations

Because `POINTER` types are intended to act as opaque handles to foreign memory rather than native mathematical primitives, the semantic analyzer enforces strict constraints on their use:

1.  **No Mathematical Operations**: You **cannot** perform arithmetic on `POINTER` types. Operations such as addition (`+`), subtraction (`-`), multiplication (`*`), and division (`/`) are strictly forbidden by the compiler's type checker (`getCommonType`). 
    *   *Attempting to do so will result in a `Type mismatch in expression` compilation error.*
    *   *If pointer arithmetic or offset calculation is necessary, it must be performed within a C wrapper function.*
2.  **No Direct Dereferencing**: Coral 66 lacks a native dereference operator (`*ptr` in C) for `POINTER` types. To read or mutate the underlying memory a pointer points to, you must either pass the pointer to an external C function that performs the operation or safely map the memory to a Coral `TABLE` or `ARRAY` using `LOCATION` tricks or `OVERLAY` techniques, if supported.
3.  **Type Coercion limitations**: While `POINTER` types can sometimes be assigned from integers representing literal memory locations (e.g. `0` for `NULL`), mixing `INTEGER` and `POINTER` types in general expressions is strongly discouraged and will trigger type mismatch errors.

## Summary

The `POINTER` type is a pragmatic extension that allows `coral66-ng` to be a viable tool in modern 64-bit ecosystems. Treat them purely as transit vehicles for C references, leaning on external C wrappers to perform any heavy lifting, arithmetic, or dereferencing required on the underlying data.
