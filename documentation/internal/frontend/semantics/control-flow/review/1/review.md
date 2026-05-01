Based on a thorough review of the newly implemented ControlFlowPass alongside the existing SymbolResolutionPass and TypeCheckingPass, here is the compliance report.

While the new ControlFlowPass establishes excellent constant folding and successfully enforces literal bounds constraints, it currently acts more like a "Constant
Validation Pass" rather than a true Control Flow Graph (CFG) analyzer. Furthermore, the introduction of constant evaluation exposes gaps in the prior passes that can
now be closed.

Here are the deficiencies and omissions that need to be corrected for full specification compliance:

1. Deficiencies in the Control Flow Pass Implementation

A. Array and Table Declaration Bounds Checks (Spec Section 4.3 & 4.4.1)
* Omission: The pass correctly intercepts ArrayAccessExpression to check against bounds, but it completely omits visiting ArrayDeclaration and TableDeclaration.
* Impact: The compiler does not verify that an array's lower bound is actually less than or equal to its upper bound (e.g., INTEGER ARRAY A[10:1] silently passes).
  Similarly, it fails to ensure a Table's width and length evaluate to strictly positive integers (> 0).
* Fix Required: Add visit(const syntax::ArrayDeclaration&) and visit(const syntax::TableDeclaration&) to evaluate and validate declaration constraints, emitting
  diagnostics like INVALID_ARRAY_BOUNDS.

B. GOTO Scope Validation (Block Boundaries) (Spec Section 5.2)
* Omission: The Coral 66 specification strictly dictates: "A goto statement must not lead from outside into a block..." The current CF pass merely visits the index
  of a GOTO statement.
* Impact: A user can currently GOTO a label defined deeply inside a nested BEGIN ... END block from an outer scope.
* Fix Required: The pass needs to compare the lexical block scope depth of the GOTO statement against the resolved scope depth of the target LabelDeclaration,
  emitting an ILLEGAL_BLOCK_JUMP if the target is deeper than the origin.

C. Switch Statement Index Bounds (Spec Section 5.2.2)
* Omission: When executing GOTO sw[idx], the pass checks that idx is an integer (in TypeCheck) but does not verify the index limits.
* Impact: If idx evaluates to a constant, the pass fails to verify that 1 <= idx <= N (where N is the number of labels in the SwitchDeclaration).
* Fix Required: In visit(GotoStatement), if the target is a Switch and the index evaluates to a constant, validate it against the switch's element count.

D. Comprehensive Return Path Analysis (Spec Section 8.3)
* Omission: The MISSING_ANSWER_STATEMENT check is currently a naive flag (m_hasAnswer) that trips if any ANSWER statement is visited in the AST hierarchy.
* Impact: A procedure like INTEGER PROCEDURE P; BEGIN IF 1=2 THEN ANSWER 1 END will pass because an ANSWER node exists in the AST, even though the default control
  flow falls through the block without returning a value.
* Fix Required: The CF pass needs a basic path-reachability tracker (e.g., tracking "does this branch unconditionally terminate?") to ensure all outgoing execution
  paths in a typed procedure end in an ANSWER.

E. Unreachable Code Detection (Good Compiler Practice / Implicit in Spec)
* Omission: There is no reachability analysis.
* Impact: Code appearing sequentially after an unconditional GOTO or ANSWER statement (without an intervening label) is completely unreachable but is silently
  parsed and typed.

  ---

2. Deficiencies Exposed in Symbol Resolution Pass

A. Deferred Array and Table Types
* Omission: In SymbolResolutionPass::visit(ArrayDeclaration) and TableDeclaration, the types are constructed with dummy dimensions (e.g. std::vector<std::uint64_t>
  dims(node.getDimensions().size(), 0);).
* Impact: The Type classes (ArrayType, TableType) are still walking around with dummy 0 sizes.
* Fix Required: Now that evaluateConstantInteger logic exists, it should be extracted into SemanticContext or a SemanticUtilities helper. The SymbolResolutionPass
  should use it to evaluate and bake the actual numeric dimensions directly into the ArrayType and TableType instantiations so the structural type definitions are
  accurate for downstream LLVM generation.

  ---

3. Deficiencies Exposed in Type Checking Pass

A. PRESET Element Count Validation (Spec Section 4.5)
* Omission: PRESET blocks are used to initialize arrays and tables. The TypeCheckingPass does not currently validate that the number of provided PRESET expressions
  matches the total capacity of the array or table.
* Impact: A user can assign 50 preset constants to an INTEGER ARRAY A[1:10], causing buffer overflows during code generation.
* Fix Required: Utilizing the newly available constant evaluated dimensions (either from the CF evaluator or baked into the ArrayType),
  TypeCheckingPass::visit(ArrayDeclaration) and visit(TableDeclaration) must verify that PRESET element counts strictly equal (or do not exceed) the statically
  evaluated size of the memory block, emitting INCORRECT_PRESET_COUNT.

Summary of Action Items

To achieve full structural and logical specification compliance, you should:
1. Extract evaluateConstantInteger into a shared utility.
2. Update SymbolResolutionPass to bake evaluated constants into ArrayType and TableType.
3. Update TypeCheckingPass to validate PRESET list sizes.
4. Expand ControlFlowPass to evaluate ArrayDeclaration lower/upper rules, Switch bounds, prevent invalid block jumps, and perform true path-based ANSWER return
   analysis.


