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


Taking point 1 for variable binding could a simple method of substitution be adopted for code variables, example the code string literal "mov eax @v1" could be come "mov eax 1" if there
existed in the Coral code a variable named v1.                                                                                                                                             

Yes, adopting a simple substitution syntax like @v1 is an excellent and very intuitive way to solve the variable binding problem from the programmer's perspective. However, how it is
implemented under the hood in coral66-ng (which uses LLVM) requires a slight distinction between compile-time text substitution and runtime register allocation.

Here is why pure text substitution is tricky, and how LLVM actually expects you to implement your exact idea.

The Limitation of Pure Text Substitution
If you literally manipulate the string at compile time to replace "mov eax, @v1" with "mov eax, 1", this will only work if v1 is a constant known at compile time.

If v1 is a standard variable that changes during the program's execution, its value isn't 1 when the compiler creates the binary; its value will live in a CPU register or on the stack at
runtime. Therefore, the assembly needs to know where v1 is located, not what its value happens to be at compile time.

The "LLVM Way" to Implement Your Idea
LLVM has built-in support for exactly the kind of substitution you are suggesting, but it uses placeholders and constraints instead of direct text replacement.

To make "mov eax, @v1" work, the compiler frontend would need to act as a translator:

1. Scanning the String: During semantic analysis, the compiler scans the CODE string literal for the @ symbol.
2. Symbol Lookup: When it finds @v1, it looks up v1 in the Coral symbol table to ensure it is in scope and gets its internal LLVM representation (llvm::Value*).
3. Rewriting for LLVM: The compiler rewrites your string, replacing @v1 with an LLVM positional placeholder (like $0). The string becomes "mov eax, $0".
4. Passing the Variable: When the code generator creates the llvm::InlineAsm instruction, it passes the string "mov eax, $0" and the llvm::Value* for v1 as an argument.

Here is roughly what the C++ code generation update would look like to support your idea:

    1 // 1. You give LLVM the rewritten string with a placeholder ($0)
    2 // 2. You specify constraints. "r" means "put this in a general-purpose register"
    3 llvm::InlineAsm* ia = llvm::InlineAsm::get(
    4     functionType, 
    5     "mov eax, $0", // Rewritten from "mov eax, @v1"
    6     "r",           // Constraint: put argument 0 into a Register
    7     true           // Has side effects
    8 );
    9
10 // 3. You pass the LLVM value of v1 into the call
11 llvm::Value* v1_llvm_value = m_locals[v1_decl];
12 m_builder.CreateCall(functionType, ia, { v1_llvm_value });

Why this approach is powerful
By translating @v1 into LLVM's operand system instead of just replacing it with text:
* LLVM handles the architecture: You don't need to know if v1 is currently in the stack or a register. LLVM's register allocator will automatically insert instructions to move v1 into
  the correct register before your assembly block runs.
* You can support inputs and outputs: You could easily extend your @ syntax. For example, reading from a variable could be @v1 (input), and writing to a variable could be !v1 (output).
  LLVM handles piping the result of the assembly block back into the Coral 66 variable.

In short: Your @v1 idea is the perfect design for the language syntax. To make it work in coral66-ng, you just need to write a quick string parser that converts your @v1 tags into LLVM
$0, $1 placeholders and passes the corresponding variables to the CreateCall function.

Taking point 4 for clobbers and optimisations would it be sufficient to save all working registers on entry to a CODE statement and restore them at the end, i.e. as far as LLVM is       
concerned only working memory may have been altered.                                                                                                                                      

While saving and restoring all working registers (e.g., wrapping the assembly in pusha and popa) sounds like a foolproof way to prevent register corruption, it is not sufficient for LLVM,
and it actually introduces several dangerous side effects.

Here is why manually saving and restoring registers inside the CODE block doesn't give LLVM the guarantees it needs, and how it directly conflicts with modern compiler optimizations:

1. The "Memory Clobber" Problem (The biggest issue)
   You correctly noted that "as far as LLVM is concerned only working memory may have been altered." However, LLVM assumes memory has not been altered unless you explicitly tell it.

If LLVM is executing a loop or a sequence of statements, it will heavily optimize by reading variables from memory into CPU registers, and keeping them there.
* If your CODE statement modifies a variable in memory...
* ...but LLVM already has an old copy of that variable cached in a register...
* ...LLVM will continue to use the stale register value after your CODE statement finishes, completely ignoring the updated memory.

To fix this, you don't need to save registers; you need to add a Memory Clobber constraint ("~{memory}") to the InlineAsm call. This forces LLVM to flush all cached registers to memory
before your assembly block, and reload them from memory after your assembly block.

2. Conflict with Variable Binding (@v1)
   If you implement the @v1 variable binding we discussed, LLVM will set up the input and output variables by placing them in specific registers right before your assembly executes.

If you blindly push all registers to the stack at the start of your code, and pop them all at the end:
* Any output you wrote to a register (e.g., returning a value to @v1) will be immediately overwritten and destroyed by your pop instructions at the end of the block. LLVM will read the
  restored (old) value instead of your new one.

3. The "Red Zone" and Stack Corruption
   On modern architectures (especially x86-64 System V ABI, which is standard on Linux/macOS), compilers use an optimization called the Red Zone. This allows the compiler to store temporary
   variables in the 128 bytes of memory below the current Stack Pointer (rsp), without actually moving the Stack Pointer.

If your inline assembly blindly uses push to save registers, the Stack Pointer will move down and overwrite the compiler's Red Zone, silently corrupting local variables that LLVM placed
there.

4. It Defeats LLVM's Register Allocator
   Pushing and popping all registers is computationally expensive. One of the main reasons for using inline assembly is performance or precise hardware control.

The "LLVM Way" to Handle Clobbers

Instead of manually saving and restoring registers in your assembly string, you should let LLVM do it for you.

LLVM allows you to provide a "Clobber List" when you create the InlineAsm call. You simply tell LLVM which registers your assembly plans to overwrite (e.g., "~{eax},~{ecx},~{memory}").

When LLVM sees this clobber list, its register allocator works magic:
1. It looks at what it currently has stored in eax and ecx.
2. It safely moves those values to other unused registers or saves them to the stack before your assembly runs.
3. It lets your assembly run and freely destroy eax and ecx.
4. It restores them seamlessly afterward.

Summary for coral66-ng
To safely implement CODE statements with optimizations enabled:
1. Never manual push/pop: Don't try to hide register usage from the compiler.
2. Explicit Clobbers: If the syntax allows the programmer to specify which registers they use, pass those to LLVM (e.g., "~{eax}").
3. The Nuclear Option (Safest Default): If you don't know what the programmer's CODE block does, you must append "~{memory}" to the constraints to ensure LLVM doesn't use stale cached
   variables. If you truly want to be safe without parsing the assembly, you would tell LLVM that every caller-saved register is clobbered, forcing LLVM to save the exact state itself,
   rather than doing it blindly inside the assembly string.

