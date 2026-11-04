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

Various examples of Coral 66 program construction can be found in the [examples](examples) directory.

## Libraries and the COMMON Communicator
Libraries in coral66-ng perform the same function as in other language implementations, i.e encapsulate a set
of functions and declarations for use by Coral 66 programs to perform specific tasks. Libraries, also in common with
other languages have a public API and private declarations. The public API is communicated to other libraries and 
programs through the COMMON communicator.

In line with the language definition above the common communicator is implemented as a single common communicator declaration as per the 
specification that must be placed after the segment declaration but before the main block declaration. This 
allows the public API of the library to be communicated entirely within this communicator, all other declarations in the library are
private to the library.

A library must begin with the SEGMENT keyword to distinguish it from a program, an example :-
```
SEGMENT libraryname
COMMON( INTEGER a := 10; )

BEGIN
  INTEGER b := 20;
END;
```

Declares a library named libraryname that exports an integer 'a' with an initial value of 10 and declares a private
integer 'b' with an initial value of 20.

When the compiler sees the SEGMENT keyword the code is compiled to an object file not an executable to allow linkage to 
the main executable. A Coral Module Interface (CMI) file is also produced, see [this](cmi-specification.md) document for the 
specification of the cmi file. The cmi file is used to update the global symbol table with the contents of the libraries
COMMON communicator when the program is linked. This implies that segments are implicitly libraries however the cmi file can be empty, this allows
the segment to be linked to the main program without declaring a public API.

A library is included in a program via the LIBRARY communicator, from the 'time' example:-
```
PROGRAM Time
LIBRARY (stdio, time, unistd)

BEGIN 
 ...
END;
```
Here the support libraries 'stdio', 'time' and 'unistd' are imported and the public API of these libraries made available to 
the program via processing of the corresponding cmi file. This method alleviates the need for the main program to have a COMMON
communicator section that mirrors that of a corresponding imported library as is common in older Coral 66 implementations that
implement inter segment symbol resolution via COMMON communicators.

Doing this of course doesn't allow the main program to filter a libraries public API by declaring a COMMON communicator that
contains only the library functions required, the whole public API is made available to the main program. It is thought however
that the convenience of the cmi file approach greatly outweighs the loss of this mechanism.

## Externals




