#!/usr/bin/env bash

#
# Project: coral66-ng
# Author : Copyright S. Hamblett <steve.hamblett@linux.com>
# Date   : 11/08/2026
#

#
# compile_external.sh
# Safely compiles C code into a 32-bit object file for linkage 
# with the coral66-ng compiler.

set -e

if [ "$#" -eq 0 ]; then
    echo "Usage: $0 <source.c> [additional gcc options]"
    echo "Example: $0 string.c -O2"
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

echo "Compiling $SOURCE_FILE to object file: $OUTPUT_FILE"

# Execute gcc with -c to output an object file
gcc -c "$SOURCE_FILE" -o "$OUTPUT_FILE" "$@"

if [ $? -eq 0 ]; then
    echo "Success!"
else
    echo "Compilation failed."
    exit 1
fi
