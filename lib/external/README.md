Contains the C wrapper code for EXTERNAL communicator definitions.

Compile and link your Coral 66 source (sieve.cor) with the C wrapper (externals.c).

1. Compile the Coral source to an object file (.o):
   Compile with the -c flag (compile only, do not link) and output it as an object file.

   ```bazel run //bin:coral66 -- -c /home/steve/Development/c++/coral/coral66-ng/test/backend/data/p1/sieve-external/sieve.cor```

2. Compile the externals wrapper and link them together:
   Pass both the C wrapper and the Coral object file gcc. The C compiler will automatically invoke the system linker,
   resolve the EXTERNAL symbol, and
   link in the standard C library (libc).

   ```gcc /home/steve/Development/c++/coral/coral66-ng/lib/external/externals.c sieve.o -o sieve```

3. Run your program:
   ./sieve

