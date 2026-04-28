#!/usr/bin/env bash
# Generate asbind20 documentation via Doxygen and Sphinx.
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
OUTPUT_DIR="$PROJECT_ROOT/build"

mkdir -p "$OUTPUT_DIR"

echo "Running Doxygen..."
cd "$OUTPUT_DIR"
doxygen "$SCRIPT_DIR/Doxyfile"

echo "Running Sphinx..."
sphinx-build -b html \
    -D "breathe_projects.asbind20=$OUTPUT_DIR/doxygen-output/xml" \
    "$SCRIPT_DIR" \
    "$OUTPUT_DIR/sphinx-output"

echo "Documentation generated in $OUTPUT_DIR/sphinx-output"
