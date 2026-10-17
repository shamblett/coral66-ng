# Project: coral66-ng

## Purpose:
- The project is an implementation of the Coral 66 programming language in a modern computing environment. It uses the LLVM compiler generation suite of tools to implement a fully functioned Coral 66 compiler targeted at a Linux, x86-64 environment.

# Project Memory
- The coral66-ng project stores its specification files in the documentation/specification directory. This contains the Coral 66 specification document "Official Definition of Coral 66.txt" and its associated grammar file "coral66-grammar.txt". These are collectivelly referred to as the specification or the spec.
- ASAN and UBSAN sanitizer libraries are installed and configured in .bazelrc. Use `--config=asan` or `--config=ubsan` with bazel commands for debugging. LLDB is also installed and should be used (in batch mode) instead of GDB.

# Antigravity Rules

## Build and Test System
- The project uses **Bazel** as its build and test system (e.g., `bazel build //...`, `bazel test //test/...`).
- NEVER use `cmake` or `ctest` in this workspace.

## Safety and Version Control
- NEVER automatically run `git commit`, `git add .`, or `git push` commands.
- Do not attempt to save project state by generating git commits.
- The user will handle all version control tasks.
