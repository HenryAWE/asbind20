#!/usr/bin/env bash

# NOTE: This script should be called from the build output directory of benchmarks

# Run all executables start with "bench_"
for exe in bench_*; do
  if [ -x "$exe" ]; then
    echo "Running benchmark: $exe"
    ./$exe
  fi
done
