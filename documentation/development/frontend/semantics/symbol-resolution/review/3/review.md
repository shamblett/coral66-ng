I have reviewed the updated first-pass SymbolResolutionPass logic against the official Coral 66 specification. The pass is now highly compliant for baseline symbol
scoping and identifier binding.

The implementation successfully handles block scoping lifetimes (Spec 7.6), part-word reference base resolution, array/table dimensions deferred evaluation, overlay
base resolution, and GOTO scoping limits (preventing jumps into inner blocks).

However, bearing in mind that we are looking at Pass 1, there are a few significant compliance issues and architectural quirks remaining that will need to be
addressed either through an additional pass or down-stream phases:

1. Forward Procedure Calls (Spec 7.1 & 8.1) - Major Compliance Issue
   The Coral 66 specification states, "There is no restriction on the order of declarations" and "Procedures may be called before they are declared".
   Currently, SymbolResolutionPass::visit(const syntax::Block& node) visits declarations synchronously. When a ProcedureDeclaration is visited, it immediately parses
   its body statements. If Procedure A calls Procedure B (which is defined later in the same block), the symbol lookup for B will fail during A's resolution and yield
   an UNDECLARED_IDENTIFIER_N.
* Required Fix: The compiler needs a true two-phase block scan: a preScanDeclarations (similar to what you already do for labels) that registers all
  procedures/variables into the SymbolTable before descending into any of the procedure bodies.

2. Switch Target Resolution (Spec 5.3) - Pending Implementation
   When a SWITCH declaration is processed, the pass successfully validates that all target labels exist in the current scope. However, because visit processes
   statements after declarations, the semantic LabelStatements haven't been generated yet. As a result, the AST semantics::SwitchDeclaration is instantiated with an
   empty std::vector<const LabelStatement*>{} array.
* Required Fix: The actual mapping of the switch indices to their control-flow blocks will need to be resolved in a subsequent Control Flow or Graph pass (or
  delayed until Code Generation), as Pass 1 discards the mapping.

3. Procedure Parameters as Variables (Spec 8.4) - Architectural Quirk
   During procedure signature evaluation, formal parameters are captured perfectly into the ProcedureType. However, when mapping those parameters into the procedure's
   internal SymbolTable scope, all parameters are instantiated as semantics::VariableDeclarations.
   For VALUE or LOCATION types, this is accurate. But for ARRAY or PROCEDURE modes, they are registered as VariableDeclarations possessing an ArrayType or
   ProcedureType.
* Impact: This is structurally sound if Code Generation treats parameter references simply as variables holding array/function pointers. However, if downstream
  passes rely on RTTI/casting specifically to semantics::ArrayDeclaration or semantics::ProcedureDeclaration, this will fail.

4. Presets and Constants (Spec 4.6) - Deferred
   Variable and Array presets (e.g., INTEGER i <- 1) are bypassed entirely during Symbol Resolution because getPresetTokens() defers them as raw lexer tokens. While
   this correctly delegates constant-expression evaluation to Pass 2 or 3 (where type context is fully available), any identifiers hypothetically referenced within a
   preset expression will not be tracked in resolvedIdentifiers during Pass 1. Given Coral 66's strict rules around constants, this is likely safe, but worth noting.

