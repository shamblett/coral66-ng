Based on a comprehensive review of the newly updated TypeCheckingPass in relation to the Coral 66 language specification, here are
the remaining deficiencies and omissions.

While the previously implemented L-value constraints, bounds-checking preparations, and parameter matching rules establish a robust
baseline, several core type-safety mechanisms are still missing.

1. BITS is Incorrectly Treated as a Interchangeable Numeric Type
   Spec Section 4.1
* Deficiency: The internal helper isNumericType() returns true for PrimitiveType::Bits. However, the Coral 66 specification
  strictly dictates: "Objects of type BITS are not numeric... The type BITS is used for logical or bitwise operations."
* Impact: Because BITS is bundled into generic numerics, the pass currently allows assigning a FLOATING value directly to a BITS
  array or mixing BITS and INTEGER variables inside arithmetic equations without explicit type conversion. Mixing BITS and
  arithmetic types should trigger a TYPE_MISMATCH unless explicitly cast via a TypeChangeExpression.

2. BinaryExpression Lacks Operator-Specific Type Constraints
   Spec Section 6.1.2
* Deficiency: visit(const syntax::BinaryExpression&) delegates type resolution to getCommonType() without introspecting the actual
  operator being used (node.getOp()->getTokenType()).
* Impact: Coral 66 strictly delineates between numeric operators (+, -, *, /) and bitwise/logical operators (DIFFER, UNION, MASK).
  The current implementation would happily allow 3.14 DIFFER 5.5 because both operands evaluate as "numeric", and it would
  incorrectly return FLOATING as the common type instead of asserting that logical operators strictly require INTEGER/BITS
  operands.

3. UnaryExpression Blindly Propagates Types Without Validation
   Spec Section 6.1.2
* Deficiency: visit(const syntax::UnaryExpression&) merely copies the type of its operand to itself
  (m_context.expressionTypes.insert_or_assign(&node, it->second)). It performs absolutely zero validation to ensure the operand is
  actually valid for the unary operator (e.g., -).
* Impact: Applying unary operations to control-flow symbols or non-numeric entities (e.g., -MyLabel) silently succeeds,
  propagating LabelType outwards and masking a profound TYPE_MISMATCH.

4. ForStatement Step and Until Clauses Evade Type Checking
   Spec Section 7.10
* Deficiency: While the previous update successfully restricted the ForStatement's loop counter variable to be numeric, the step
  and until constraint expressions are merely visited, but their resulting evaluated types are never checked.
* Impact: A user could write FOR I := 1 STEP MyProcedure UNTIL MyLabel DO ..., and the pass would silently accept it instead of
  enforcing that step/until boundaries must evaluate to purely numeric data types (EXPECTED_NUMERIC_TYPE).

5. TypeChangeExpression Does Not Validate Its Wrapped Expression
   Spec Section 6.1.1.2.2
* Deficiency: When executing a type change cast (e.g., INTEGER(expr)), the pass correctly validates the numeric indices (scale
  bits, etc.), but it fails to assert that the wrapped expr itself resolves to a valid, castable data type.
* Impact: Attempting to cast non-data entities like Labels, Switches, or untyped Procedures (e.g., FIXED(MyLabel)) will currently
  succeed with no diagnostics, "laundering" an invalid memory reference into a valid numeric type for downstream AST evaluations.

  ---

(Note on Control Flow Dependencies: Fixing the BITS constraints and BinaryExpression operator limitations does not require the
Control Flow pass and can be addressed strictly within the current semantics phase by expanding getCommonType() logic and adding
operator switch branches).

