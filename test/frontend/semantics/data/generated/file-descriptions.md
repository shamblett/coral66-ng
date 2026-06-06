1. g1.cor:
    - Uses unstropped keywords.
    - Starts with a PROGRAM declaration.
    - Includes a COMMON communicator with variables and arrays.
    - Exercises constructs like procedures (VALUE/LOCATION), data types (INTEGER, FLOATING, FIXED, BITS), OCTAL
      literals, inline IF...THEN...ELSE conditional
      expressions, jump labels (GOTO), block-level scoping, and both mid-code COMMENT ...; sentences and trailing
      END ...; comments.


2. g2.cor:
    - Uses stropped keywords ('PROGRAM', 'COMMON', 'FOR', etc.).
    - Starts with a 'PROGRAM' declaration.
    - Evaluates iterating constructs, specifically using a 'FOR ... STEP ... UNTIL ... DO' statement to assign
      bounds-checked elements of a floating-point array
      inside a common block.


3. g3.cor:
    - Uses unstropped keywords.
    - Starts with a SEGMENT declaration instead of a PROGRAM declaration.
    - Verifies the SWITCH declaration alongside jump-table-driven control-flow via GOTO, demonstrating jumping into
      sequential state machines correctly utilizing
      labels and assignment statements.


4. g4.cor
   This file exercises table and overlay data structures, procedures, logic operators, bitwise operators, loops, and literal references:
    - TABLE, PRESET, BIT, UNSIGNED (Used to define and pre-fill structured tables).
    - OVERLAY, WITH (Used to overlap memory).
    - RECURSIVE, ANSWER (Used to generate a recursive factorial procedure).
    - LABEL (Used as a parameter passing mode for a jumping procedure).
    - MASK, UNION, DIFFER (Bitwise logical operations between integers).
    - AND, OR (Used in IF ... THEN statements for multi-condition branching).
    - WHILE (Used for unbounded FOR loop evaluation).
    - LITERAL (Used for direct character-code resolution).


5. g5.cor
  This file specifically targets edge features like inline system code and the Coral 66 macro preprocessor:
   - DEFINE (Preprocessor keyword to define a macro string).
   - DELETE (Preprocessor keyword to erase a macro from memory).
   - CODE (To verify the CODE BEGIN "..." END inline code assembly/escape).


6. g6.cor
  This file focuses on multidimensional arrays, scientific notation literals, and advanced loops:
   - Evaluates multi-dimensional arrays (e.g., `INTEGER ARRAY grid [1:5, 1:5]`).
   - Uses floating-point scientific notation constants (e.g., `1.5 10 3`).
   - Validates `FOR` loops with multiple sequence elements and mixed forms (`FOR i := 1, 2, 4, 8 DO` and `FOR i := 1, i * 2 WHILE i < 10 DO`).


7. g7.cor
  This file targets type coercion, macro arguments, and memory access operators:
   - Tests parametrized macros (`DEFINE macroWithArgs(A, B) " A + B ";`).
   - Verifies typed casting functions (`INTEGER (3.14)`, `FLOATING (42)`).
   - Tests explicit `LOCATION ( Wordreference )` semantics.


8. g8.cor
  This file exercises the `COMMON` communicator with complex items:
   - Demonstrates declaring and accessing a `LABEL` within a `COMMON` block.
   - Demonstrates declaring a `PROCEDURE` interface inside a `COMMON` block.
   - Tests an `ARRAY` shared across scopes via a `COMMON` communicator.


9. g9.cor
  This file validates procedures accepting complex parameters:
   - Tests procedures that receive `INTEGER ARRAY` references.
   - Tests passing `LABEL` and `SWITCH` parameters to influence the calling context's control flow.
   - Exercises local jump destinations passed as arguments and invoked dynamically.


10. g10.cor
  This file checks string literal handling and complex block scoping:
   - Passes `STRING` literals into procedures (e.g., `"Hello Coral 66"`).
   - Contains deeply nested `BEGIN ... END` block structures to test proper lexical scoping and variable shadowing.

11. g10.cor and g11.cor
    These file check duplicate and missing label handling:

12. g12.cor
    This file checks EXTERNAL communicator processing.
