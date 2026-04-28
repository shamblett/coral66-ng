Based on a thorough review of the TypeCheckingPass code implementation (lib/frontend/semantics/type_checking_pass.cpp) against the official Coral 66 language
specification, the following are the primary discrepancies and missing functionality that the type checking phase needs to support.

(Note: It is understood that certain implementations—like dataflow evaluation or constant bounds checking—might depend on the third pass, but the structural AST type
assignments below fall strictly within the scope of this semantics pass).

1. Anonymous References (Spec Section 6.1.1.2.1)
* Spec Requirement: The language allows anonymous word references in the form [ Index ], which evaluate the index as an address and take the implicit numeric type
  INTEGER.
* Current Deficit: In the AST, anonymous references lack an array identifier (e.g., m_array in ArrayAccessExpression is nullptr). TypeCheckingPass::visit(const
  syntax::ArrayAccessExpression&) strictly looks up the node in m_context.resolvedIdentifiers. For anonymous references, this lookup fails and it falls back to
  ErrorType{}, incorrectly triggering downstream TYPE_MISMATCH errors wherever an anonymous reference is used.

2. Table Element Accesses (Spec Section 4.4.4)
* Spec Requirement: Table elements are declared inside tables and can be accessed as standard identifiers or with a row index. They possess a defined numeric type
  (whole-word or part-word).
* Current Deficit: Neither visit(const syntax::IdentifierExpression&) nor visit(const syntax::ArrayAccessExpression&) checks
  usable->tryAs<TableElementDeclaration>(). Referencing any valid table element currently falls through the type-casting chains to assign ErrorType{}, completely
  breaking type checking for valid packed data access.

3. Array Dimensionality Checks (Spec Section 4.3)
* Spec Requirement: Array elements must be referred to with an index list providing exactly one index expression for each dimension defined in the array's
  declaration.
* Current Deficit: visit(const syntax::ArrayAccessExpression&) iterates over indices to ensure they are INTEGERs, but it never validates that
  node.getIndices().size() matches the array's declared dimension count (arrType->getDimensions().size()). Accessing a 2D array with 1 index currently passes type
  checking.

4. Array Parameter Matching (Spec Section 8.3.2.2)
* Spec Requirement: When passing an array to a procedure parameter, the spec explicitly states: "The numeric type of the actual array name must agree with this
  formal specification". The formal specification (e.g., INTEGER ARRAY) limits the base type, not the bounds or dimensionality.
* Current Deficit: During ProcedureCallStatement argument validation, reference parameters are strictly compared using *param.type == *itArg->second. Since
  ArrayType::equalsImpl checks both the primitive type and the dimensions vector, passing an array to a procedure will falsely emit a
  PROCEDURE_ARGUMENT_TYPE_MISMATCH if the caller and definition have differently sized bounds, violating the language's array parameter flexibility.

5. Enforcing FOR Loop Iteration Variables (Spec Section 7.10)
* Spec Requirement: The loop variable in a FOR statement must be a valid numeric data reference because step and until operations evaluate, scale, and cast to the
  numeric type of this reference.
* Current Deficit: visit(const syntax::ForStatement&) visits the loop variable but never asserts that the variable is actually a valid numeric type or a reference.
  Using an invalid assignment target (like a Procedure name or an Array base name) as the loop counter will currently bypass type-checking silently.

6. Place Parameters and Procedure Parameters (Spec Section 8.3.3 & 8.3.4)
* Spec Requirement: Coral 66 procedures can accept Labels (LABEL), Switches (SWITCH), and other Procedures as arguments.
* Current Deficit: The IdentifierExpression visitor does not map LabelDeclaration or SwitchDeclaration to their corresponding AST Types (LabelType, SwitchType),
  resolving them as ErrorType. Consequently, passing a valid label or switch into a procedure fails the isCompatible check during argument validation, blocking
  these control-flow patterns.

