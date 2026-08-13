#!/usr/bin/env bash

#
# Project: coral66-ng
# Author : Copyright S. Hamblett <steve.hamblett@linux.com>
# Date   : 13/08/2026
#

#
# compile-supported.sh
# Compiles a Coral 66 source file against default external and library directories
#

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
COMPILE_SH="$SCRIPT_DIR/compile.sh"

EXT_DIR="$ROOT_DIR/support/external"
LIB_DIR="$ROOT_DIR/support/library"

# Determine if user has provided -X or -L to override search directories
USER_EXT_DIR="$EXT_DIR"
USER_LIB_DIR="$LIB_DIR"

ARGS=("$@")
for (( i=0; i<${#ARGS[@]}; i++ )); do
    if [[ "${ARGS[$i]}" == "-X" || "${ARGS[$i]}" == "--extpath" ]]; then
        if [[ $((i+1)) < ${#ARGS[@]} ]]; then
            USER_EXT_DIR="${ARGS[$((i+1))]}"
        fi
    elif [[ "${ARGS[$i]}" == "-L" || "${ARGS[$i]}" == "--libpath" ]]; then
        if [[ $((i+1)) < ${#ARGS[@]} ]]; then
            USER_LIB_DIR="${ARGS[$((i+1))]}"
        fi
    fi
done

# Gather object files from the externals directory
EXT_ARGS=()
shopt -s nullglob
if [ -d "$USER_EXT_DIR" ]; then
    for obj in "$USER_EXT_DIR"/*.o; do
        base=$(basename "$obj" .o)
        EXT_ARGS+=("-x" "$base")
    done
fi

# Gather object files from the libraries directory
LIB_ARGS=()
if [ -d "$USER_LIB_DIR" ]; then
    for obj in "$USER_LIB_DIR"/*.o; do
        base=$(basename "$obj" .o)
        LIB_ARGS+=("-l" "$base")
    done
fi
shopt -u nullglob

# Set default directories explicitly so compile.sh defaults are overridden
DEFAULT_PATHS=("-L" "$LIB_DIR" "-X" "$EXT_DIR")

"$COMPILE_SH" "${DEFAULT_PATHS[@]}" "${LIB_ARGS[@]}" "${EXT_ARGS[@]}" "$@"
