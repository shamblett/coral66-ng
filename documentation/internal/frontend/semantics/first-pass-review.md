I have completed a review of the current SymbolResolutionPass implementation against the "Official Definition of Coral 66" specification document.

While the first pass successfully establishes block hierarchies and tracks standard variables, arrays, and parameter lifecycles, there are several fundamental
scoping and resolution rules mandated by the spec that are currently missing or incorrectly implemented.

Here is the breakdown of missing or non-compliant functionality:

1. Implicit Label Declarations and Block Scoping
- Spec Mandate: Section 3.4 mandates that the scope of a label encompasses the entire smallest block (from BEGIN to END) embracing the statement where it is
  defined. Consequently, labels can legally be referenced (via GOTO) before they are explicitly set/defined in the text.
- Current Deficit: The SymbolResolutionPass traverses statements linearly and explicitly leaves label resolution to a future ControlFlowPass (per internal
  comments). However, in Coral 66, labels share the identifier namespace and can mask outer-scope variables. The resolution pass must perform a pre-scan of all
  Statement nodes within a Block to extract their getLabels() and register them into the symbol table before walking the block's body; otherwise, scope-shadowing
  behaves incorrectly and forward-referenced identifiers fail standard symbol resolution.

2. Recursive vs. Non-Recursive Procedure Scoping
- Spec Mandate: Section 3.5 enforces strict linear definition boundaries. A procedure identifier is only fully defined and accessible in scope after its entire
  declaration. A procedure is not permitted to call itself unless it is explicitly decorated with the RECURSIVE keyword.
- Current Deficit: SymbolResolutionPass::visit(const syntax::ProcedureDeclaration& node) eagerly registers the procedure’s identifier into the parent scope
  immediately before calling visit(node.getBody()). It entirely ignores the AST's node.isRecursive() flag. This flaw unconditionally permits all procedures to
  invoke themselves recursively and incorrectly allows the procedure's name to shadow outer identifiers inside its own body.

3. Missing Table Element Field Registration
- Spec Mandate: Section 4.4 defines that Table-element declarations (the structured fields within a table) are globally accessible data references exposed in the
  same scope as the table itself.
- Current Deficit: When processing a TableDeclaration, the pass registers the table's main identifier but entirely skips traversing node.getElements(). Because the
  underlying TableElementDeclaration nodes are never visited, their identifiers are never injected into the symbol table. Any downstream attempt to reference these
  elements (e.g., Field[Table_Index]) will erroneously crash with an UNDECLARED_IDENTIFIER_N diagnostic.

4. Ignoring COMMON and LIBRARY Communicators
- Spec Mandate: Sections 3.3 and 9.1 specify that COMMON communicators introduce labels, switches, variables, and procedures into the outermost environment scope
  prior to block execution, facilitating multi-segment program linking and cross-referencing.
- Current Deficit: visit(const syntax::Program& node) checks for and enters node.getBlock(), but completely ignores node.getCommunicator(). Consequently, any
  forward-declared COMMON data elements or LIBRARY procedures parsed from the AST are silently dropped. They never enter the global symbol table, resulting in
  widespread UNDECLARED_IDENTIFIER_N failures when the program segment attempts to use them.

5. SWITCH Declarations Completely Missing
- Spec Mandate: Section 5 states that a SWITCH provides a preset, unalterable array of labels scoped to the declaration.
- Current Deficit: This is primarily a broader frontend parser deficit, as there currently is no SwitchDeclaration AST node being emitted by the parser, meaning the
  SymbolResolutionPass has no visitor hook to register them. When implemented, the resolution pass must ensure that switches correctly register their array-like
  identifiers while binding to target labels within the active scope.

