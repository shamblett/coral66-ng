# Program Structure, Linking and Externals
Section 2.2 of the Official Definition of Coral 66, the specification, explicitly states :-
```
The language definition does not specify how the program or its segments shall be named or how the segments are to be 
separated or terminated...
```
The compiler has taken the following approach, Coral 66 code files must start with the keyword PROGRAM or SEGMENT.

Code starting with the keyword PROGRAM is built into an executable by the compiler, code starting with the keyword SEGMENT
is built into an object(.o) file by the compiler. This allows the sharing and linking of library code between programs
using the LIBRARY and EXTERNAL communicators. The final keyword of a source final must be the END keyword which must
match a preceding BEGIN keyword.

Before expanding on this other structural requirements of Coral 66 source programs are as follows :-

   - The source file must have the file type '.cor' whether it is a program or a library.
   - Historically Coral 66 keywords can be stropped, e.g. 'BEGIN'. The specification allows both 
     stropped and unstropped keywords but they must be in uppercase. This allows compatibility
     with legacy source code implementations. Mixing is allowed in a source file although this is
     discouraged.