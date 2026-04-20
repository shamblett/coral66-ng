1. test/frontend/parser/data/golden/generated/g1.cor:
    - Uses unstropped keywords.
    - Starts with a PROGRAM declaration.
    - Includes a COMMON communicator with variables and arrays.
    - Exercises constructs like procedures (VALUE/LOCATION), data types (INTEGER, FLOATING, FIXED, BITS), OCTAL
      literals, inline IF...THEN...ELSE conditional
      expressions, jump labels (GOTO), block-level scoping, and both mid-code COMMENT ...; sentences and trailing
      END ...; comments.

2. test/frontend/parser/data/golden/generated/g2.cor:
    - Uses stropped keywords ('PROGRAM', 'COMMON', 'FOR', etc.).
    - Starts with a 'PROGRAM' declaration.
    - Evaluates iterating constructs, specifically using a 'FOR ... STEP ... UNTIL ... DO' statement to assign
      bounds-checked elements of a floating-point array
      inside a common block.

3. test/frontend/parser/data/golden/generated/g3.cor:
    - Uses unstropped keywords.
    - Starts with a SEGMENT declaration instead of a PROGRAM declaration.
    - Verifies the SWITCH declaration alongside jump-table-driven control-flow via GOTO, demonstrating jumping into
      sequential state machines correctly utilizing
      labels and assignment statements.