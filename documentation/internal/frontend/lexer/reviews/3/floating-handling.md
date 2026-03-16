Based on the official Coral 66 specification, removing the 10 symbol entirely means the lexer is no longer fully compliant with the specification.

Here is why:


What the Specification Mandates
The Coral 66 grammar explicitly defines Real (floating-point) numbers with an exponent symbol (written as 10 in the grammar documents):


1 Real ::=
2     Digitlist . Digitlist
3     Digitlist 10 Signedinteger
4         10 Signedinteger
5     Digitlist . Digitlist 10 Signedinteger
According to the specification, strings like 1.2 10 3 (meaning $1.2 \times 10^3$) or 10 -2 (meaning $10^{-2}$) are perfectly valid single floating-point literals.


The Problem with ASCII and the 10 Symbol
In the original 1970 Coral 66 standard, the exponent symbol was intended to be a single, distinct hardware character, usually represented visually as a subscript ₁₀ or a
bold 10.


Because modern ASCII lacks this specific character, the lexer was previously attempting to use the regular integer literal 10 to act as the exponent symbol. However, as
we saw earlier, this approach is fundamentally flawed because it creates an unresolvable syntactic ambiguity (e.g., does 10 - 2 mean the integer arithmetic expression
$10 - 2 = 8$, or the floating-point literal $10^{-2} = 0.01$?).


Conclusion
By removing the greedy 10 logic:
1. You fixed a major bug: Standard integer arithmetic (like 10 - 2) will now tokenize and parse correctly.
2. You broke a specification requirement: The lexer no longer supports floating-point exponent notation.


Recommended Fix
To be both compliant with the specification and functionally correct in a modern ASCII environment, the typical solution for historic Coral 66 compilers was to adopt a
transliterated character for the exponent symbol (like e, E, or @), similar to how C/C++ uses e (e.g., 1.2e3). If you'd like, we can introduce e/E or a specific
character as the accepted exponent symbol to restore full specification compliance without breaking integer arithmetic.
