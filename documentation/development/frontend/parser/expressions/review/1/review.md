# Coral 66 Parser Review: Expressions (Section 6)

A review of the expression parsing implementation in `lib/frontend/parser/parser_expressions.cpp` against the Coral 66 language specification and grammar has been performed. The implementation exhibits a high degree of fidelity to the official standard. The parser correctly maps complex precedence rules—such as conditions vs. simple expressions, the absence of an exponentiation operator, left-associativity of arithmetic, and the `MASK` > `UNION` > `DIFFER` bitwise operation precedence.

Below are the highlighted discrepancies where the parser diverges from the strict grammar specification.

### 1. Functionality mandated by the specification that is not supported by the code

Based on the review of Section 6 of the Official Definition of Coral 66 and the provided `coral66-grammar.txt`, **there is no missing functionality in the expression parser.** 

The code accurately models all required syntactic structures, including:
* **Conditional Expressions:** Strictly enforces the `IF Condition THEN Expression ELSE Expression` form.
* **Typing Hierarchy:** Correctly segregates `Untypedprimary` (reals and bracketed expressions) from `Typedprimary` (integers, word references, part-words, etc.).
* **Anonymous Word References:** Correctly supports the `[ Index ]` syntax mandated in Section 6.1.1.2.1.
* **Precedence Rules:** Correctly models condition hierarchies (`OR` > `AND` > `Comparator`) and arithmetic/bitwise precedence trees.

---

### 2. Functionality in the code that is not needed by the specification

The parser is structurally more permissive than the strict Coral 66 grammar dictates in a few specific areas regarding scale and bit constraints. While a Semantic Analyzer may catch these later, the syntactic parser accepts a wider range of inputs than necessary.

#### A. Unscaled Explicit Type-Changing (`FIXED` without `Scale`)
The code permits explicit type conversion to a fixed-point number without providing the mandatory scaling factor.
* **Implementation:** In `parseTypedPrimary`, the code accepts `FIXED ( Expression )` and constructs a `TypeChangeExpression` without fractional parameters.
* **Specification:** The grammar strictly defines the number type as `Numbertype ::= FLOATING | FIXED Scale | INTEGER`, where `Scale ::= ( Totalbits , Fractionbits )`. Therefore, a fixed-point type change must always take the form `FIXED ( Totalbits , Fractionbits ) ( Expression )`. Allowing it without the scale is an extension not defined by the standard.

#### B. Permissive Evaluation of Numeric Constraints (`Scale` and `BITS`)
The code uses the generic `parseExpression` function to parse the constraint fields for fixed scales and part-word bit extractions.
* **Implementation:** 
  * In the `FIXED` keyword block, the `Totalbits` and `Fractionbits` are parsed using `parseExpression`.
  * In the `BITS` keyword block, the `Totalbits` and `Bitposition` are parsed using `parseExpression`.
* **Specification:** The grammar strictly mandates that these fields are compile-time integer constructs, not full arithmetic expressions:
  * For `Scale`: `Totalbits` must be an `Integer` and `Fractionbits` must be a `Signedinteger`.
  * For `Partword`: `BITS [ Totalbits , Bitposition ] Typedprimary` requires both `Totalbits` and `Bitposition` to be an `Integer`. 
* **Impact:** The code accepts arbitrarily complex expressions (e.g., `BITS [ x + y * 2, z ] a`) at the parsing stage, whereas the grammar expects simple integer literals.

#### C. Actuallist (Procedure Call Arguments) Abstraction
The code parses procedure call arguments generically via `parseExpression`.
* **Implementation:** `parseTypedPrimary` handles procedure calls by parsing a comma-separated list of arguments utilizing `parseExpression`.
* **Specification:** `Actuallist` elements (`Actual`) can be an `Expression`, `Wordreference`, `Destination`, or `Name`.
* **Impact:** Because `Wordreference`, `Destination` (labels/switches), and `Name` all fall under the syntactic umbrella of expressions or identifiers in this AST model, `parseExpression` successfully subsumes them. This is functionally sound for a parser (delegating exact validation to the semantic analyzer), but strictly speaking, it conflates syntactic categories defined in the specification.
