# coral66-ng
A Coral66 compiler for the next generation implemented in C++.

## Introduction
This project provides a Coral 66 language compiler for x64 linux based platforms. It implements the 
Official Definition of Coral 66 as defined in [this](documentation/specification/Official%20Definition%20of%20Coral%2066.pdf) 
document published by the UK MOD in 1970 and qualified
by the compliance matrix found [here](documentation/compliance/Compliance%20Matrix.ods). The language is largely fully 
compliant with this specification, implementation of libraries and the linkage of these to the main program is achieved through an implementation defined means as
allowed by the specification.

The compiler itself named 'coral66' comprises of a single binary file, usual compiler like options can be specified
on the command line to control output file generation, library linkage and final image attributes such as optimization,
PIC/PIE etc.

A simple usage example :-
```
coral66 --O3 --pic main.cor
```
compiles the Coral 66 source file 'main.cor' at optimization level 3 in pic format. The output of the compiler is a single
runnable executable or object file.

Various code examples with instructions on how to build them can be found in the [examples](examples) directory with compiler usage examples
and options documented in the usage document [here](documentation/user/usage.md).

### Implementation
The compiler uses a standard frontend implementation comprising a lexer, parser, preprocessor and semantic analysis phase. This produces a
semantic AST that is used by the backend code generation phase. LLVM is used to generate the code although no LLVM
libraries are needed at runtime. A working gcc install however is required as gcc is used to provide final linkage of compiled object files.
The C++ standard used is c++23 allowing use of modern idiomatic C++ constructs such as conceptss etc.

Currently only a linux x64 ABI is implemented although provision has been made for a bare metal target for future usage.

### Build System and Dependencies
The project is fully contained in a single repository built using the [bazel](https://bazel.build/) build system. 
You will need a local Bazel install to build the compiler locally however podman scripts are provided to build the
compiler in a container if you do not want to to install Bazel and other dependencies.

All code needed to build the compiler is in this repository, including unit test harness code and supporting source libraries.
The code does however depend on an installed LLVM development toolchain targeting LLVM version 22.

To build the compiler an run all the unit tests do the following :-
```aiignore
bazel test //...
```

To just build the compiler :-
```
bazel build //...
```

The unit test harness used is the [Catch2](https://github.com/catchorg/catch2) library.

### Batteries Included
Unlike many other languages Coral 66 does not specify I/O functionality 
such as 'print' statements etc. The omission of these makes the language largely unusable on modern platforms.
To alleviate this the compiler comes equipped with a basic set of library functions implementing subsets of stdio, string, unistd and time C functions from the
platforms libc. This allows program I/O to be performed from the get go, see the [support](support) directory for details.

The compiler allows easy implementation of external C functions using the EXTERNAL and LIBRARY communicators so the user can add any external
functionality needed.

A range of tools and helper scripts are also provided to facilitate building of programs using the supporting libraries and diagnostic
generation through the dumping of parser and semantic AST's.

### Credits
Besides the dependence on LLVM for code generation the [cld](https://github.com/zero9178/cld) project provided the overall project structure and support 
implementations although the frontend and backend has been completely re written for coral66-ng.

Also a shout out to my coding assistants, those being gemini-cli originally and antigravity now. I learned lots about using these tools developing this project,
what they are good at and what they aren't. A question often asked in here is 'Could you have done this without AI assistance?'. The answer to this is 'Yes'.
A better question is 'Could you have done this in the same timeframe without AI assistance?'. The answer being an unqualified 'No, not a chance' and that includes a
somewhat steep learning curve at the start of the project.

### Motivation
So, why do this? Well, Coral 66 was the first programming language I had formal instruction on back in 1985. I then used it for a few years in production
environments on a number of defence projects, finally moving to Ada 83 in the early 90's so its sort of a historical thing for me. I looked around for some 
Coral 66 compilers and couldn't find any other than binary implementations on old PDP 11 images which was not really what I wanted, so in true open source  fashion 
I decided  to scratch that particular itch by providing one myself.

## Getting Started

To build and install the compiler please read the build and install document [here](documentation/user/build-and-install.md)

Once installed the [usage](documentation/user/usage.md) document describes usage of the compiler from the command line and its supporting helper scripts.

The user directory in the documentation directory contains other documents describing Coral 66 program structure, library creation etc. and can be read in
any order you wish although the [Program Structure](documentation/user/program-structure-linking-and-externals.md) document is a good starting place.

The examples directory contains several examples for your perusal, these can be built from the command line, using the helper scripts or
the build-examples.sh script in the [scripts](scripts) directory.

## Other reading
The documentation directory contains the specification document, along with others. The compliance
directory contains the compliance matrix for the compiler. The development directory is effectively the project development file.
This contains a lot of information on why/how features were added, review documentation and other general development information.
Note this is for general interest only, some information it contains may be wrong or outdated. Current functionality supported by
the compiler is to be found in the user directory.

## Maintenance
Please raise any issues or submit any PR's you wish for bugs, feature additions etc.


