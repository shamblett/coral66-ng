Here is how the evaluation algorithm works across the compiler pipeline:                                                                          
──────
### 1. Parsing & Grouping: (Expression) vs. Numbertype(Expression)

In parser_expressions.cpp:

• Operator Precedence & Associativity: Binary operators are parsed into left-associative BinaryExpression trees in increasing order of binding    
priority:                                                                                                                                         
1. Conditional (IF ... THEN ... ELSE ...) in parseConditionalExpression                                                                       
2. Logical OR (parseCondition) and AND (parseSubcondition)                                                                                    
3. Relational comparisons (<, <=, =, >=, >, <>) in parseComparison                                                                            
4. Additive (+, -) in parseSimpleExpression                                                                                                   
5. Multiplicative (*, /) in parseTerm                                                                                                         
6. Unary +/- and untyped primaries in parseFactor
7. Word-logic DIFFER (parseBitwiseDiffer), UNION (parseBitwiseUnion), and MASK (parseBitwiseMask)
8. Typed primaries in parseTypedPrimary
• Plain Round Brackets (Expression): Handled in parseFactor. They do not create a wrapper AST node (untypedExpr = std::move(expr)). They only     
alter tree grouping (and prevent direct application of DIFFER, UNION, or MASK, which require a typed primary).
• Typed Primary Numbertype(Expression): Handled in parseTypedPrimary, which wraps the inner expression in an explicit TypeChangeExpression AST    
node.
──────
### 2. Type Determination: Bottom-Up Common-Type Promotion

Rather than pushing the target variable's type top-down into outermost terms, TypeCheckingPass synthesizes types strictly bottom-up from leaves to
root:

• Binary Promotion Hierarchy: In TypeCheckingPass::visit(const syntax::BinaryExpression&) and TypeCheckingPass::getCommonType, each binary node's
result type is derived solely from its two child operands:
1. FLOATING dominates: If either operand is FLOATING, the result type is FLOATING.
2. FIXED vs. FIXED: If both operands are FixedType, the result takes the FixedType with the larger number of fractional bits (f1-             
>getFractionalBits() >= f2->getFractionalBits() ? *f1 : *f2).
3. FIXED vs. INTEGER: If one operand is FixedType and the other is INTEGER, the result is that FixedType (INTEGER is treated as fixed-point   
with 0 fractional bits).
4. INTEGER vs. INTEGER: The result is INTEGER.
5. Word-logic (DIFFER, UNION, MASK) & Comparisons: Always yield INTEGER.
• Destination Conversion Only at the Root: Because types are synthesized bottom-up, the target type of an assignment
(CodeGenerator::visit(const syntax::AssignmentStatement&)), VALUE argument (CodeGenerator::visit(const syntax::FunctionCallExpression&)), or      
ANSWER statement (CodeGenerator::visit(const syntax::AnswerStatement&)) only converts the final result of the entire expression, not its          
individual outermost terms.
──────
### 3. Sequence of Events in Code Generation

In CodeGenerator::visit(const syntax::BinaryExpression&) and CodeGenerator::processFixedBinaryExpression:

1. Depth-First, Left-to-Right Evaluation Order:
   • For any binary operation, the left sub-expression is evaluated first (L = visit(expression.getLeft())), followed by the right sub-expression
   (R = visit(expression.getRight())).
   • (Note: When processFixedBinaryExpression is called from visit(const syntax::BinaryExpression&), it invokes visit on the left and right      
   operands a second time.)
   • Function call arguments in CodeGenerator::visit(const syntax::FunctionCallExpression&) are likewise evaluated in left-to-right order.
2. Per-Operator Scaling and Coercion:
   • FIXED with FIXED / INTEGER (processFixedBinaryExpression):
   • +, -, and comparisons: The operand with fewer fractional bits is shifted left (shl) by |leftFraction - rightFraction| so both operands  
   share max(leftFraction, rightFraction) fractional bits before add/sub/icmp.
   • *: Emits a raw 32-bit integer mul (producing leftFraction + rightFraction fractional bits) and immediately shifts (ashr or shl) to match
   resultFraction (max(leftFraction, rightFraction)).
   • /: Shifts the dividend left (shl) by resultFraction + rightFraction - leftFraction prior to signed division (sdiv) (or divides then     
   shifts right ashr if negative).
   • FLOATING mixed with INTEGER / FIXED (code_generator_expressions.cpp:405-438):
   • The integer/fixed operand is converted to float via sitofp. If it is a FixedType with fractionalBits > 0, it is unscaled via fdiv by    
   2^{fractionalBits} before executing fadd/fsub/fmul/fdiv.

3. Imposing Evaluation Order via Numbertype(Expression):
   • In CodeGenerator::visit(const syntax::TypeChangeExpression&), wrapping a sub-expression in INTEGER(...), FLOATING(...), or FIXED(t, f)(...)
   forces the inner sub-expression to be evaluated and immediately converted/rescaled (via sitofp/fdiv, fmul/fptosi, or shl/ashr fractional bit  
   scaling) to that exact type and scale before participating in surrounding operations.
