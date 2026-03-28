Based on a review of lib/frontend/parser/parser_statements.cpp alongside Section 7 of the Official Definition of Coral 66, the parser implements the general
structure for the supported statement types but fails to comply with the language specification in several key areas.

Here are the specific mandated functionalities that are either not supported or incorrectly parsed:

1. FOR Statements (Section 7.10)
* Restricted Control Variable: The Coral 66 grammar defines the target of a FOR loop as Forstatement = FOR Wordreference <- Forlist DO Statement. A Wordreference
  can be an identifier, an indexed array element (e.g., A[I]), or an anonymous memory reference (e.g., [LOCATION(X)]). However, the parser forcibly expects a bare
  identifier:

1   if (begin != end && begin->getTokenType() == lexer::TokenType::Identifier)
Any valid array index or memory reference target will throw an EXPECTED_IDENTIFIER error.
* Invalid WHILE Condition Parsing: A valid FOR element can optionally take the form Expression WHILE Condition. The parser evaluates the right-hand side of WHILE
  using parseExpression(begin, end, context). In Coral 66, boolean logical conditions (using operators like =, <, AND) are distinctly parsed as Condition rather
  than standard numeric Expressions (the parser uses parseCondition for IF statements). By calling parseExpression, the parser will fail to comprehend comparison
  operators within WHILE loops.

2. GOTO Statements (Section 7.2)
* Flawed Open Square Bracket Requirement: The grammar dictates Gotostatement = GOTO Destination, where Destination is either a Label (an identifier) or a
  Switch[Index]. While the parser correctly attempts to parse an optional index for a switch, it has a critical defect immediately following this check. It skips an
  optional semicolon and then mandates the presence of an open bracket:

1   if (begin != end && begin->getTokenType() != lexer::TokenType::OpenSquareBracket)
2   {
3       context.log(errors::parser::EXPECTED_OPEN_SQUARE_BRACKET...);
4       return nullptr;
5   }
Because of this rogue check, any standard unstructured jump (e.g., GOTO L;) that isn't immediately followed by an opening bracket token on the next statement will
fail to parse and error out.

3. Procedure Statements (Section 7.3)
* Semicolon Usage Errors: Coral 66 uses semicolons as statement separators, not terminators. This means a statement sitting at the end of a block (e.g., BEGIN P(X)
  END) or before an ELSE condition (e.g., IF cond THEN P(X) ELSE...) must not have a trailing semicolon.
  The parser identifies Procedure Calls (which start with an identifier, similar to assignments) by mandatorily expecting a semicolon:

1   else if (begin != end && begin->getTokenType() == lexer::TokenType::SemiColon)
2   {
3       begin++;
4       return std::make_unique<Syntax::ProcedureCallStatement>(start, begin, std::move(expr));
5   }
If a Procedure Call legitimately appears without a trailing semicolon (like at the end of a block or before an ELSE statement), this check fails, and the parser
mistakenly throws an EXPECTED_ASSIGNMENT error.

4. Code Statements (Section 7.5)
* String Literal Requirement Constraint: The specification indicates that CODE BEGIN ... END can contain any sequence of code instructions (typically nested
  assembly or specialized code). The parser currently enforces that the entire inner body of a CODE statement must be a single string literal
  lexer::TokenType::StringLiteral. While Section 7.5 states "the form of the code is not defined," wrapping the code sequence in string quotes is technically a
  compiler-specific constraint not envisioned by standard Coral text syntax.
