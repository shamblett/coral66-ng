Based on a thorough review of the current TypeCheckingPass implementation against the Coral 66 language specification and the
existing semantic error definitions in lib/frontend/common/messages/errors.hpp, here is a report detailing the remaining
deficiencies and omissions.

Several checks are structurally missing from the AST traversal, while others rely on the deferred Control Flow pass for constant
evaluation.

1. L-Value Semantic Violations for Assignments
   Spec Section 7.1
* Deficiency: visit(const syntax::AssignmentStatement&) merely verifies that the left-hand side (LHS) and right-hand side (RHS)
  evaluate to compatible numeric types. It entirely fails to verify whether the LHS is actually an assignable memory location
  (L-value). This means assigning to a literal or an evaluated arithmetic expression (e.g., (A + B) := 10 or 5 := 1) will
  currently pass the type checker without error.
* Deficiency: The predefined diagnostic CANNOT_ASSIGN_TO_CONST is entirely unused. The SymbolResolutionPass explicitly tags VALUE
  parameters with Type::Location vs Type::Value flags. However, the assignment pass does not block modifications to variables
  carrying the Type::Value flag.

2. Location Parameter & Built-in Address Constraints
   Spec Section 8.3.1 & Built-ins
* Deficiency: When a procedure call matches a parameter defined strictly by location (param.isValue == false), visit(const
  syntax::FunctionCallExpression&) only checks for type matching. It does not enforce that the provided argument is a valid data
  reference (L-value). Consequently, passing an R-value like P(5 + 5) to a location parameter bypasses the type-checker. The
  LOCATION_PARAMETER_REQUIRES_LVALUE diagnostic exists for this exact scenario but is unimplemented.
* Deficiency: Similarly, the visit(const syntax::LocationExpression&) handler (representing the LOCATION(expr) operator) accepts
  any arbitrary expression instead of asserting that the operand represents a valid, addressable word reference.

3. Conditional Expression Condition Checks
   Spec Section 6.2
* Deficiency: While visit(const syntax::IfStatement&) successfully validates that its evaluation condition is of type INTEGER
  (Coral 66's truthy primitive), the visit(const syntax::ConditionalExpression&) logic (IF cond THEN a ELSE b) completely skips
  validating the condition. It only resolves the common bounds of the Then and Else branches, omitting the EXPECTED_INTEGER_TYPE
  diagnostic on the conditional clause.

4. Part-Word Type Verification
   Spec Section 6.1.1
* Deficiency: visit(const syntax::PartWordExpression&) (e.g., BITS i, j) visits the startBit and width expressions but never
  verifies their evaluated types. The pass must be updated to enforce that these internal expressions strictly evaluate to INTEGER
  types, emitting EXPECTED_INTEGER_TYPE otherwise.
* Deficiency (Deferred): The diagnostic INVALID_BIT_RANGE is unused. While verifying this requires resolving the expressions to
  compile-time constants (which falls under the purview of the upcoming Control Flow pass), the bounds check (e.g., ensuring
  startBit + width does not exceed the target machine's word scale) will need to be hooked in here.

5. Fixed-Point Scaling Validations
   Spec Section 4.1.2
* Deficiency: When resolving a FixedType scale—whether via visit(const syntax::TypeChangeExpression&) macros or variable
  declarations—the pass does not evaluate the bounds. The INVALID_SCALE_FOR_FIXED diagnostic exists but is never emitted.
* Deficiency (Deferred): Like Part-Word expressions, verifying that the fractional bits do not exceed the total bits relies on
  folding the scale indices into constants, which requires the Control Flow pass to be fully actualized. However, the expressions
  themselves (Total Bits and Fraction Bits) currently lack the basic EXPECTED_INTEGER_TYPE assertions.


