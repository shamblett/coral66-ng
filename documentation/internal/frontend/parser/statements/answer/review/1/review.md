Based on Section 7.4 of the Official Definition of Coral 66 specification and a review of the parser code within lib/frontend/parser/parser_statements.cpp, the
following mandated answer statement functionality is not supported by the parser:

1. Context Restriction (Procedure Body Validation)
   Specification (7.4): "An answer statement is used only within a procedure body..."
   Missing Functionality: The parser code does not maintain context or enforce any state validating that the ANSWER statement is strictly located inside a typed
   procedure body. parseStatement processes an AnswerKeyword blindly and successfully generates a Syntax::AnswerStatement AST node regardless of where it appears in the
   codebase (e.g., globally or inside the main block instead of a procedure).

2. Statement Labeling
   Specification (Section 7 - Statements): The grammar formally mandates that any statement can be optionally prefixed by a label (Statement = Label : Statement),
   including the answer statement.
   Missing Functionality: The parser in parser_statements.cpp lacks any logic to recognize labels on statements. If an answer statement were correctly prefixed with a
   label (e.g., lbl : ANSWER 5;), the parser incorrectly evaluates the identifier (lbl) as an implicit ProcedureCallStatement, leaves the colon (:) unconsumed, and
   fails to parse the subsequent ANSWER statement entirely.

3. Untyped Procedure Check
   Specification (Section 8.1): Typed procedures must have an answer statement to give the procedure a value, whereas "an untyped procedure has no answer statement..."
   Missing Functionality: While checking procedure types typically borders on semantic analysis, the parser itself does not check whether the procedural scope it parses
   is currently evaluating a typed versus untyped procedure block before accepting an ANSWER statement.

(Note: The second part of Section 7.4 states that the answer statement is "followed by automatic exit from the procedure body". The parser merely maps the ANSWER
Expression syntax into the abstract syntax tree and defers control-flow generation to the backend—which is typical parser behavior, but worth noting in the context
of the full mandate.)

