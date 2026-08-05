#!/usr/bin/env bash
# compile.sh: A friendly wrapper for the Coral 66 compiler

set -e

SRC_FILE=""
LIB_PATH="."
LIBS=()
EXT_PATH="."
EXTS=()

while [[ "$#" -gt 0 ]]; do
    case $1 in
        -L|--libpath) LIB_PATH="$2"; shift ;;
        -l|--lib) LIBS+=("$2"); shift ;;
        -X|--extpath) EXT_PATH="$2"; shift ;;
        -x|--ext) EXTS+=("$2"); shift ;;
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
echo "Compiling $SRC_FILE with libraries: ${LIBS[*]} and externals: ${EXTS[*]}..."
# Execute the compiler - assuming it's available in PATH or a known location
coral66 "$SRC_FILE" $OBJ_FILES -L "$LIB_PATH" -X "$EXT_PATH" -o "$OUT_EXE"
