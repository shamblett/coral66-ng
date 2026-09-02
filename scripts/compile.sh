#!/usr/bin/env bash

#
# Project: coral66-ng
# Author : Copyright S. Hamblett <steve.hamblett@linux.com>
# Date   : 11/08/2026
#

#
# compile.sh
# Compiles a single Coral 66 source file
#

set -e

SRC_FILE=""
LIB_PATH="."
LIBS=()
EXT_PATH="."
EXTS=()
EXTRA_ARGS=()
OUT_EXT=""

while [[ "$#" -gt 0 ]]; do
    case $1 in
        -L|--libpath) LIB_PATH="$2"; shift ;;
        -l|--lib) LIBS+=("$2"); shift ;;
        -X|--extpath) EXT_PATH="$2"; shift ;;
        -x|--ext) EXTS+=("$2"); shift ;;
        -c) EXTRA_ARGS+=("-c"); OUT_EXT=".o" ;;
        -s) EXTRA_ARGS+=("-s"); OUT_EXT=".s" ;;
        -*) EXTRA_ARGS+=("$1") ;;
        *.cor) SRC_FILE="$1" ;;
        *) echo "Unknown parameter: $1"; exit 1 ;;
    esac
    shift
done

if [[ -z "$SRC_FILE" ]]; then
    echo "Error: No source file specified."
    echo "Usage: $0 <source_file.cor> [-l library_name] [-L library_path] [-x external_name] [-X external_path]"
    exit 1
fi

# 1. Build the list of object files the compiler needs to link
OBJ_FILES=""
for lib in "${LIBS[@]}"; do
    OBJ_FILES="$OBJ_FILES $LIB_PATH/$lib.o"
done
for ext in "${EXTS[@]}"; do
    OBJ_FILES="$OBJ_FILES $EXT_PATH/$ext.o"
done

# 2. Derive output executable name
OUT_EXE="${SRC_FILE%.cor}"

# 3. Execute the explicit compiler command
PRINT_LIBS="${LIBS[*]}"
if [ -z "$PRINT_LIBS" ]; then
    PRINT_LIBS="default"
fi
PRINT_EXTS="${EXTS[*]}"
if [ -z "$PRINT_EXTS" ]; then
    PRINT_EXTS="default"
fi
echo "Compiling $SRC_FILE with libraries: $PRINT_LIBS and externals: $PRINT_EXTS..."
# Execute the compiler - assuming it's available in PATH or a known location
if [ ${#EXTRA_ARGS[@]} -eq 0 ]; then
  coral66 "$SRC_FILE" "$OBJ_FILES" -L "$LIB_PATH" -X "$EXT_PATH" -o "$OUT_EXE"
else
  coral66 "$SRC_FILE" "$OBJ_FILES" -L "$LIB_PATH" -X "$EXT_PATH" "${EXTRA_ARGS[@]}" -o "$OUT_EXE$OUT_EXT"
fi
