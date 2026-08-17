#!/usr/bin/env bash

#
# Project: coral66-ng
# Author : Copyright S. Hamblett <steve.hamblett@linux.com>
# Date   : 11/08/2026
#

#
# install.sh
# Install the coral66 compiler into the users bin directory.
#
set -e

# Default values
EXTERNAL_PATH=""
LIBRARY_PATH=""
INSTALL_DIR="$HOME/bin"

# Help message
usage() {
    cat << HELP_EOF
Usage: $0 [OPTIONS]

Installs the coral66 binary and optionally its associated external/ and library/ directories.

Options:
  -h          Show this help message and exit.
  -X PATH     Copy the contents of the 'external' directory to PATH.
  -L PATH     Copy the contents of the 'library' directory to PATH.
HELP_EOF
}

# Parse command line options
while getopts ":hX:L:" opt; do
    case "${opt}" in
        h)
            usage
            exit 0
            ;;
        X)
            EXTERNAL_PATH="${OPTARG}"
            ;;
        L)
            LIBRARY_PATH="${OPTARG}"
            ;;
        \?)
            echo "Error: Invalid option -${OPTARG}" >&2
            usage >&2
            exit 1
            ;;
        :)
            echo "Error: Option -${OPTARG} requires an argument." >&2
            usage >&2
            exit 1
            ;;
    esac
done

# Get the directory where the script is located to find source directories relative to it
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

# Paths to the source files
CORAL_BIN="$ROOT_DIR/bazel-bin/bin/coral66"
EXTERNAL_SRC="$ROOT_DIR/external"
LIBRARY_SRC="$ROOT_DIR/library"

# 1. Copy the coral66 binary
if [[ ! -f "$CORAL_BIN" ]]; then
    echo "Error: coral66 binary not found at $CORAL_BIN" >&2
    echo "Please build the project first (e.g., 'bazel build //bin:coral66')." >&2
    exit 1
fi

echo "Installing coral66 binary to $INSTALL_DIR..."
if [[ ! -d "$INSTALL_DIR" ]]; then
    echo "Creating directory $INSTALL_DIR..."
    if ! mkdir -p "$INSTALL_DIR"; then
        echo "Error: Failed to create directory $INSTALL_DIR" >&2
        exit 1
    fi
fi

# Remove existing binary to prevent permission errors if it is read-only
rm -f "$INSTALL_DIR/coral66"

if ! cp "$CORAL_BIN" "$INSTALL_DIR/coral66"; then
    echo "Error: Failed to copy coral66 binary to $INSTALL_DIR" >&2
    exit 1
fi

if ! chmod +x "$INSTALL_DIR/coral66"; then
    echo "Error: Failed to set execute permission on $INSTALL_DIR/coral66" >&2
    exit 1
fi
echo "Successfully installed coral66 to $INSTALL_DIR/coral66"

# 2. Copy the external directory if requested
if [[ -n "$EXTERNAL_PATH" ]]; then
    echo "Installing external directory to $EXTERNAL_PATH..."
    if [[ ! -d "$EXTERNAL_SRC" ]]; then
        echo "Error: Source external directory not found at $EXTERNAL_SRC" >&2
        exit 1
    fi
    
    if [[ ! -d "$EXTERNAL_PATH" ]]; then
        echo "Creating directory $EXTERNAL_PATH..."
        if ! mkdir -p "$EXTERNAL_PATH"; then
            echo "Error: Failed to create directory $EXTERNAL_PATH" >&2
            exit 1
        fi
    fi
    
    if ! cp -r "$EXTERNAL_SRC"/. "$EXTERNAL_PATH/"; then
        echo "Error: Failed to copy contents to $EXTERNAL_PATH" >&2
        exit 1
    fi
    echo "Successfully installed external directory to $EXTERNAL_PATH"
fi

# 3. Copy the library directory if requested
if [[ -n "$LIBRARY_PATH" ]]; then
    echo "Installing library directory to $LIBRARY_PATH..."
    if [[ ! -d "$LIBRARY_SRC" ]]; then
        echo "Error: Source library directory not found at $LIBRARY_SRC" >&2
        exit 1
    fi
    
    if [[ ! -d "$LIBRARY_PATH" ]]; then
        echo "Creating directory $LIBRARY_PATH..."
        if ! mkdir -p "$LIBRARY_PATH"; then
            echo "Error: Failed to create directory $LIBRARY_PATH" >&2
            exit 1
        fi
    fi
    
    if ! cp -r "$LIBRARY_SRC"/. "$LIBRARY_PATH/"; then
        echo "Error: Failed to copy contents to $LIBRARY_PATH" >&2
        exit 1
    fi
    echo "Successfully installed library directory to $LIBRARY_PATH"
fi

echo "Installation complete."
