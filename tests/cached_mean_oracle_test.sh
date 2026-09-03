#!/usr/bin/env bash
set -euo pipefail

TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

# The legacy production pattern must be rejected by the compiler's
# maybe-uninitialized diagnostic. This is the independent proof that the old
# implementation is unsafe, not a runtime test that relies on stack layout.
if g++ -O2 -std=c++17 -Wall -Wextra -Werror=maybe-uninitialized \
    -c "$TEST_DIR/cached_mean_legacy.cpp" -o "$TMP_DIR/legacy.o" \
    >"$TMP_DIR/legacy.stdout" 2>"$TMP_DIR/legacy.stderr"; then
  echo "legacy cached-mean implementation unexpectedly compiled" >&2
  exit 1
fi
grep -q "maybe-uninitialized\|uninitialized" "$TMP_DIR/legacy.stderr"

g++ -O2 -std=c++17 -Wall -Wextra \
  "$TEST_DIR/cached_mean_oracle.cpp" -o "$TMP_DIR/oracle"
"$TMP_DIR/oracle"

SOURCE_FILE="$TEST_DIR/../src/vio.cpp"
grep -q 'float ref_mean = ref_patch_temp->mean_;' "$SOURCE_FILE"
grep -q 'float other_mean = (\*itm)->mean_;' "$SOURCE_FILE"

echo "cached-mean legacy red proof and cached-path oracle passed"
