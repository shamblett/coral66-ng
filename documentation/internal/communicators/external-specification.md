✦ To achieve linkage between Coral 66 procedures and external C libraries like libc, we must bridge Coral 66's abstract
semantics with LLVM's code generation and the system linker.

Since the Coral 66 specification allows for implementation-defined behaviour regarding the EXTERNAL 
communicator, here is a specification detailing how to achieve this, covering parsing, semantics, and code generation.

Transparent C-ABI Mapping via EXTERNAL

Map EXTERNAL procedures directly to the standard C Application Binary Interface (C ABI) for the target
platform (e.g., sysv-x64).

1. Parsing (lib/frontend/parser/)

* AST Structuring: Currently, communicators in syntax.cpp only hold a vector of token iterators. Update
  ExternalCommunicator to hold a structured AST node, such as
  Syntax::ExternalProcedureDeclaration.
* Parsing Logic: Update parseCommunicator in parser_communicators.cpp to process the EXTERNAL keyword. It will parse a
  Commonitemlist where the item is a
  Procedurespec (e.g., INTEGER PROCEDURE PrintInteger(VALUE INTEGER)). Unlike standard procedures, this lacks a
  Statement body.

2. Semantics (lib/frontend/semantics/)

* Symbol Registration: Introduce a Semantics::ExternalProcedureDeclaration class derived from Semantics::Useable.
* Semantic Analysis: In SemanticAnalysis::visit, add the parsed external procedure to the global symbol table. This
  ensures that a ProcedureCallStatement targeting
  PrintInteger is correctly type-checked against the declared signature (validating argument types and counts) just like
  an internal procedure.
* Bodyless Representation: m_body for this node type will remain empty.

3. Code Generation (lib/backend/code_generator.cpp)

* LLVM Declaration: When iterating over global declarations, if the generator encounters a Semantics::
  ExternalProcedureDeclaration, it uses llvm::Function::Create
  with llvm::GlobalValue::ExternalLinkage to emit a function declaration (without a BasicBlock body) into the LLVM
  Module.
* ABI Type Mapping: Map Coral 66 types to standard C ABI types during IR generation:
    * VALUE INTEGER $\rightarrow$ LLVM i32 or i64 (platform C int).
    * LOCATION INTEGER $\rightarrow$ LLVM i32* or i64* (C pointer).
    * FLOATING $\rightarrow$ LLVM float or double.
* Call Execution: The existing visit(const Semantics::CallExpression& expr) logic will naturally emit an llvm::CallInst
  pointing to this unresolved external LLVM
  function.

4. Command Line & Linkage (lib/main/)

* Update command_line_processor.cpp to parse standard linker switches like -l (e.g., -lc for libc) and -L (library
  paths).
* The compiler driver (action_executor.cpp) will pass these collected switches to the final linking step (e.g., lld or
  gcc/clang).

✦ In the above it should be noted that standard Coral 66 does not have a built-in mechanism (like a [DllImport("printf")] attribute or an ALIAS keyword) to map a Coral 66 identifier to a
different symbol name at the linker level.

However, you can achieve this mapping cleanly using a C-Side wrapper interface, example :-

wrapper.c:

1 #include <stdio.h>
2
3 // Expose the exact name the Coral 66 program expects
4 int PrintInteger(int a) {
5     return printf("%d\n", a);
6 }

Coral 66 Code:

1 EXTERNAL ( INTEGER PROCEDURE PrintInteger(VALUE INTEGER) );
2
3 BEGIN
4     PrintInteger(42)
5 END
Pros: Type safe, handles C varargs/format strings effortlessly, and requires no changes to coral66-ng. You just link the compiled wrapper.o with your Coral 66 object
file.



