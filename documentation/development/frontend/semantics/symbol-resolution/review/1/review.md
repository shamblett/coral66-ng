Based on a thorough review of the SymbolResolutionPass implementation (lib/frontend/semantics/symbol_resolution_pass.cpp) against the official Coral 66 language
specification, here are the primary compliance issues and missing functionality.

(Note: While certain deeper validations like cross-procedural flow analysis belong to the ControlFlowPass or later code-generation stages, the fundamental symbol
tracking issues below strictly fall within the mandate of the first symbol resolution pass).

1. Communicator Declarations Ignored (Spec Section 9)
* Spec Requirement: Coral 66 programs and segments utilize Communicators (COMMON, LIBRARY, EXTERNAL, and ABSOLUTE) at the top level to declare externally linked or
  globally shared symbols.
* Current Deficit: SymbolResolutionPass::visit(const syntax::Program& node) strictly delegates down to node.getBlock() and entirely skips node.getCommunicator().
  Consequently, any variables, arrays, or symbols defined within a communicator are omitted from the global symbol table. References to these global symbols
  throughout the program will erroneously fail with an UNDECLARED_IDENTIFIER diagnostic.

2. Destruction of Parameter Data Types (Spec Section 8.3)
* Spec Requirement: Procedure parameters support various modes, including simple VALUE and LOCATION references, ARRAYs, TABLEs, LABELs, SWITCHes, and other
  PROCEDUREs.
* Current Deficit: SymbolResolutionPass::visit(const syntax::ParameterDeclaration& node) completely ignores the parameter's AST ParameterMode (which captures its
  original intent). Instead, the visitor aggressively coerces every parameter into a standard VariableDeclaration with a default Integer base type. Stripping
  parameters of their Array, Label, or Switch properties at the symbol level guarantees that subsequent type-checking or usage validation (e.g., trying to index an
  array parameter) will fail.

3. Switch Label Target Resolution (Spec Section 5)
* Spec Requirement: A switch declaration (SWITCH sw := l1, l2, l3;) assigns a list of program labels to a switch identifier.
* Current Deficit: visit(const syntax::SwitchDeclaration& node) registers the switch identifier correctly but ignores the right-hand label list (node.getLabels()).
  Because labels are correctly pre-scanned during block entry, their declarations exist in the symbol table by the time the switch declaration is visited. The pass
  should cross-reference these right-hand identifiers against the symbol table to ensure they resolve to valid LabelDeclarations in scope, rather than allowing
  invalid/undeclared labels to slip through.

4. Overlay Base Verification (Spec Section 4.8)
* Spec Requirement: An OVERLAY declaration is used to map a new data structure over an existing, previously declared data location (the base reference).
* Current Deficit: visit(const syntax::OverlayDeclaration& node) correctly descends into and registers the new overlaid structure, but it never visits or resolves
  node.getBase(). The pass fails to check the symbol table to confirm that the base identifier has actually been declared earlier in the scope hierarchy.

5. Part-Word Table Element Erasure (Spec Section 4.4.2.2)
* Spec Requirement: Table elements are intricately structured. Part-word elements specify explicit bit counts and are implicitly treated as unsigned integers unless
  a signed explicit type is given.
* Current Deficit: While the pass properly maps elements as TableElementDeclarations, it relies on a naive token-to-primitive conversion, falling back to a standard
  PrimitiveType::Integer for anything without a type keyword. Even though explicit bounds math and bit calculations are a Code Generation/Pass 3 concern, throwing
  away the bit-level sizing constraints and unsigned nature at the symbol definition layer deprives the TypeCheckingPass of the necessary fidelity required to check
  assignment bounds.
