In the official Coral 66 specification, an ELSE clause is strictly required for conditional expressions (Section 6.2) but remains optional for conditional statements
(Section 7.9).

Why do conditional expressions mandate an ELSE clause?

This distinction comes down to the fundamental difference between an expression and a statement in programming languages:

1. Expressions Must Evaluate to a Value: The fundamental purpose of any expression is to be evaluated and return a result (a value). If you had a conditional
   expression like IF x > 0 THEN 1 without an ELSE clause, and x was less than or equal to 0, the expression would have no value to return. Because expressions are
   used in places where a value is strictly required (such as an assignment y := IF x > 0 THEN 1 ELSE -1, or as an operand in an equation), allowing them to
   evaluate to "nothing" would result in undefined behavior or an invalid state. This is why Section 6.2 dictates ELSE Expression and notes it is "with the usual
   interpretation."


2. Statements Execute Actions: A statement, on the other hand, performs an action or a sequence of actions; it doesn't need to return a value. For a conditional
   statement, if the condition evaluates to false and there is no ELSE clause, the program simply skips the "consequence" action and moves on to the next statement.
   As Section 7.9 explicitly notes: "If the condition is false and no ELSE is present, the conditional statement has no effect beyond evaluation of the condition."
   Doing "nothing" is a perfectly valid outcome for a statement, but it is an invalid outcome for an expression.
