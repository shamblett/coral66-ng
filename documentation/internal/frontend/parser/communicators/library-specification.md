✦ While the "in-place textual inclusion" method you described is conceptually similar to C's #include, it has several significant drawbacks when applied to a modern
compiler backend like LLVM.

If you just copy the text of myMaths.cor into the source file, you will run into:
1. Macro Leakage: Any DEFINE macros in myMaths.cor will leak into the caller's file, potentially causing silent and catastrophic name collisions.
2. Linker Conflicts (ODR Violations): If two different files in your project both say LIBRARY myMaths, the LLVM code generator will emit the implementation of
   myMaths into both object files. The linker will then fail with "multiple definition" errors.
3. Compilation Bloat: You are re-parsing, re-checking, and re-compiling the entire library every time it is imported.

The Better Way: "Semantic Interface Extraction" (Module Model)

Instead of injecting the source text, the compiler should inject only the semantic symbols, and rely on separate compilation for the implementation. This brings
Coral 66 closer to a modern module system (like Rust's mod or Java's import).

Here is how you would architect this across the compiler phases:

1. Preprocessing Phase: Isolated Contexts
   When the compiler driver sees LIBRARY myMaths, it spawns an entirely new, isolated instance of the preprocessor to process myMaths.cor.
* Why? This guarantees that the DEFINE macros used inside myMaths.cor to build the library do not pollute the global macro table (m_defines) of the file that
  imported it. The library's internals remain strictly encapsulated.

2. Parsing Phase: Separate ASTs
   Instead of merging token streams, the parser parses the preprocessed myMaths.cor into its own, independent Abstract Syntax Tree (Syntax::Program).
* In the main file, the parser simply parses LIBRARY myMaths and creates a Syntax::LibraryCommunicator node containing the identifier myMaths.

3. Semantic Phase: Symbol Table Injection
   This is where the magic happens. When the SemanticAnalysis pass visits the LibraryCommunicator:
1. It queries a "Module Manager" for the AST of myMaths.cor.
2. It traverses the global declarations of the myMaths AST (looking for ProcedureDeclaration, CommonData, etc.).
3. It registers these global symbols into the current file's Symbol Table, but explicitly tags them as External (or creates a specialized
   LibraryProcedureDeclaration node).
4. Crucially: It ignores the m_body (the implementation statements) of the library procedures.

This gives the current file complete type-safety. If you call myMaths_Sin(), the semantic analyzer knows its exact signature without needing the user to type it out,
but it doesn't clutter the current AST with the library's internal logic.

4. Code Generation (LLVM IR)
   Because the imported symbols were tagged as External during the Semantic phase, the code_generator.cpp handles them elegantly:
* When it iterates over the symbols, it sees they are library imports.
* It emits an llvm::Function::Create with llvm::GlobalValue::ExternalLinkage (a declaration, e.g., declare i32 @myMaths_Sin(i32)).
* It does not emit LLVM BasicBlocks or instructions for the body. This keeps the resulting object file perfectly lean.

5. Driver & Linker Phase (Action Executor)
   Finally, to make the user experience seamless, the compiler driver (action_executor.cpp) acts as a smart build system:
1. It checks if myMaths.cor has already been compiled into myMaths.o (and if the object file is newer than the source).
2. If not, it spawns a background compiler thread to compile myMaths.cor to myMaths.o.
3. During the final linking stage, it automatically appends myMaths.o to the linker command line.

Summary of Benefits

By shifting from "Textual Inclusion" to "Semantic Interface Extraction", you achieve the exact user experience you wanted (LIBRARY myMaths magically makes the
library available) but with massive engineering advantages:
* Zero Linker Errors: The library implementation is compiled exactly once.
* True Encapsulation: Macros and local variables in the library cannot corrupt the calling code.
* High Performance: Library ASTs can be cached by the compiler driver so they aren't repeatedly parsed across a large project.
