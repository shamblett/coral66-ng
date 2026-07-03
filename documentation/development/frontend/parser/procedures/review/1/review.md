Based on an analysis of the Coral 66 procedure handling within lib/frontend/parser/parser_procedures.cpp and its interactions with
lib/frontend/parser/parser_declarations.cpp, here is a structured summary of the mandated procedure functionality from Section 8 of the Coral 66 specification that
is currently unsupported or incorrectly implemented:

1. Answer Specification & Typed Procedures (Section 8.1)
   The parser currently fails to properly capture the return type for typed procedures. In parseDeclaration, the numeric type (e.g. INTEGER, FLOATING) is consumed but
   subsequently discarded when it fails to find a variable identifier. When the logic falls back to parseProcedure, it erroneously assigns the PROCEDURE keyword token
   itself as the type token rather than the provided numeric type. This results in the complete loss of the procedure's intended answer type. The VOID answer
   specification is also fundamentally unaccounted for.

2. Parameter Specification Deficiencies (Section 8.3)
   The formal parameter list currently relies entirely on the generic parseDeclaration method, which is strictly designed for standard variable, array, and table
   declarations. It completely lacks support for the following mandated parameter types:
* Value Parameters (8.3.1): The VALUE keyword (VALUE Numbertype Idlist) is unhandled.
* Word Location Parameters (8.3.2.1): The LOCATION keyword (LOCATION Numbertype Idlist) is unhandled.
* Place Parameters (8.3.3): LABEL and SWITCH parameters are not recognized.
* Procedure Parameters (8.3.4): The ability to pass a procedure as a formal parameter is missing.
* Non-standard Parameter Specifications (8.3.5): Implementation-dependent syntactic forms like VALUE Id : Id or LOCATION Id : Id are unsupported.

(Note: Array (8.3.2.2) and Table (8.3.2.3) parameters are coincidentally accepted because parseDeclaration parses them and its permissive logic allows omitting
dimension bounds, but the parser is not explicitly enforcing procedure parameter semantics).

3. Procedure Body Constraints (Section 8.4)
   The parser explicitly requires a procedure body to start with the BEGIN keyword (lexer::TokenType::BeginKeyword). This violates the specification, which defines the
   body simply as a generic Statement. In Coral 66, the body can be any single statement (which is implicitly treated as a block for scoping purposes whether or not it
   has local declarations) and does not strictly require explicit BEGIN ... END block boundaries unless it contains multiple statements.

4. General Implementation Structure
   The architecture of the procedure parser is currently disjointed and duplicates logic. Procedure parsing is awkwardly split between parseDeclaration (which captures
   untyped procedures and hardcodes them as isTyped = false) and parseProcedure (which expects typed procedures that fall through the cracks, and hardcodes them as
   isTyped = true). This fragmented structure creates a fragile execution path that actively misidentifies and drops valid syntax like single-statement bodies and typed
   procedure definitions.

