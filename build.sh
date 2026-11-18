#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
run_after_build=false
force_configure=false
target="all"
config=""
positional=()
run_args=()

usage() {
  echo "Usage: $0 [-r|--run] [-c|--configure] [engine|examples|all] <debug|release> [-- <args passed to ume>]" >&2
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
      positional+=("$1")
      shift
      ;;
  esac
done

case "${#positional[@]}" in
  1) config="${positional[0]}" ;;
  2) target="${positional[0]}"; config="${positional[1]}" ;;
  *) usage ;;
esac

case "$target" in engine|examples|all) ;; *) usage ;; esac
case "$config" in debug|release) ;; *) usage ;; esac

engine_preset="${config}-metal"
examples_preset="$config"

# configure if needed, build, and point <src>/compile_commands.json at the
# build. subshells keep the caller's working directory unchanged, so relative
# paths in run_args still resolve against where the script was invoked
build_project() {
  local src="$1" preset="$2"
  local rel_build_dir="build/$preset"

  if "$force_configure" || [[ ! -f "$src/$rel_build_dir/CMakeCache.txt" ]]; then
    (cd "$src" && cmake --preset "$preset")
  fi
  (cd "$src" && cmake --build --preset "$preset")

  local cc_link="$src/compile_commands.json"
  local cc_target="$rel_build_dir/compile_commands.json"
  if [[ -e "$src/$cc_target" ]]; then
    local current
    current="$(readlink "$cc_link" 2>/dev/null || true)"
    if [[ "$current" != "$cc_target" ]]; then
      ln -sfn "$cc_target" "$cc_link"
    fi
  fi
}

if [[ "$target" == engine || "$target" == all ]]; then
  build_project "$repo_root" "$engine_preset"
fi

if [[ "$target" == examples || "$target" == all ]]; then
  build_project "$repo_root/examples" "$examples_preset"
fi

if "$run_after_build"; then
  binary="$repo_root/build/$engine_preset/ume"
  if [[ ! -x "$binary" ]]; then
    echo "error: expected executable at $binary but it wasn't found" >&2
    exit 1
  fi

  exec "$binary" "${run_args[@]+"${run_args[@]}"}"
fi
