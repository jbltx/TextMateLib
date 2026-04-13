#!/bin/bash

# Copy WASM build output from ../tml-cpp/ to the wasm/ directory
# Supports different build variants: wasm, wasm-standard (default: wasm-standard)

set -e

VARIANT="${1:-wasm-standard}"
SOURCE_DIR="../tml-cpp/build/$VARIANT/browser"
DEST_DIR="wasm"

# Verify source directory exists
if [ ! -d "$SOURCE_DIR" ]; then
    echo "Error: Source directory not found: $SOURCE_DIR"
    echo "Available variants: wasm, wasm-standard"
    exit 1
fi

# Create destination directory if it doesn't exist
mkdir -p "$DEST_DIR"

# Copy WASM files
echo "Copying WASM files from $SOURCE_DIR to $DEST_DIR..."

if [ -f "$SOURCE_DIR/tml-standard.wasm" ]; then
    cp "$SOURCE_DIR/tml-standard.wasm" "$DEST_DIR/"
    echo "✓ Copied tml-standard.wasm"
else
    echo "Error: tml-standard.wasm not found in $SOURCE_DIR"
    exit 1
fi

if [ -f "$SOURCE_DIR/tml-standard.js" ]; then
    cp "$SOURCE_DIR/tml-standard.js" "$DEST_DIR/"
    echo "✓ Copied tml-standard.js"
else
    echo "Error: tml-standard.js not found in $SOURCE_DIR"
    exit 1
fi

echo "Done! WASM files updated in $DEST_DIR"
