
# Communicator specification for `coral66-ng`

1. The COMMON communicator will be supported as per the specification.


2. The LIBRARY communicator will allow importing of Coral 66 code from an external named library. This code will be placed into global scope
   at the position of the LIBRARY communicator in the source file and compiled as if it was present in the source file. An example is
   `LIBRARY math;` whereby the compiler will look for a file named `math.cor` in the library path as specified on the command line.


3. The EXTERNAL communicator will allow linkage between Coral 66 procedures and external libraries such as stdlib so that access
   to the platforms facilities such as I/O can be achieved by linking the program against the appropriate shared library.


4. The ABSOLUTE communicator is not supported, there is no concept of an absolute memory address on this platform as far as 
   Coral 66 is concerned.