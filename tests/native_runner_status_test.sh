#!/usr/bin/env bash
set -u
set -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../tools/native_runner_status.sh"

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

assert_status() {
  local expected="$1"
  local node_rc="$2"
  local source_rc="$3"
  local sentinel="$4"
  local actual
  actual="$(native_runner_classify "$node_rc" "$source_rc" "$sentinel")"
  if [[ "$actual" != "$expected" ]]; then
    echo "expected $expected, got $actual for node_rc=$node_rc source_rc=$source_rc sentinel=$sentinel" >&2
    exit 1
  fi
}

assert_status INCOMPLETE_CRASH 139 0 "$TMP_DIR/missing.sentinel"

cat >"$TMP_DIR/complete.sentinel" <<'EOF'
processing_complete: 1
eof_drained: 1
trajectory_flushed: 1
counters_flushed: 1
input_queues_drained: 1
no_processable_epoch_remaining: 1
expected_final_epoch_reached: 1
EOF

assert_status CLEAN_SUCCESS 0 0 "$TMP_DIR/complete.sentinel"
assert_status PROCESSING_COMPLETE_WITH_SHUTDOWN_FAULT 139 0 "$TMP_DIR/complete.sentinel"
assert_status TIMEOUT 124 0 "$TMP_DIR/complete.sentinel"
assert_status CANCELLED 130 0 "$TMP_DIR/complete.sentinel"
assert_status CONTAMINATED 0 2 "$TMP_DIR/complete.sentinel"

[[ "$(native_runner_exit_code CLEAN_SUCCESS)" == 0 ]]
[[ "$(native_runner_exit_code PROCESSING_COMPLETE_WITH_SHUTDOWN_FAULT)" == 1 ]]

echo "native runner status tests passed"
