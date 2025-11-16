#!/usr/bin/env bash
set -euo pipefail

# NOTE: This script should be called from the build output directory of benchmarks

# For Emscripten outputs
USE_NODE_JS=false

parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --node-js)
                USE_NODE_JS=true
                shift
                ;;
            *)
                echo "Unknown option: $1"
                exit 1
                ;;
        esac
    done
}

# Run all executables start with "bench_"
run_executable_benchmarks() {
    for exe in bench_*; do
        if [ -x "$exe" ]; then
            echo "Running benchmark: $exe"
            ./$exe
        fi
    done
}

# Run all files start with "bench_" and end with ".js"
run_nodejs_benchmarks() {
    for file in bench_*.js; do
        if [ -f "$file" ]; then
            echo "Running benchmark: $file"
            node "$file"
        fi
    done
}

parse_arguments "$@"

if [ "$USE_NODE_JS" = true ]; then
    run_nodejs_benchmarks
else
    run_executable_benchmarks
fi
