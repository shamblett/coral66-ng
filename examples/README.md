# Examples

This directory contains working examples of Coral 66 programs built with the compiler.

The ackerman, sieve and whetstone examples show the compilation of legacy Coral 66 code.

The time example shows the usage of libraries built from the support library.

The countdown example is particularly interesting as it takes the Countdown game numbers solver by Dave Hudson
originally written in C++ and implements it in Coral 66. The C++ version was class based and heavily recursive,
this implementation 'flattens' this into the classless block structure of Coral 66.

These can be built individually using the helper scripts or together using the [build-examples](../scripts/build-examples.sh) script.