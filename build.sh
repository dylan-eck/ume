#!/usr/bin/env bash
set -euo pipefail

preset="$1"
build_dir="build/$preset"

cmake --preset "$preset"
cmake --build --preset "$preset"

ln -sf "$build_dir/compile_commands.json" compile_commands.json