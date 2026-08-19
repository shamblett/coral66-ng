#!/usr/bin/env bash

#
# Project: coral66-ng
# Author : Copyright S. Hamblett <steve.hamblett@linux.com>
# Date   : 19/08/2026
#

#
# uppercase-keywords.sh
# Converts stropped lowercase keywords e.g 'comment' to uppercase ones
# i.e. 'COMMENT'
#

if [ -z "$1" ]; then
    echo "Usage: $0 <filename>"
    echo "Example: $0 sieve.cor"
    exit 1
fi

FILE="$1"

if [ ! -f "$FILE" ]; then
    echo "Error: File '$FILE' not found."
    exit 1
fi

# Uppercase all stropped keywords (e.g. 'integer' -> 'INTEGER')
perl -pi -e "s/'([a-z]+)'/'\U\$1'/g" "$FILE"

echo "Successfully uppercased stropped keywords in '$FILE'."
