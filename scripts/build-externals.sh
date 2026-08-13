#!/usr/bin/env bash

#
# Project: coral66-ng
# Author : Copyright S. Hamblett <steve.hamblett@linux.com>
# Date   : 13/08/2026
#

#
# build-externals.sh
# Builds external C files for coral66-ng
#

# Resolve absolute path to script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

EXT_DIR="$ROOT_DIR/support/external"

while getopts "X:" opt; do
  case $opt in
    X) EXT_DIR="$OPTARG" ;;
    *) echo "Usage: $0 [-X externals_directory]"; exit 1 ;;
  esac
done

COMPILE_EXT_SH="$SCRIPT_DIR/compile-external.sh"

shopt -s nullglob
C_FILES=("$EXT_DIR"/*.c)
shopt -u nullglob

if [ ${#C_FILES[@]} -eq 0 ]; then
  echo "No C files found in $EXT_DIR"
  exit 0
fi

for file in "${C_FILES[@]}"; do
  echo "Building external $file..."
  (cd "$EXT_DIR" && "$COMPILE_EXT_SH" "$(basename "$file")")
done
