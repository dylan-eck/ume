#!/usr/bin/env bash
set -euo pipefail

repo_root="$(pwd)"
target_name="ume"
run_after_build=false
force_configure=false
preset=""
run_args=()

usage() {
  echo "Usage: $0 [-r|--run] [-c|--configure] <preset> [-- <args passed to the executable>]" >&2
  exit 1
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -r|--run)
      run_after_build=true
      shift
      ;;
    -c|--configure)
      force_configure=true
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

rel_build_dir="build/$preset"
build_dir="$repo_root/$rel_build_dir"

if "$force_configure" || [[ ! -f "$build_dir/CMakeCache.txt" ]]; then
  cmake --preset "$preset"
fi

cmake --build --preset "$preset"

cc_link="$repo_root/compile_commands.json"
cc_target="$rel_build_dir/compile_commands.json"
if [[ -e "$build_dir/compile_commands.json" ]]; then
  current="$(readlink "$cc_link" 2>/dev/null || true)"
  if [[ "$current" != "$cc_target" ]]; then
    ln -sfn "$cc_target" "$cc_link"
  fi
fi

if "$run_after_build"; then
  binary="$build_dir/$target_name"
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