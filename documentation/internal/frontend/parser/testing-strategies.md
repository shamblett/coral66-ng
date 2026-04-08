To verify that a parser produces a correct and complete Abstract Syntax Tree (AST) for whole programs—beyond isolating
specific rules in unit tests—compiler
engineers typically employ integration-level strategies. These strategies treat the parser as a black box that
transforms source text into a data structure,
verifying the fidelity and structural correctness of that transformation.

Here are the primary strategies you can use to validate whole-program AST generation for Coral 66:

1. AST Serialization and Snapshot Testing (Golden Files)
   This is the industry standard for production compilers (used heavily in LLVM/Clang, GCC, and Rust).

How it works:

* The Mechanism: You write an AST Visitor (e.g., an ASTDumper class) that traverses the entire AST and prints it to a
  structured, human-readable string format. This
  could be a custom Lisp-like S-expression, JSON, or YAML.
* The Test: You maintain a directory of complete, well-formed Coral 66 programs (.cor files) alongside "golden" expected
  output files (.ast or .expected).
* The Verification: The test suite parses the .cor file, passes the AST to the ASTDumper, and compares the resulting
  string to the .expected file. If they differ,
  the test fails, highlighting exactly how the tree structure deviated from expectations.

Why it's effective: It guarantees that the parser's output for an entire program doesn't change unexpectedly. If you
tweak the parser to fix an expression bug, the
snapshot tests will immediately fail if that tweak accidentally breaks loop parsing elsewhere in a full program.

2. Round-Trip "Unparsing" (Idempotency Testing)
   This strategy verifies that the AST captures the full semantic meaning of the original program without losing
   information.

How it works:

* The Mechanism: You write an ASTPrinter visitor that takes an AST and generates standard Coral 66 source code back out
  of it (essentially a code formatter or
  pretty-printer).
* The Test:
    1. Parse the original program.cor to AST_A.
    2. Pass AST_A through the ASTPrinter to generate program_generated.cor.
    3. Parse program_generated.cor to create AST_B.
* The Verification: Assert that AST_A is structurally identical to AST_B.

Why it's effective: You don't need to write manual assertions or golden files. You can feed your parser thousands of
legacy Coral 66 programs. If the parser is
dropping elements (like missing a STEP expression in a FOR loop) or misinterpreting precedence, AST_B will inevitably
look different from AST_A.

3. Structural Querying / Semantic Validation Passes
   Before feeding the AST to the actual Semantic Analyzer (which assigns types and checks for logical errors), you can
   write a "Sanity Check" visitor designed strictly
   for testing.

How it works:

* The Mechanism: A specialized AST visitor that enforces invariants that must be true for a well-formed AST, but might
  be too complex to express easily in parsing
  logic.
* The Verification: For example, it traverses a full program and verifies:
    * Every ProcedureDeclaration node has a matching number of parameter types to its identifiers.
    * No Block node contains a null statement pointer.
    * All PlaceReference switches have at least one label.
    * Source locations (line/column numbers) strictly increase as you traverse the tree left-to-right (ensuring tokens
      weren't mapped out of order).

4. Property-Based Testing (AST Fuzzing)
   Instead of writing Coral 66 programs by hand, you generate them algorithmically.

How it works:

* The Mechanism: Use a fuzzing library (or a custom generator based on the Coral 66 grammar) to generate random, deeply
  nested, but syntactically valid Coral 66
  ASTs in memory.
* The Verification: Use the ASTPrinter from Strategy 2 to convert the generated AST into source code. Then, parse that
  source code and verify the resulting AST
  perfectly matches the randomly generated AST.

Recommendation for coral66-ng

Given your current architecture (C++, Catch2, Bazel, and an existing Visitor pattern in the semantics phase), Strategy
1 (Snapshot Testing) is the most robust and
immediate next step.

Implementation Steps:

1. Create an ASTDumper class that implements a generic visitor interface for the Syntax namespace.
2. Have it output S-expressions. For example, INTEGER PROCEDURE p(VALUE INTEGER i); ANSWER i; becomes:

```

    1     (Program
    2       (ProcedureDeclaration name="p" type="Integer" recursive=false
    3         (Parameters 
    4           (Value mode="Value" type="Integer" identifiers=["i"])
    5         )
    6         (Block
    7           (AnswerStatement
    8              (WordReference "i")
    9           )
10         )
11       )
12     )
```

3. Add a Bazel test rule (often utilizing diff) that runs the parser over the examples in test/examples/ and compares
   the standard output against committed .golden
   files.

