Importation of code defined through the LIBRARY communicator is to be achieved via "Semantic Interface Extraction" (
Module Model).

The compiler should inject only the semantic symbols, and rely on separate compilation for the implementation. This
brings
Coral 66 closer to a modern module system (like Rust's mod or Java's import). Library code should be entirely contained
in external
Coral 66 files in a directory as indicated on the compiler command line.

Here is how this should be architected this across the compiler phases using the `LIBRARY myMaths` syntax :

1. Preprocessing Phase: Isolated Contexts

   When the compiler driver sees `LIBRARY myMaths`, it spawns an entirely new, isolated instance of the preprocessor to
   process myMaths.cor.
   
   This guarantees that the DEFINE macros used inside myMaths.cor to build the library do not pollute the global macro
   table (m_defines) of the file that
   imported it. The library's internals remain strictly encapsulated.


2. Parsing Phase: Separate ASTs

   The parser parses the preprocessed myMaths.cor into its own, independent Abstract Syntax Tree (Syntax::Program).

    In the main file, the parser simply parses `LIBRARY myMaths` and creates a Syntax::LibraryCommunicator node containing
    the identifier myMaths.


3. Semantic Phase: Symbol Table Injection

   When the SemanticAnalysis pass visits the LibraryCommunicator:
1. It queries a "Module Manager" for the AST of myMaths.cor.
2. It traverses the global declarations of the myMaths AST (looking for ProcedureDeclaration, CommonData, etc.).
3. It registers these global symbols into the current file's Symbol Table, but explicitly tags them as External (or
   creates a specialized
   LibraryProcedureDeclaration node).
4. Crucially: It ignores the m_body (the implementation statements) of the library procedures.


This gives the current file complete type-safety. If you call myMaths_Sin(), the semantic analyzer knows its exact
signature without needing the user to type it out,
but it doesn't clutter the current AST with the library's internal logic.


4. Code Generation (LLVM IR) :

   Because the imported symbols were tagged as External during the Semantic phase, the code_generator.cpp handles them
   elegantly:

   When it iterates over the symbols, it sees they are library imports.
   It emits an llvm::Function::Create with llvm::GlobalValue::ExternalLinkage (a declaration, e.g., declare i32
   @myMaths_Sin(i32)).

   It does not emit LLVM BasicBlocks or instructions for the body. This keeps the resulting object file perfectly lean.


5. Driver & Linker Phase (Action Executor) :

   The compiler driver (action_executor.cpp) should act as a smart build system:
1. It checks if myMaths.cor has already been compiled into myMaths.o (and if the object file is newer than the source).
2. If not, it spawns a background compiler thread to compile myMaths.cor to myMaths.o.
3. During the final linking stage, it automatically appends myMaths.o to the linker command line.
