✦ To achieve linkage between Coral 66 procedures and external C libraries like libc, we must bridge Coral 66's abstract
semantics with LLVM's code generation and the
system linker.

Since the Coral 66 specification allows for implementation-defined behavior regarding EXTERNAL and LIBRARY
communicators, here is a proposal detailing two
alternative paths, covering parsing, semantics, and code generation.

Method 1: Transparent C-ABI Mapping via EXTERNAL (Recommended)

This approach maps EXTERNAL procedures directly to the standard C Application Binary Interface (C ABI) for the target
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

  ---

Method 2: Utilizing the LIBRARY Communicator (Semantic Alternative)

Section 9.2 of the Coral 66 specification states that LIBRARY communicators are explicitly intended for "objects whose
full definition lies outside the program, for
example library procedures." While EXTERNAL (Section 9.3) is usually reserved for cross-segment objects built within the
project, LIBRARY is semantically perfect for
libc.

1. Syntax & Parsing

* Since the spec says LIBRARY should "be styled to conform with the rest of the language," we can parse it similarly to
  COMMON:
  LIBRARY libc ( INTEGER PROCEDURE PrintInteger(VALUE INTEGER); )
* The parser captures both the library identifier (libc) and the Procedurespec.

2. Semantics & Code Gen

* The semantic and LLVM code generation steps remain identical to Method 1 (creating a GlobalValue::ExternalLinkage
  function).
* However, if coral66-ng ever implements a custom, non-C ABI for pure Coral-to-Coral segment linkage (e.g., passing
  array metadata bounds), LIBRARY cleanly
  separates standard C-ABI interoperability from EXTERNAL Coral 66 module linkage.

3. Command Line Automation

* Because the library identifier (e.g., libc, libm) is explicitly captured in the communicator, the compiler can
  automatically infer and append the -lc or -lm
  linkage flag during the action_executor linking phase, relieving the user from manually providing the -l switch on the
  command line.

  ---

Conclusion:
Implementing Method 1 offers the quickest path to achieving FFI (Foreign Function Interface) support using standard
linker CLI flags. However, expanding this logic
into Method 2 down the line offers a highly idiomatic Coral 66 experience while protecting the purity of the language's
native ABI.

Let me know if you would like me to proceed with implementing one of these approaches!

✦ In Method 1, standard Coral 66 does not have a built-in mechanism (like a [DllImport("printf")] attribute or an ALIAS keyword) to map a Coral 66 identifier to a
different symbol name at the linker level.

However, you can achieve this mapping cleanly using either standard Coral 66 language features or by extending coral66-ng. Here are the three ways to accomplish
this:

1. The Standard Coral 66 Way: DEFINE Macros
   You can use the Coral 66 macro preprocessor (which coral66-ng supports via the DEFINE keyword) to alias the C function name to your preferred Coral 66 identifier.

You simply declare the EXTERNAL procedure using the exact C symbol name (printf), and then DEFINE your preferred alias:

1 EXTERNAL ( INTEGER PROCEDURE printf(VALUE INTEGER) );
2
3 DEFINE PrintInteger < printf > ;
4
5 BEGIN
6     PrintInteger(42)  ! The preprocessor converts this to printf(42) before parsing
7 END
Pros: Requires zero modifications to the coral66-ng compiler. It relies entirely on standard language features.

2. The C-Side Wrapper Way
   Because printf is a variadic C function (int printf(const char *format, ...)), directly mapping it to INTEGER PROCEDURE printf(VALUE INTEGER) is actually dangerous
   at the ABI level (you are missing the format string!).

The safest and most common way to interoperate between Coral 66 and C is to write a tiny C wrapper file. This solves both the naming issue and the type-safety issue.

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

3. The coral66-ng Extension Way (Compiler Enhancement)
   If you want to build direct symbol aliasing directly into the compiler AST (bypassing the preprocessor or wrappers), we would need to extend coral66-ng.

I see that coral66-ng already has a foundational attribute system (lib/frontend/common/attributes.hpp), which includes things like DllImportAttribute and
AlignedAttribute. We could introduce a new AliasAttribute and extend the parser to accept it within communicators.

Proposed Syntax Extension:

1 EXTERNAL (
2     [ALIAS("printf")] INTEGER PROCEDURE PrintInteger(VALUE INTEGER)
3 );

Implementation Steps:
1. Add AliasAttribute: Add struct AliasAttribute { std::string targetName; }; to attributes.hpp.
2. Update Parser: Modify parser_communicators.cpp to parse square-bracketed attributes immediately preceding the Procedurespec.
3. Update Code Generator: In code_generator.cpp, when emitting the llvm::GlobalValue for the external declaration, check for the AliasAttribute. If it exists, pass
   targetName to llvm::Function::Create instead of the Coral 66 identifier.

  ---

Recommendation: For immediate results, Option 2 (C-Wrapper) is the best engineering practice because it safely bridges Coral 66 semantics with C's formatting and
variadic arguments. If you specifically want to avoid C-wrappers for strict 1:1 functions (like mapping abs() to abs()), Option 1 (DEFINE) works perfectly out of the
box.



