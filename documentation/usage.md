# Coral 66 Compiler Usage Guide

This document provides instructions on how to use the `coral66` compiler command line options and its supporting scripts.
It is assumed the coral66 binary is on your path, this can be achieved by running the install.sh script before using the compiler.

## 1. Command Line

The `coral66` compiler is invoked with the following general syntax:

```bash
coral66 [options] <input source file> [object files...]
```

### Core Options

*   **`<input source file>`**: The main Coral 66 source file to compile (usually `.cor`).
*   **`[object files...]`**: Optional compiled object files (`.o`) to link into the final executable.
*   **`-o <filename>`**: Specifies the name of the output executable or object file.
*   **`-c`**: Stop after compiling object files. Does not link the final executable. Useful for creating libraries.
*   **`-s`**: Stop after compiling and output assembly files (`.s`).
*   **`-e`**: Preprocess to stdout or the specified output file.
*   **`--emit-llvm`**: Emit LLVM IR instead of machine code and assembly.
*   **`--syntax-only`**: Stop after semantic analysis and produce no output file. Useful for quick syntax checking.
*   **`--parse-only`**: Stop after parsing and produce no output file.

### Environment & Include Paths

*   **`-L <dirpath>`**: Specifies the library directory path. The compiler will look here for Coral 66 `LIBRARY` dependencies.
*   **`-X <dirpath>`**: Specifies the external code directory path. The compiler will look here for non-Coral `EXTERNAL` dependencies (like C functions).

### Target Environment & Code Generation

*   **`--target-env=<value>`**: Specifies the target environment for the compiler.
    *   `os`: Operating System (default). Suitable for modern OS targets.
    *   `baremetal`: Bare-metal. Enables support for `ABSOLUTE` addressing, allowing mapping variables to specific memory addresses.
*   **`--pic`**: Build position independent code.
*   **`--pie`**: Build a position independent executable.

### Optimization & Debugging

*   **`--O1`, `--O2`, `--O3`**: Optimization levels. `O1` enables trivial optimizations, `O2` (default) enables standard optimizations, and `O3` enables expensive optimizations.
*   **`--g1`, `--g2`, `--g3`**: Debug levels. `g1` provides lines only, `g2` generates standard debugging info, and `g3` generates extended debugging info.
*   **`--color`**: Use colors in output (default=autodetect).
*   **`--time`**: Output the time taken for each stage of compilation.

### Other Options

*   **`-v`**: Display the version of this program.
*   **`--noact`**: Check command line parameter validity, take no further action.

## 2. Common Tasks

Here are some real-world examples of how to perform common tasks using the `coral66` compiler directly.

### Compiling a simple program

To compile a self-contained program into an executable named `my_program`:

```bash
coral66 main.cor -o my_program
```

### Creating a Library

If you have a file `math_utils.cor` that contains a set of common macros or procedures that you want to include in other programs, compile it to an object file using the `-c` flag:

```bash
coral66 -c math_utils.cor -o math_utils.o
```

### Compiling a Main Program with a Library Dependency

To compile a main program (`main.cor`) that depends on the `math_utils` library created above. You need to provide the path to the directory containing the library using `-L`, and link the object file:

```bash
# Assuming math_utils.o is in the 'libs' directory
coral66 main.cor libs/math_utils.o -L libs -o main_app
```

### Compiling a Main Program with an External Dependency

If your Coral 66 code calls external functions (e.g., written in C) declared using an `EXTERNAL` communicator. First, compile your C code to an object file, then compile your Coral code, linking the external object and pointing `-X` to the directory where the external code resides:

```bash
# 1. Compile the C code
gcc -c my_c_funcs.c -o my_c_funcs.o

# 2. Compile and link the Coral 66 code
# Assuming my_c_funcs.o is in the 'ext_src' directory
coral66 main.cor ext_src/my_c_funcs.o -X ext_src -o main_app
```

## 3. Helper Scripts

To simplify the compilation process, especially when dealing with multiple libraries and external dependencies, helper scripts are provided named`scripts/compile.sh`
and `scripts/compile-supported.sh`.
The compile script is a wrapper around the `coral66` compiler that automatically handles object file linking and path management for you. 
The compile-supported script wraps the compile script but links the source file against the supported libraries and externals without needing to specify them on the 
command line. For new users and getting started tasks this script is highly recommended. 

### Compile Supported Script Usage

```bash
./scripts/compile-supported.sh <source_file.cor>
```
### Examples using the compile supported helper script

**1. Compiling a simple program:**
The script automatically finds and links the supported libraries and externals. To compile the helloworld example in its directory

```bash
../../scripts/compile-supported.sh helloworld.cor
```

### Compile Script Usage

```bash
./scripts/compile.sh <source_file.cor> [-l library_name] [-L library_path] [-x external_name] [-X external_path]
```

### Examples using the Compile helper Script

**1. Compiling a simple program:**
The script automatically names the output executable based on the source file name (e.g., `main`).

```bash
./scripts/compile.sh main.cor
```

**2. Compiling with a library dependency:**
Instead of manually specifying the `.o` file, you provide the library name (without the `.o` extension) and the directory path. The script will automatically resolve and link `libs/math_utils.o`.

```bash
./scripts/compile.sh main.cor -l math_utils -L libs
```

**3. Compiling with an external dependency:**
Similarly, for external code, you provide the external name and the path. The script will automatically link `ext_src/my_c_funcs.o`.

```bash
./scripts/compile.sh main.cor -x my_c_funcs -X ext_src
```

**4. Compiling with multiple dependencies:**
You can combine multiple `-l` and `-x` flags to build complex projects easily.

```bash
./scripts/compile.sh main.cor -l string_utils -L common_libs -x hardware_io -X drivers
```