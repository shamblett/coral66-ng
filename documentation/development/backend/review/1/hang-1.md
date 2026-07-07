I apologize for the delay. I've found the issue that's causing the hang.

The root cause of the infinite loop in the Whetstone test is a backward jump in the generated LLVM IR for FOR ... WHILE ... DO loops.

What is Happening?
By analyzing the generated LLVM IR from the Whetstone source code (specifically lines like 'FOR' xnorm := x 'WHILE' xnorm > 1.0 'DO'), I found that the
code_generator::visit(const syntax::ForStatement& statement) function in lib/backend/code_generator_statements.cpp is incorrectly wiring the basic blocks for
whileCondition loops.

Here is the flaw in the current WHILE loop generation:

1. Initialization (evalBB): It evaluates the assignment expression (xnorm := x) and stores it. It then unconditionally branches to condBB.
2. Condition (condBB): It evaluates the WHILE condition (xnorm > 1.0) and branches to bodyBB if true, or nextElBB if false.
3. Body (bodyBB): It executes the loop body.
4. The Bug: At the end of bodyBB, it was previously jumping back to condBB. However, because the loop variable (xnorm) isn't updated within the condition block
   itself (it's updated inside the body by statements like xnorm := xnorm * 0.25), jumping back to condBB evaluates the condition against the new value, which is
   correct.

But wait, I found the real problem.
In Coral 66, a FOR V := E1 WHILE E2 DO loop evaluates E1 every iteration, not just at the start.
The Coral 66 specification states for FOR i := E1 WHILE E2 DO Statement:
> The statement is executed repeatedly; before each execution i is set to the current value of E1. The repetition stops when E2 is false.

The current code generator has an evalBB (evaluate E1) and a condBB (evaluate E2).
At the end of the loop body (bodyBB), the code was doing:
m_builder.CreateBr(condBB);

This means it was skipping the re-evaluation of E1 (evalBB) on subsequent iterations. If the loop body modifies variables used in E1, but E1 is never re-evaluated to
update i, and the loop relies on E1 updating i to eventually make E2 false, the loop will never terminate.

In Whetstone, the loop is:

1 'FOR' xnorm := x 'WHILE' xnorm > 1.0 'DO'
2    'BEGIN'
3       rootpwr := rootpwr * 2.0;
4       xnorm := xnorm * 0.25
5    'END'
Actually, wait—in this specific Whetstone loop, x is not modified in the body, only xnorm is. If the loop jumped back to evalBB, it would re-evaluate xnorm := x
(resetting xnorm to the original x), then check xnorm > 1.0, which would remain true forever, causing an infinite loop.

So my previous fix attempt (changing the branch to evalBB) actually created an infinite loop for this specific structure, which is why the test is still hanging.

The Correct Coral 66 WHILE Semantics
Let's look at the Coral 66 standard for FOR i := expr1 WHILE expr2 DO statement.
The variable i is assigned the value of expr1. Then expr2 is evaluated. If true, the statement is executed.
Does it re-evaluate expr1 on every iteration?
Yes, Coral 66 FOR elements like E1 WHILE E2 mean:
1. Evaluate E1, assign to i.
2. Evaluate E2. If false, exit this element.
3. Execute Body.
4. Go to step 1.

If this is true, then FOR xnorm := x WHILE xnorm > 1.0 DO xnorm := xnorm * 0.25 is an infinite loop in Coral 66 if x > 1.0! Because every iteration resets xnorm to
x.
Wait, looking closely at the Whetstone code:

1    'IF' x > 1.0 'THEN'
2          'BEGIN'
3             'FOR' xnorm := x 'WHILE' xnorm > 1.0 'DO'
4                'BEGIN'
5                   rootpwr := rootpwr * 2.0;
6                   xnorm := xnorm * 0.25
7                'END'
8          'END'
Wait, if xnorm is reset to x every time, and x is never modified, this is an infinite loop.
Is it possible the Whetstone source code provided in the test suite has a bug, or is there a misunderstanding of the WHILE construct?
Actually, in ALGOL 60 (which Coral is based on), FOR V := E1 WHILE E2 evaluates E1, assigns to V, evaluates E2, and if true executes the body, then repeats.
Wait! In the Whetstone benchmark written in ALGOL, it's usually x := x * 0.25 or xnorm := xnorm * 0.25. But here the initialization is xnorm := x.

Let's check the generated IR before my fix:
Before my fix, it jumped to condBB (evaluating xnorm > 1.0). Inside the body, xnorm is updated. Because it jumps to condBB without resetting xnorm to x, xnorm
decreases, and the loop should terminate.
If it should terminate, why did it hang in the first place?

Let's investigate the exact loop in IR that hangs. I will restore the original behavior, dump the IR again, and pinpoint the exact block causing the hang without
making assumptions. I will not run the test suite again until I have statically proven the loop logic.

