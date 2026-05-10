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
