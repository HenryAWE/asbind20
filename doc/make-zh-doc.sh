#!/usr/bin/env bash
# Generate asbind20 documentation (Simplified Chinese) via Doxygen and Sphinx.
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
OUTPUT_DIR="$PROJECT_ROOT/build"
DOXYGEN_OUTPUT="$OUTPUT_DIR/doxygen-output-zh"
SPHINX_OUTPUT="$OUTPUT_DIR/sphinx-output-zh"

mkdir -p "$OUTPUT_DIR" "$DOXYGEN_OUTPUT"

echo "Running Doxygen..."
cd "$OUTPUT_DIR"
(cat "$SCRIPT_DIR/Doxyfile"; echo "XML_OUTPUT = doxygen-output-zh/xml") | doxygen -

echo "Running Sphinx..."
sphinx-build -b html \
    -D "language=zh_CN" \
    -D "breathe_projects.asbind20=$DOXYGEN_OUTPUT/xml" \
    "$SCRIPT_DIR" \
    "$SPHINX_OUTPUT"

echo "Documentation generated in $SPHINX_OUTPUT"
