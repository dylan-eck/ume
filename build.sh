#!/usr/bin/env bash
set -euo pipefail

repo_root="$(pwd)"
target_name="ume"
run_after_build=false
preset=""
run_args=()

usage() {
  echo "Usage: $0 [-r|--run] <preset> [-- <args passed to the executable>]" >&2
  exit 1
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -r|--run)
      run_after_build=true
      shift
      ;;
    --)
      shift
      run_args=("$@")
      break
      ;;
    -*)
      echo "error: unknown option: $1" >&2
      usage
      ;;
    *)
      if [[ -n "$preset" ]]; then
        echo "error: unexpected argument: $1" >&2
        usage
      fi
      preset="$1"
      shift
      ;;
  esac
done

[[ -n "$preset" ]] || usage

build_dir="build/$preset"

cmake --preset "$preset"
cmake --build --preset "$preset"

ln -sf "$build_dir/compile_commands.json" compile_commands.json

if "$run_after_build"; then
  binary="$repo_root/$build_dir/$target_name"
  if [[ ! -x "$binary" ]]; then
    echo "error: expected executable at $binary but it wasn't found" >&2
    exit 1
  fi

  sandbox_dir="$repo_root/sandbox"
  if [[ ! -d "$sandbox_dir" ]]; then
    echo "error: sandbox directory not found at $sandbox_dir" >&2
    exit 1
  fi

  cd "$sandbox_dir"
  exec "$binary" "${run_args[@]+"${run_args[@]}"}"
fi