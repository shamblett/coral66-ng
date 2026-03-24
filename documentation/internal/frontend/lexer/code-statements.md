In the 1970 "Official Definition of Coral 66" (Section 7.5), "inclusion of nested Coral text" meant that a CODE BEGIN ... END block was not intended to be an opaque wall of pure machine
code or assembly. Instead, the specification recommended that compilers allow programmers to interleave standard Coral 66 language constructs—such as referencing Coral variables,
evaluating Coral expressions, or even using Coral control flow (IF, FOR)—directly within the inline assembly.

The primary goal of this was interoperability: as the spec notes, "it must be possible to use any identifier of the program within the code statement". This allowed developers to write
assembly that directly interacted with Coral variables in scope, rather than forcing them to manually calculate memory addresses or stack offsets for those variables.

How CODE works in this implementation (coral66-ng)

You assumed that the body of a code statement in this implementation would comprise only hex numbers or identifiers. However, coral66-ng has actually diverged from the original 1970
grammar and simplified the syntax.

If you look at lib/frontend/parser/parser_statements.cpp and test/backend/code_generator_tests.cpp, the compiler does not use the BEGIN ... END block for CODE. Instead, it expects a
string literal:
1 CODE "nop";
In the backend (lib/backend/code_generator.cpp), this string literal is passed directly to LLVM as an llvm::InlineAsm call.

This means that the body of a CODE statement in coral66-ng is expected to be valid, target-dependent assembly instructions (like "mov eax, ebx" or "nop"), written as a string literal,
which LLVM's integrated assembler will then parse and emit. It is not limited to raw hex codes.

What other considerations are needed for CODE statements?

To fully align coral66-ng with the spirit of the original Coral 66 specification—and to make CODE statements safe and useful in a modern LLVM-based compiler—several major architectural
considerations need to be addressed:

1. Variable Binding (The "Nested Text" requirement):
   Currently, coral66-ng calls the LLVM inline assembly with zero arguments (m_builder.CreateCall(ft, ia, {})). It has no way to pass Coral variables into or out of the assembly string.
   To support the specification's requirement to use in-scope identifiers, the compiler needs a mechanism to map Coral variables to assembly registers or memory operands. This would likely
   resemble GCC's extended inline assembly (__asm__("..." : "=r"(out) : "r"(in))), but adapted for Coral's syntax.

2. Parsing Raw Text:
   If the project eventually wants to support the legacy CODE BEGIN ... END syntax, the lexer will need a special state to consume raw, unformatted assembly text without throwing syntax
   errors, stopping only when it encounters the END keyword.

3. Returning a Value:
   Section 7.5 states: "In some implementations, a code statement may be said to possess a value. The 'statement' may then be used as a primary in an expression". Currently, coral66-ng
   parses CODE strictly as a statement and the LLVM backend types it as returning void. The AST and code generator would need to be updated to treat it as an expression.

4. Clobbers and LLVM Optimizations:
   LLVM's optimizer aggressively assumes it knows the state of registers and memory. If a CODE statement modifies registers, memory, or control flow, the compiler must inform LLVM about
   these side effects (via "clobber lists"). Without this, the optimizer might corrupt the program state.

5. Portability:
   Assembly strings are inherently architecture-dependent. If coral66-ng targets multiple architectures (e.g., x86_64, ARM), CODE statements will instantly break portability unless paired
   with a macro system or conditional compilation (like an IFDEF) to provide different assembly strings for different targets.
