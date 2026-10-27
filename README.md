# coral66-ng
A Coral66 compiler for the modern age.

## Introduction
This project provides a Coral 66 language compiler for x64 linux based platforms. It implements the 
Official Definition of Coral 66 as defined in [this](documentation/specification/Official%20Definition%20of%20Coral%2066.pdf) 
document published by the UK MOD in 1970 and qualified
by the compliance matrix found [here](documentation/compliance/Compliance%20Matrix.ods). The language is largely fully 
compliant with the specification, implementation of libraries and the linkage of these to the main program is achieved through an implementation defined means as
allowed by the specification.

The compiler itself named 'coral66' comprises of a single binary file, usual compiler like options can be specified
on the command line to control output file generation, library linkage and final image attributes such as optimization,
PIC/PIE etc.

A simple usage example :-
```
coral66 --O3 --pic main.cor
```
compiles the Coral 66 source file 'main.cor' at optimisation level 3 in pic format. The output of the compiler is a single
runnable executable.

Various code examples with instructions on how to build them can be found in the [examples](examples) directory with compiler usage examples
and options documented in the usage document [here](documentation/user/usage.md).

## Implementation
The compiler uses a standard frontend implementation comprising a lexer, parser, preprocessor and semantic analyser. This produces a
semantic AST that is used by the backend code generation to generate the executable. LLVM is used to generate the code although no LLVM
libraries are needed at runtime. A working gcc install however is required as gcc is used to provide final linkage of compiled object files.

Currently only a linux x64 ABI is implemented although provision has been made for a bare metal target for future usage.






