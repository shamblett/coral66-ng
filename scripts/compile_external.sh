#!/usr/bin/env bash
#
# compile_external.sh
# Safely compiles C code into a 32-bit object file for linkage 
# with the coral66-ng compiler.

set -e

if [ "$#" -eq 0 ]; then
    echo "Usage: $0 <source.c> [additional gcc options]"
    echo "Example: $0 libstring.c -O2"
    exit 1
fi

SOURCE_FILE="$1"
shift

# Check if the source file exists
if [ ! -f "$SOURCE_FILE" ]; then
    echo "Error: Source file '$SOURCE_FILE' not found."
    exit 1
fi

# Extract filename without extension for the output object file
BASENAME=${SOURCE_FILE##*/}
BASENAME=${BASENAME%.c}
OUTPUT_FILE="${BASENAME}.o"

echo "Compiling $SOURCE_FILE to 32-bit object file: $OUTPUT_FILE"

# Execute gcc with -m32 for 32-bit compilation, -c to output an object file
gcc -m32 -c "$SOURCE_FILE" -o "$OUTPUT_FILE" "$@"

if [ $? -eq 0 ]; then
    echo "Success! Linked via: coral66 my_program.o $OUTPUT_FILE -o my_program"
else
    echo "Compilation failed."
    exit 1
fi
