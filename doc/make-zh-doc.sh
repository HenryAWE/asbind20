#!/usr/bin/env bash
# Generate asbind20 documentation (Simplified Chinese) via Doxygen and Sphinx.
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
OUTPUT_DIR="$PROJECT_ROOT/build"
DOXYGEN_XML="$OUTPUT_DIR/doxygen-output/xml"
SPHINX_OUTPUT="$OUTPUT_DIR/sphinx-output-zh"

mkdir -p "$OUTPUT_DIR"

# Doxygen XML (from C++ headers) is language-independent — run once, shared with English build.
if [ ! -d "$DOXYGEN_XML" ]; then
    echo "Running Doxygen..."
    cd "$OUTPUT_DIR"
    doxygen "$SCRIPT_DIR/Doxyfile"
fi

echo "Running Sphinx (zh_CN)..."
sphinx-build -b html \
    -D "language=zh_CN" \
    -D "breathe_projects.asbind20=$DOXYGEN_XML" \
    "$SCRIPT_DIR" \
    "$SPHINX_OUTPUT"

echo "Documentation generated in $SPHINX_OUTPUT"
