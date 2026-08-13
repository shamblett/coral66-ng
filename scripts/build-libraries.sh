#!/usr/bin/env bash

#
# Project: coral66-ng
# Author : Copyright S. Hamblett <steve.hamblett@linux.com>
# Date   : 13/08/2026
#

#
# build-libraries.sh
# Builds Coral 66 library files for coral66-ng
#

# Resolve absolute path to script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

LIB_DIR="$ROOT_DIR/support/library"

while getopts "L:" opt; do
  case $opt in
    L) LIB_DIR="$OPTARG" ;;
    *) echo "Usage: $0 [-L library_directory]"; exit 1 ;;
  esac
done

COMPILE_SH="$SCRIPT_DIR/compile.sh"

shopt -s nullglob
COR_FILES=("$LIB_DIR"/*.cor)
shopt -u nullglob

if [ ${#COR_FILES[@]} -eq 0 ]; then
  echo "No Coral 66 library files found in $LIB_DIR"
  exit 0
fi

for file in "${COR_FILES[@]}"; do
  echo "Building library $file..."
  (cd "$LIB_DIR" && "$COMPILE_SH" "$(basename "$file")")
done
