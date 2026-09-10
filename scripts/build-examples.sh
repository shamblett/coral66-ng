#!/usr/bin/env bash

#
# Project: coral66-ng
# Author : Copyright S. Hamblett <steve.hamblett@linux.com>
# Date   : 10/09/2026
#

#
# build-examples.sh
# Builds the examples against the supported libraries
#

# Resolve absolute path to script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
EXAMPLES_DIR="$ROOT_DIR/examples"

for dir in "$EXAMPLES_DIR"/*/; do
    dir="${dir%/}"
    dir_name=$(basename "$dir")
    echo "Building $dir_name example..."
    
    COR_FILE="$dir/${dir_name}.cor"
    if [[ -f "$COR_FILE" ]]; then
        "$SCRIPT_DIR/compile-supported.sh" "$COR_FILE"
    else
        echo "Warning: Source file $COR_FILE not found."
    fi
done
