The common communicator is implemented as a single common communicator declaration as per the specification that must be placed after the program declaration but before any
segment name and subsequent block declarations. Rationale for this is as below.

According to Section 2.2 ("Program") of the Official Definition of Coral 66:
> "The language definition does not specify how the program or its segments shall be named or how the segments are to be separated or terminated..."

The specification explicitly leaves the top-level structure of a compilation unit up to the implementation. It then provides a "typical form" as an example:

1 name of program
2 COMMON etc ;
3 segment name 1
4 BEGIN . . . END ;

By enforcing that a single COMMON communicator appears after the program declaration and before the outermost block, coral66-ng is choosing a direct, literal
interpretation of this "typical form" for its compilation units.

Drawbacks to this approach

While reasonable and compliant, enforcing a single COMMON communicator at the top of the file has a few practical drawbacks:

1. Reduced Logical Grouping: Developers are forced to lump all global definitions into one massive COMMON ( ... ) list. You cannot use multiple COMMON sections to
   logically separate different groups of state (e.g., separating I/O globals from mathematical globals).
2. Friction with File Inclusion/Macros: If you are using a preprocessor to include shared files across segments, you cannot easily #include multiple independent
   COMMON headers at the top level. Because there can only be one COMMON wrapper, you are forced to inject included fragments awkwardly inside the parentheses of a
   single COMMON list.
3. Monolithic Structure: For large files, keeping every globally accessible declaration in a single, continuous block at the very top can reduce readability and
   make the code harder to navigate.
4. Single-File Modularity: It restricts the ability to interleave segments and the specific communicators they depend on. In an implementation that allows multiple
   communicators, a developer might place a COMMON block right next to the specific segment that uses it.

In summary, it's a perfectly valid compiler design choice that simplifies parsing, but it trades away some organizational flexibility for developers working on large
codebases.
