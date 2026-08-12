Based on the architecture of coral66-ng and the official Coral 66 specification, here is an analysis of your current approach and a proposal for implementing a
robust string library.

1. Is passing strings as INTEGER unique to EXTERNAL?

No, it is a fundamental characteristic of the language.
According to Section 10.4 of the Official Definition of Coral 66: "A string is classed as an unconditional expression... and its value is its location, but it may
not be used as a LOCATION parameter."

Because Coral 66 lacks a native POINTER type, a string literal evaluates to a memory address, and the only standard data type capable of holding that address is an
INTEGER. When you pass a string literal like "HELLO" to a procedure, you are passing its memory location by value. The EXTERNAL communicator simply leverages this
existing language mechanic to ferry pointers to C functions.

2. Is this a sound way to proceed?

Historically, yes. On modern 64-bit architectures, no.
In the 1970s, an INTEGER was typically 24-bit or 32-bit, perfectly matching the size of a machine pointer. However, on a modern 64-bit machine, this design
introduces a critical risk:

In the current coral66-ng code generator (lib/backend/code_generator.cpp), an INTEGER is mapped strictly to an LLVM 32-bit integer (i32). Meanwhile, a string literal
is emitted as a 64-bit pointer (i8*). When a string literal is passed to a VALUE INTEGER argument, the compiler (in lib/backend/code_generator_expressions.cpp) uses
a PtrToInt instruction to cast the 64-bit pointer into a 32-bit integer.

This silently truncates the upper 32 bits of the pointer. When your C library receives this integer and casts it back to a char*, it will likely result in a
segmentation fault if the string was allocated above the 4GB memory boundary.

3. Proposed Strategy for a General String Library

Building the library in C and bridging it via the EXTERNAL communicator is unequivocally the best way forward. Implementing string manipulation natively in Coral 66
(packing/unpacking bytes into INTEGER ARRAY words) would be extremely inefficient and cumbersome.

To make the EXTERNAL approach safe and sound, you must resolve the pointer truncation issue using one of the following compiler adjustments:

* Option A: 64-bit Promotion (Simplest): Update coral66-ng so that INTEGER maps to llvm::Type::getInt64Ty instead of i32, ensuring a memory pointer always fits
  inside an INTEGER without truncation.
* Option B: 32-bit ABI Target: Compile the LLVM IR using a 32-bit target triple (e.g., -m32 equivalent) where pointers natively fit within a 32-bit integer
  constraint.
* Option C: The Handle Pattern: Instead of having the compiler emit string literals as raw memory addresses, modify the code generator to register all literal
  strings into a global compiler-generated array. A string expression would then evaluate to its 32-bit index (handle) in this array, bypassing raw pointers
  entirely.

4. Library Implementation (Assuming 64-bit Pointers or Handles are resolved)

The library should be designed using an opaque handle or pointer model. The C side manages the memory, and the Coral 66 side simply shuttles the INTEGER handles
around.

C-Side Implementation (e.g., libstring.c)

    1 #include <string.h>
    2 #include <stdlib.h>
    3
    4 // If INTEGER is promoted to 64-bit, this receives the raw pointer.
    5 // If using the Handle Pattern, this receives an integer ID.
    6 typedef const char* coral_string;
    7
    8 coral_string String_Concat(coral_string str1, coral_string str2) {
    9     size_t len1 = strlen(str1);
10     size_t len2 = strlen(str2);
11     char* result = malloc(len1 + len2 + 1);
12     strcpy(result, str1);
13     strcat(result, str2);
14     return result; // Returned as INTEGER to Coral 66
15 }
16
17 int String_Length(coral_string str) {
18     return (int)strlen(str);
19 }
20
21 coral_string String_Substring(coral_string str, int start, int length) {
22     char* result = malloc(length + 1);
23     strncpy(result, str + start, length);
24     result[length] = '\0';
25     return result;
26 }
27
28 // Section 10.4 states: "Procedures capable of selecting individual characters
29 // from a string should be designed so that characters are represented by the
30 // same integer values as are defined for literal constants."
31 int String_CharAt(coral_string str, int index) {
32     return (int)str[index];
33 }

Coral 66 Interface Wrapper

    1 EXTERNAL (
    2     INTEGER PROCEDURE StringConcat(VALUE INTEGER, VALUE INTEGER);
    3     INTEGER PROCEDURE StringLength(VALUE INTEGER);
    4     INTEGER PROCEDURE StringSubstring(VALUE INTEGER, VALUE INTEGER, VALUE INTEGER);
    5     INTEGER PROCEDURE StringCharAt(VALUE INTEGER, VALUE INTEGER)
    6 );
    7
    8 BEGIN
    9     INTEGER strA, strB, strRes, charVal;
10     
11     ! Because strings evaluate to their location, literals can be assigned to INTEGERs
12     strA := "HELLO ";
13     strB := "WORLD";
14     
15     strRes := StringConcat(strA, strB);
16     charVal := StringCharAt(strRes, 0) ! Retrieves 'H' as an integer value
17 END

Summary: The EXTERNAL communicator mapping strings to INTEGERs is perfectly idiomatic Coral 66. However, to construct a sound, general-purpose library, you must
address the 32-bit INTEGER versus 64-bit pointer mismatch at the compiler's code-generation layer before mapping these pointers into your C library.


