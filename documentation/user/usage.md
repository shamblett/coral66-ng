# Coral 66 Compiler Usage Guide

This document provides instructions on how to use the `coral66` compiler command line options, its helper scripts, and supporting tools.
It is assumed the `coral66` binary is on your `PATH`; this can be achieved by running the `./scripts/install.sh` script after building the compiler (see [Building and installing](build-and-install.md) for details).

## 1. Command Line

The `coral66` compiler is invoked with the following general syntax:

```bash
coral66 [options] <input source file> [object files...]
```

Single-character and multi-character options may be prefixed with either a single dash (`-`) or a double dash (`--`).

### Core Options

* **`<input source file>`**: The Coral 66 source file to compile (must have a `.cor` extension). Files beginning with `PROGRAM` compile to an executable by default; files beginning with `SEGMENT` are treated as libraries and also generate a Coral Module Interface (`.cmi`) file alongside their output.
* **`[object files...]`**: Optional compiled object files (`.o`) to link into the final executable when linking.
* **`-o <filename>`**: Specifies the name of the output file. If omitted, the output name is derived from the input source file:
    * Default (compile and link): `<source_stem>` (executable with no extension).
    * With `-c`: `<source_stem>.o` (or `<source_stem>.bc` when combined with `--emit-llvm`).
    * With `-s`: `<source_stem>.s` (or `<source_stem>.ll` when combined with `--emit-llvm`).
* **`-c`**: Stop after compiling and output an object file (`.o`). Does not link the final executable. Used when creating libraries from `SEGMENT` source files (which also generates `<output_stem>.cmi`).
* **`-s`**: Stop after compiling and output an assembly file (`.s`), or human-readable LLVM IR (`.ll`) if `--emit-llvm` is also specified.
* **`-e`**: Run the preprocessor only and print the reconstructed source code to `stdout`.
* **`--emit-llvm`** (or `-emit-llvm`): Emit LLVM IR instead of native machine code or assembly. Produces LLVM bitcode (`.bc`) by default or with `-c`, or textual LLVM IR (`.ll`) when combined with `-s`. Does not link an executable.
* **`--syntax-only`** (or `-syntax-only`): Stop after semantic analysis and produce no output file. Useful for verifying syntax and type checking.
* **`--parse-only`** (or `-parse-only`): Stop after parsing and produce no output file.

### Environment & Include Paths

* **`-L <dirpath>`**: Specifies the library directory path. The compiler searches this directory for Coral Module Interface (`<library_name>.cmi`) files when resolving `LIBRARY` communicators.
* **`-X <dirpath>`**: Specifies the external code directory path for non-Coral `EXTERNAL` dependencies (such as compiled C wrapper files).

### Target Environment & Code Generation

* **`--target-env=<value>`**: Specifies the target environment for the compiler:
    * `os`: Operating System (default). Suitable for standard hosted OS targets.
    * `baremetal`: Bare-metal. Enables support for the `ABSOLUTE` communicator, allowing variables to be mapped to specific physical memory addresses.
* **`--pic`** (or `-pic`): Build position independent code.
* **`--pie`** (or `-pie`): Build a position independent executable.

### Optimization & Debugging

* **`-O1`, `-O2`, `-O3`** (or `--O1`, `--O2`, `--O3`, `-O=<level>`): Optimization levels. If omitted, no optimization passes are applied (`O0`). `O1` enables trivial optimizations, `O2` enables standard default optimizations, and `O3` enables aggressive/expensive optimizations.
* **`-g1`, `-g2`, `-g3`** (or `--g1`, `--g2`, `--g3`, `-g=<level>`): Debug information levels. If omitted, no debug metadata is emitted. `g1` emits line tables only, `g2` generates standard debugging information, and `g3` generates extended debugging information.
     *Note: Not implemented in version 1, for future use only, these flags have no effect on the generated object files or executables.*
* **`--color`**: Use colors in diagnostic output (default is automatic detection based on terminal capabilities).
* **`--time`** (or `-time`): Output the time taken for each compilation stage (`Compile`, `Pass Management`, etc.).

### Other Options

* **`--help`** (or `-help`, `--help-list`): Display available command line options.
* **`--version`** (or `-version`): Display the compiler version.
* **`--noact`** (or `-noact`): Check command line parameter validity, emit a warning that no action was taken, and exit without compiling.

## 2. Common Tasks

Here are examples of performing common compilation tasks using the `coral66` compiler directly.

### Compiling a Simple Program

To compile a self-contained `PROGRAM` source file into an executable named `my_program`:

```bash
coral66 main.cor -o my_program
```

If `-o` is omitted, the resulting executable is named after the source file without the `.cor` extension (`main`).

### Creating a Library

A Coral 66 library source file starts with the `SEGMENT <name>` header and declares its public API within a `COMMON` communicator (see [Program Structure, Linking and Externals](program-structure-linking-and-externals.md) and [CMI Specification](cmi-specification.md)).

To compile `math_utils.cor` into a reusable library object file, use the `-c` flag:

```bash
coral66 -c math_utils.cor -o math_utils.o
```

This generates both the compiled object file (`math_utils.o`) and the Coral Module Interface file (`math_utils.cmi`) in the output directory.

### Compiling a Main Program with a Library Dependency

To compile a main program (`main.cor`) that imports `math_utils` via a `LIBRARY` communicator, provide the directory containing `math_utils.cmi` with `-L` and pass `math_utils.o` so the linker can resolve its symbols:

```bash
# Assuming math_utils.o and math_utils.cmi are in the 'libs' directory
coral66 main.cor libs/math_utils.o -L libs -o main_app
```

### Compiling a Main Program with an External Dependency

If your Coral 66 code calls external functions (e.g., written in C) declared via an `EXTERNAL` communicator (or implements mangled `_c66_common_` library symbols in C; see [Name Mangling Specification](name-mangling-specification.md)), first compile the C source to an object file, then compile and link your Coral 66 program:

```bash
# 1. Compile the C code (or use ./scripts/compile-external.sh)
gcc -c ext_src/my_c_funcs.c -o ext_src/my_c_funcs.o

# 2. Compile and link the Coral 66 code
coral66 main.cor ext_src/my_c_funcs.o -X ext_src -o main_app
```

## 3. Compilation Helper Scripts

To simplify building programs with multiple library and external dependencies, two primary compilation wrapper scripts are provided in the `scripts/` directory: `scripts/compile-supported.sh` and `scripts/compile.sh`.

* **`scripts/compile.sh`** wraps the `coral66` compiler and automatically constructs the list of `.o` files and `-L`/`-X` search paths from library (`-l`) and external (`-x`) names.
* **`scripts/compile-supported.sh`** wraps `scripts/compile.sh` and automatically links against all pre-built support libraries (`support/library`, providing `stdio`, `string`, `time`, and `unistd`) and external C wrappers (`support/external`). For new users and general program compilation, `compile-supported.sh` is the recommended starting point.

### Compile Supported Script Usage

```bash
./scripts/compile-supported.sh <source_file.cor> [options...]
```

By default, `compile-supported.sh` searches `support/library` and `support/external` in the repository and links every `.o` file found there. You can override the library or external search directory with `-L`/`--libpath` or `-X`/`--extpath`, and pass any additional `compile.sh` or `coral66` options (such as `--O2`, `-g2`, `-c`, or `-s`).

#### Examples using `compile-supported.sh`

**1. Compiling an example program from the repository root:**

```bash
./scripts/compile-supported.sh examples/helloworld/helloworld.cor
```

**2. Compiling an example program from within its directory:**

```bash
cd examples/helloworld
../../scripts/compile-supported.sh helloworld.cor
```

**3. Compiling with optimization enabled:**

```bash
./scripts/compile-supported.sh examples/sieve/sieve.cor --O3
```

### Compile Script Usage

```bash
./scripts/compile.sh <source_file.cor> [-l|--lib <library_name>] [-L|--libpath <library_path>] [-x|--ext <external_name>] [-X|--extpath <external_path>] [-c] [-s] [compiler_options...]
```

* **`-l, --lib <library_name>`**: Name of a Coral 66 library (without `.o` extension) located in the library path. May be specified multiple times.
* **`-L, --libpath <library_path>`**: Directory containing Coral 66 library `.o` and `.cmi` files (defaults to `.`).
* **`-x, --ext <external_name>`**: Name of an external object file (without `.o` extension) located in the external path. May be specified multiple times.
* **`-X, --extpath <external_path>`**: Directory containing external `.o` files (defaults to `.`).
* **`-c`**: Compile to an object file (`<source_stem>.o`, plus `<source_stem>.cmi` for `SEGMENT` files) without linking.
* **`-s`**: Compile to an assembly file (`<source_stem>.s`) without linking.
* **`[compiler_options...]`**: Any other flags starting with `-` (e.g., `--O2`, `--g2`, `--pic`, `--time`) are forwarded directly to `coral66`.

#### Examples using `compile.sh`

**1. Compiling a standalone program:**
The script automatically names the output executable after the source file (e.g., `main`).

```bash
./scripts/compile.sh main.cor
```

**2. Compiling a library segment to an object and `.cmi` file:**

```bash
./scripts/compile.sh math_utils.cor -c
```

**3. Compiling with a library dependency:**
Specify the library name (without `.o`) and its directory path; the script resolves and links `libs/math_utils.o` and passes `-L libs` to `coral66`.

```bash
./scripts/compile.sh main.cor -l math_utils -L libs
```

**4. Compiling with an external dependency:**
Specify the external module name and its directory path; the script resolves and links `ext_src/my_c_funcs.o`.

```bash
./scripts/compile.sh main.cor -x my_c_funcs -X ext_src
```

**5. Compiling with multiple dependencies and compiler flags:**
Combine multiple `-l` and `-x` flags along with `coral66` options to build larger projects.

```bash
./scripts/compile.sh main.cor -l stdio -l string -L support/library -x stdio_wrappers -x string_wrappers -X support/external --O2
```

## 4. Other Helper Scripts

In addition to `compile.sh` and `compile-supported.sh`, several other scripts are provided in `scripts/` (and at the repository root) to manage installation, support libraries, examples, and builds.

### `scripts/install.sh`

Installs the compiled `coral66` binary into `$HOME/bin/coral66` (automatically detecting a local Bazel build at `bazel-bin/bin/coral66` or a Podman build at `podman-build/coral66`), and optionally copies the external and library directories to user-specified locations.

```bash
./scripts/install.sh [-h] [-X <external_dest_path>] [-L <library_dest_path>]
```

* **`-h`**: Display the help message and exit.
* **`-X <PATH>`**: Copy the contents of the `external` directory to `<PATH>`.
* **`-L <PATH>`**: Copy the contents of the `library` directory to `<PATH>`.

### `scripts/compile-external.sh`

Compiles a single C source file into an object file (`<basename>.o`) in the current working directory using `gcc -c`, ready for linkage with `coral66`. Additional `gcc` flags can be appended.

```bash
./scripts/compile-external.sh <source.c> [additional gcc options...]
```

Example:

```bash
./scripts/compile-external.sh support/external/string_wrappers.c -O2
```

### `scripts/build-externals.sh`

Builds all C source files (`*.c`) in an externals directory into `.o` object files using `scripts/compile-external.sh`.

```bash
./scripts/build-externals.sh [-X <externals_directory>]
```

If `-X` is omitted, it defaults to rebuilding the supplied external wrappers in `support/external/` (`stdio_wrappers.c`, `string_wrappers.c`, `time_wrappers.c`, `unistd_wrappers.c`).

### `scripts/build-libraries.sh`

Builds all Coral 66 library source files (`*.cor`) in a library directory into `.o` object files and `.cmi` interface files using `scripts/compile.sh -c`.

```bash
./scripts/build-libraries.sh [-L <library_directory>]
```

If `-L` is omitted, it defaults to rebuilding the supplied support libraries in `support/library/` (`stdio.cor`, `string.cor`, `time.cor`, `unistd.cor`).

### `scripts/build-examples.sh`

Compiles all example programs under the `examples/` directory (`ackerman`, `countdown`, `helloworld`, `sieve`, `time`, and `whetstone`) against the support libraries and externals using `scripts/compile-supported.sh`.

```bash
./scripts/build-examples.sh
```

### `scripts/clean-build.sh`

Completely cleans the local build environment by expunging the Bazel build cache (`bazel clean --expunge`) and clearing `ccache` (`ccache --clear`).

```bash
./scripts/clean-build.sh
```

### Podman Build Scripts (`developer-podman.sh` and `user-podman.sh`)

Located in the repository root, these scripts build and test `coral66-ng` inside a Fedora 44 Podman container without requiring local Bazel or LLVM 22 installations:

* **`./developer-podman.sh`**: Mounts the local repository into the container, runs the Bazel build and test suite, and outputs the `coral66` binary and logs to `./podman-build/`.
* **`./user-podman.sh [branch]`**: Clones a clean copy of the repository (defaulting to `main`) inside the container, builds and tests it hermetically, and extracts the binary and logs to `./user_build_artifacts/`.

See [Building and installing](build-and-install.md) for full details on container builds and SELinux configuration.

## 5. Support Tools

The `tools/` directory contains diagnostic utilities, a source formatter, and editor integration files to assist with developing and debugging Coral 66 programs and the compiler itself.

### Preprocessor Dump (`ppdump`)

Located in `tools/preprocessor/ppdump`, `ppdump` runs the lexer and macro preprocessor on a Coral 66 source file and writes the reconstructed post-preprocessing source code to `<input_stem>_pp.cor` in the same directory as the input file.

```bash
bazel run //tools/preprocessor/ppdump/bin:ppdump -- /absolute/path/to/source.cor
```

### Parser AST Dump (`astdump`)

Located in `tools/parser/astdump`, `astdump` lexes, preprocesses, and parses a Coral 66 source file, then serializes the parser's Abstract Syntax Tree (AST) into a formatted JSON file named `<input_stem>_ast.json` alongside the input source file. It is used for fault investigation and parser golden testing.

```bash
bazel run //tools/parser/astdump/bin:astdump -- /absolute/path/to/source.cor
```

### Semantics Program Dump (`programdump`)

Located in `tools/semantics/programdump`, `programdump` executes the complete frontend pipeline (lexing, preprocessing, parsing, and semantic analysis) and dumps the resulting semantic program context to a formatted JSON file named `<input_stem>_program.json` alongside the input source file.

The generated JSON document contains:
* The symbol table
* Resolved identifiers
* Expression types
* Usables (resolved symbols)
* Auxiliary types
* Table information
* Semantic errors and diagnostics

```bash
bazel run //tools/semantics/programdump/bin:programdump -- /absolute/path/to/source.cor
```

### Coral 66 Source Formatter (`coral-fmt.py`)

Located at `tools/formatter/coral-fmt.py`, this Python script formats Coral 66 source files in-place by normalizing block indentation according to structural keywords (`BEGIN`, `THEN`, `DO`, `END`, `ELSE`).

```bash
./tools/formatter/coral-fmt.py <source_file.cor> [--indent <size>]
```

* **`--indent <size>`**: Number of spaces per indentation level (defaults to `2`).

### Editor & IDE Syntax Highlighting

Syntax highlighting and editor settings for Coral 66 source files are provided under `tools/platform/syntax-highlighting/`:

* **Kate / KTextEditor**: `tools/platform/syntax-highlighting/kate/coral66.xml`
* **CLion**: `tools/platform/syntax-highlighting/clion/settings.zip`
