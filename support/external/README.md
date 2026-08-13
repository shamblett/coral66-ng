Contains the C wrapper code for EXTERNAL communicator definitions.

Compile and link your Coral 66 source with the C wrapper or wrappers you need.

1. Compile the externals wrapper(s) and link them together:
   Pass both the C wrapper and the Coral object file gcc. The C compiler will automatically invoke the system linker,
   resolve the EXTERNAL symbol, and
   link in the standard C library (libc).

   ```gcc -m32 -c string.c -o libstring.o -O2 -Wall```

2. Compile the Coral 66 source file using the scripts/compile-supported.sh script

   ```./compile-supported.sh myprogram.cor```

   Note this will use the default library and externals directories in the support directory.

3. Run your program:
   ./myprogram

