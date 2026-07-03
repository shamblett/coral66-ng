✦ While decomposing the implementation by syntax categories (Expressions, Declarations, Statements) works excellently
for a top-down recursive descent parser, it is
generally an anti-pattern for Semantic Analysis.

In fact, looking at the project's recent git history, stub files for this exact scheme (
semantic_analysis_expressions.cpp, etc.) were recently removed because they
didn't fit the natural flow of the semantics phase.

Here is an analysis of why the parser's scheme doesn't map well to semantics, followed by a proposed architecture that
does.

Why Syntax-Based Decomposition Fails for Semantics

1. Context-Free vs. Context-Sensitive

* Parsing is mostly context-free. parser_expressions.cpp only needs the token stream to build an expression tree. It
  doesn't care about the surrounding program.
* Semantics is deeply context-sensitive. An AssignmentStatement (Statements) needs to evaluate the type of its RHS (
  Expressions) and look up the LHS in the Symbol
  Table (Declarations). Splitting these across files creates tight, invisible dependencies because they constantly rely
  on shared state to communicate.

2. The "God Class" and Shared State
   If you split SemanticAnalysis::visit() methods across multiple files, they must all belong to the same
   SemanticAnalysis class to share access to the symbol table,
   scope managers, and error reporters.

* Header Bloat: Every time you need a small helper function for expressions, it has to be declared in the main
  semantic_analysis.hpp header.
* Recompilation: Changing one private helper for expressions in the header will force a recompilation of all semantic
  analysis .cpp files.
* Temporal Coupling: Currently, expressions pass their results back to statements via a shared class member (
  m_lastExpression). Spreading this side-effect-driven
  logic across multiple files makes debugging very difficult.

3. Tangling of Concerns
   Semantic analysis isn't a single task. It involves Name Resolution, Type Checking, Constant Folding, and Control Flow
   Validation. If you create
   semantics_expressions.cpp, that file will become a messy mixture of all these separate concerns applied to
   expressions.

  ---

A Better Scheme: Decomposition by Responsibility / Pass

Instead of decomposing horizontally by Syntax (What part of the language is it?), semantic analysis should be decomposed
vertically by Responsibility (What question
are we answering?).

For a language like Coral 66, a Pass-Based or Component-Based architecture is highly recommended.

Scheme 1: Multi-Pass Architecture (Recommended)
You decompose the semantic processor into separate, focused classes that walk the AST, each living in its own .hpp/.cpp
pair:

1. SymbolResolutionPass (symbol_resolution.cpp)
    * Goal: Walk the AST to identify all scopes, declarations, and map identifiers to their definitions.
    * Output: A populated Symbol Table attached to the AST or Semantic Context.
2. TypeCheckingPass (type_checking.cpp)
    * Goal: Walk the AST to infer the types of expressions and ensure strict type compatibility for assignments,
      procedure calls, and operators.
    * Output: Type annotations on the AST nodes and type-mismatch diagnostics.
3. ControlFlowPass (control_flow.cpp)
    * Goal: Validate GOTO targets, ensure labels are within valid scopes, check loop structures, and verify procedure
      returns.

Why this works better: Each pass only cares about a specific set of rules. The Type Checker doesn't have to worry about
building the symbol table; it assumes the
table is already built.

Scheme 2: Component-Based Single Pass (Alternative)
If performance dictates a single-pass approach, do not build one giant class. Instead, create a lightweight
SemanticContext to hold the state, and delegate to
separate stateless worker classes:

* SemanticContext: Holds the symbol table, diagnostics, and current scope.
    * ExpressionEvaluator.cpp: Exposes a static or stateless interface like Type evaluateExpression(const Syntax::
      Expression& expr, SemanticContext& ctx). It explicitly
      returns a value rather than relying on an m_lastExpression side-effect.
    * DeclarationBinder.cpp: Exposes void bindDeclaration(const Syntax::Declaration& decl, SemanticContext& ctx).

  Summary Recommendation
  Do not recreate semantic_analysis_expressions.cpp. Instead, refactor the monolithic SemanticAnalysis class into
  distinct logical phases (Symbol Resolution, Type
  Checking, Control Flow). This yields high cohesion (all type logic is together) and loose coupling (state is
  explicitly passed rather than hidden in a God Class).
