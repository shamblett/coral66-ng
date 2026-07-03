I have analyzed the current state of lib/backend and mapped the requirements for Phase 1 (sieve.cor) to the existing C++ classes. The code generator currently uses
an AST visitor pattern (visit()) to translate semantic nodes into LLVM IR. Many of these visitor methods are incomplete or contain basic stubs.

To fully support Phase 1 (compiling and executing the Sieve benchmark), the following areas in lib/backend need to be updated:

1. code_generator.hpp & code_generator.cpp Updates

1.1. Array Support (ArrayDeclaration and ArrayAccess)
Currently, code_generator only handles VariableDeclaration and ProcedureDeclaration in its top-level generation loop and visitor. sieve.cor defines an array
('INTEGER' 'ARRAY' Flags [0 : SIZE]).
* Header (code_generator.hpp): Add visitor overrides:
    * void visit(const semantics::ArrayDeclaration& decl);
    * llvm::Value* visit(const semantics::ArrayAccess& expr);
* Implementation (code_generator.cpp):
    * Update generate() to forward ArrayDeclaration to the new visitor.
    * Update visit(const semantics::Declaration& decl) to handle ArrayDeclaration.
    * Update visit(const semantics::ExpressionBase& expr) dispatch to handle ArrayAccess.
    * Implement visit(ArrayDeclaration) to allocate LLVM Array Types (e.g., [8191 x i32]) via GlobalVariable (for globals) or alloca (for locals). Ensure bounds
      calculations account for 0-indexed vs non-zero indexed arrays (though Sieve uses [0 : SIZE]).
    * Implement visit(ArrayAccess) to emit a GEP (GetElementPtr) instruction to calculate the address, followed by a load (if R-value).
    * Update emitAddress() to handle ArrayAccess so that assignments (Flags[i] := ...) can get the correct L-value pointer.

1.2. Local Variable Scoping (code_generator::visit(BlockStatement))
* Implementation: The current implementation of visit(const semantics::BlockStatement& stmt) contains a warning: // Warning: currently locals are treated as globals
  by code generator.... This needs to be fixed.
    * When visiting a VariableDeclaration or ArrayDeclaration inside a BlockStatement (which means we are inside a function), it must generate an llvm::AllocaInst
      in the entry block of the current function (m_currentFunction) rather than a GlobalVariable.
    * Store these locals in the m_locals map so emitAddress and visit(DeclarationRead) can find them.

1.3. While Loop Support (code_generator::visit(ForStatement))
* Implementation: The current visit(ForStatement) stub only handles the 'STEP' ... 'UNTIL' structure (if (el.step && el.until)). Sieve uses a while loop: 'FOR' i :=
  i 'WHILE' k <= SIZE 'DO'.
    * Update visit(ForStatement) to detect and generate IR for while loops (el.whileCondition). This involves setting up condition, body, and exit basic blocks with
      appropriate branches.

1.4. Procedure Calls (CallExpression / ProcedureCallStatement)
* Implementation: Sieve calls a procedure (sieve()) and assigns the result (ans := sieve).
    * The visit(const semantics::ProcedureCallStatement& stmt) currently forwards to visit(stmt.getCallExpr()).
    * The visit(const semantics::CallExpression& expr) stub is incomplete (it just returns nullptr or relies on a stub).
    * Implement visit(CallExpression) to:
        1. Resolve the target function pointer/address.
        2. Evaluate and collect arguments (Sieve has 0 arguments, making this simple for Phase 1).
        3. Emit an llvm::CallInst via m_builder.CreateCall().
        4. Return the call instruction so it can be used in an assignment if needed.

1.5. Logical Expressions & Comparisons
* Implementation: The Sieve benchmark uses <= (k <= SIZE) and <> (Flags[i] <> 0).
    * Verify visit(const semantics::BinaryOperator& expr) correctly handles semantics::BinaryOperator::LessThanOrEqual (needs to emit CreateICmpSLE) and ensure the
      existing NotEqual (CreateICmpNE) handles integers cleanly.

2. ABI and Architecture

2.1. Basic Function Setup and Return (AnswerStatement)
* Implementation: Ensure the generateProcedure method and the visit(AnswerStatement) method correctly align with LLVM's expected function signatures. sieve.cor
  defines 'INTEGER' 'PROCEDURE' sieve; which returns a value.
    * Ensure AnswerStatement correctly terminates the basic block and issues an llvm::ReturnInst.
    * Ensure the main PROGRAM entry point maps cleanly to main() or an ABI-compatible entry function so the executable can be run and tested.

Summary of Actions for Phase 1
1. Add Array AST Visitors: Implement IR generation for ArrayDeclaration and ArrayAccess (including GEP generation in emitAddress).
2. Fix Local Variables: Change block-level declarations to use alloca instead of global variables.
3. Implement While Loops: Add IR generation for the WHILE variant of the FOR statement.
4. Implement Procedure Calls: Complete visit(CallExpression) to emit llvm::CallInst.
5. Complete Comparisons: Ensure <= is handled in the BinaryOperator visitor.

These are the required code modifications to support Phase 1. Once you confirm, we can proceed with implementation.

