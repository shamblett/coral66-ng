✦ Section 2.2 of the Official Definition of Coral 66, the specification explicitly states:

> "The language definition does not specify how the program or its segments shall be named or how the segments are to be separated or terminated..."

Because the specification leaves the outer structural envelope completely undefined, compiler writers are forced to invent their own conventions for identifying the
start of a compilation unit.

Is it a reasonable implementation decision to mandate that a Coral 66 source file must start with a PROGRAM or SEGMENT declaration?

Yes, it is a highly reasonable and pragmatic decision.

By mandating a PROGRAM <name> or SEGMENT <name> declaration at the top of the file, coral66-ng achieves several things:
1. Unambiguous Parsing: It provides the parser with a clear, anchored entry point, completely eliminating ambiguity about whether the file represents a standalone
   program or a segment meant for separate compilation.
2. Historical Consistency: This approach aligns perfectly with the standard practices of other ALGOL-descended languages of the era (e.g., Pascal requiring program
   foo; or Fortran requiring PROGRAM foo), as well as the spec's own suggested "typical form".
3. Symbol Table Generation: It immediately provides the compiler with an identifier to name the resulting object file, executable, or internal module.

What are the drawbacks of this approach?

While reasonable from a compiler design standpoint, enforcing this strict header has a few drawbacks:

1. Portability of Legacy Code: Because the spec left this undefined, older historical Coral 66 compilers (e.g., for VAX, PDP-11, or the original RSRE compilers)
   often had their own unique pragmas, OS-specific directives, or simply allowed a file to start immediately with BEGIN or COMMON. Porting legacy code to coral66-ng
   will require manually modifying the source files to prepend the PROGRAM or SEGMENT keywords.
2. Boilerplate overhead: It forces developers to write boilerplate for every file. If you are writing a tiny three-line test script or benchmarking snippet, you
   still have to satisfy the outer file-structure syntax.
3. Tooling and Concatenation Friction: If a user or build system relies on #include-style text substitution or blindly concatenating multiple source files into a
   single stream before feeding them to the compiler, a strict "first-line must be PROGRAM" rule can break those workflows.
4. Limits Pure Block-Structure: Coral 66 is fundamentally a block-structured language where a program is essentially an outermost block (BEGIN ... END). Mandating a
   file-level wrapper conceptually separates the "file" from the "code block", which isn't strictly necessary from a theoretical language design perspective.

