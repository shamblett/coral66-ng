# Specification: CODE Statements in `coral66-ng`

## 1. Overview
Section 7.5 of the Official Definition of Coral 66 states that `CODE` statements should provide for the inclusion of nested Coral text, allowing programmers to use in-scope program identifiers within inline assembly. 

To align with modern LLVM compiler infrastructure while providing an intuitive developer experience, `coral66-ng` will implement a variable binding syntax using string literals. This allows standard Coral 66 variables to be mapped to LLVM assembly operands without requiring manual register manipulation or compromising compiler optimizations.

## 2. Syntax & Semantics
The `CODE` statement takes a string literal representing the target-specific assembly instructions.

### Variable Binding
Programmers use special prefix characters within the assembly string to bind Coral 66 variables:
*   **Input Binding (`@`):** Reads the value of a Coral variable into the assembly block. Example: `@v1`
*   **Output Binding (`!`):** Writes a value from the assembly block back to a Coral variable. Example: `!v1`

### Example
```coral
PROGRAM InlineAsmDemo
BEGIN
  INTEGER a, b, result;
  a := 10;
  b := 20;
  
  COMMENT 
    Adds 'a' and 'b', storing in 'result'. 
    Using x86 assembly for this example.
  ;
  CODE "mov eax, @a \n add eax, @b \n mov !result, eax";
END
```

---

## 3. Frontend Impacts

### 3.1. Lexer (`lib/frontend/lexer/`)
**Impact: None.**
The lexer already correctly tokenizes the `CODE` keyword and string literals (`lexer::TokenType::CodeKeyword`, `lexer::TokenType::StringLiteral`). No changes are required.

### 3.2. Parser (`lib/frontend/parser/`)
**Impact: Minimal.**
The parser (`parser_statements.cpp`) currently constructs a `Syntax::CodeStatement` containing the raw string literal. This behavior remains correct. 

The grammar remains:
`CODE BEGIN "assembly string" END;` as per the specification section 7.5.

### 3.3. Semantic Analysis (`lib/frontend/semantics/`)
**Impact: High.**
The semantic analyzer (`semantic_analysis.cpp` and `semantics.hpp`) must be updated to process the raw assembly string and resolve variable bindings.

1.  **String Scanning:** When visiting a `Syntax::CodeStatement`, the analyzer scans the string literal for occurrences of `@identifier` and `!identifier`.
2.  **Symbol Resolution:** For each `identifier` found, the analyzer looks it up in the active `m_symbolTable`.
    *   If the identifier is not found, a semantic error (e.g., `UNDEFINED_IDENTIFIER`) must be emitted.
    *   If found, the `Declaration` pointer is recorded.
3.  **String Translation:** The analyzer rewrites the assembly string for LLVM. LLVM expects positional placeholders (`$0`, `$1`, `$2`, etc.).
    *   The first bound variable (whether input or output) becomes `$0`, the second `$1`, and so on.
    *   Example: `"mov eax, @a \n add eax, @b \n mov !result, eax"` translates to `"mov eax, $0 \n add eax, $1 \n mov $2, eax"`.
4.  **AST Update:** The `Semantics::CodeStatement` class must be updated to store:
    *   `m_llvmCode`: The translated string (e.g., `"mov eax, $0 ..."`).
    *   `m_inputs`: A `std::vector<const Semantics::DeclarationRead*>` or equivalent list of variable inputs.
    *   `m_outputs`: A `std::vector<const Semantics::Declaration*>` of variable outputs.

---

## 4. Backend Impacts (Code Generation)

### 4.1. Code Generator (`lib/backend/code_generator.cpp`)
**Impact: High.**
The backend's `visit(const Semantics::CodeStatement& stmt)` must be completely overhauled to support LLVM `InlineAsm` arguments and constraints.

1.  **Argument Resolution:** Iterate through `m_inputs` and `m_outputs` to gather the corresponding `llvm::Value*` pointers using `emitAddress()` or by reading local/global maps.
    *   Inputs: Load the value from the variable's address (using `m_builder.CreateLoad`).
    *   Outputs: Retrieve the pointer (address) of the variable to store the result.

2.  **Constraint String Construction:** LLVM requires a constraint string to map the `$N` placeholders to physical CPU constraints.
    *   For each Input: Append `"r"` (bind to a general-purpose register).
    *   For each Output: Append `"=r"` (if returned) or `"*m"`/`"=*r"` (if passed as a pointer). *Implementation note: Returning multiple values via LLVM struct extraction is often cleaner than using pointer constraints for inline assembly, but passing pointers with `*m` is also valid.*
    *   **Memory Clobber (CRITICAL):** The constraint string **MUST** end with `,~{memory}`. This informs LLVM's optimizer that the assembly block may read or write arbitrary memory, forcing LLVM to flush cached registers to the stack before execution and reload them afterward. This guarantees that `CODE` statements do not corrupt LLVM's optimization state.
    *   *Example Constraint String:* `"r,r,=*m,~{memory}"`

3.  **Function Type and Call:** 
    *   Construct the `llvm::FunctionType` based on the types of the inputs and outputs.
    *   Call `llvm::InlineAsm::get(functionType, stmt.getLlvmCode(), constraints, true /* hasSideEffects */)`.
    *   Emit the call instruction using `m_builder.CreateCall(..., {args...})`.

### Example LLVM IR Output
For the previous example, the generated LLVM IR will logically resemble:
```llvm
%a_val = load i32, ptr %a
%b_val = load i32, ptr %b
call void asm sideeffect "mov eax, $0 \0A add eax, $1 \0A mov $2, eax", "r,r,*m,~{memory}"(i32 %a_val, i32 %b_val, ptr %result)
```

## 5. Security & Safety
By delegating register preservation to LLVM via the `~{memory}` clobber and standard constraints (`r`, `=r`), `coral66-ng` avoids stack corruption (e.g., overwriting the Red Zone) and prevents stale register reads. Programmers are explicitly discouraged from using manual `pusha`/`popa` instructions within their `CODE` statements.
