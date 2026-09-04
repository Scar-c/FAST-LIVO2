#!/usr/bin/env bash

# Shared runner classification.  A completed artifact never changes the
# process return code: 139 remains 139 in the metadata, while the runner's
# own return code is non-zero unless the process exited cleanly.

native_runner_complete_sentinel() {
  local sentinel="$1"
  local require_eof="${2:-0}"
  [[ -s "$sentinel" ]] || return 1
  grep -qx 'processing_complete: 1' "$sentinel" || return 1
  grep -qx 'trajectory_flushed: 1' "$sentinel" || return 1
  grep -qx 'counters_flushed: 1' "$sentinel" || return 1
  grep -qx 'input_queues_drained: 1' "$sentinel" || return 1
  grep -qx 'no_processable_epoch_remaining: 1' "$sentinel" || return 1
  grep -qx 'expected_final_epoch_reached: 1' "$sentinel" || return 1
  if [[ "$require_eof" -eq 1 ]]; then
    grep -qx 'eof_drained: 1' "$sentinel" || return 1
  fi
}

native_runner_classify() {
  local node_rc="$1"
  local source_rc="$2"
  local sentinel="$3"
  local require_eof="${4:-0}"

  if [[ "$node_rc" -eq 124 || "$source_rc" -eq 124 ]]; then
    echo TIMEOUT
  elif [[ "$node_rc" -eq 130 || "$node_rc" -eq 143 ||
          "$source_rc" -eq 130 || "$source_rc" -eq 143 ]]; then
    echo CANCELLED
  elif [[ "$source_rc" -ne 0 ]]; then
    echo CONTAMINATED
  elif native_runner_complete_sentinel "$sentinel" "$require_eof"; then
    if [[ "$node_rc" -eq 0 ]]; then
      echo CLEAN_SUCCESS
    elif [[ "$node_rc" -eq 139 ]]; then
      echo PROCESSING_COMPLETE_WITH_SHUTDOWN_FAULT
    else
      echo INCOMPLETE_CRASH
    fi
  else
    echo INCOMPLETE_CRASH
  fi
}

native_runner_exit_code() {
  [[ "$1" == CLEAN_SUCCESS ]] && echo 0 || echo 1
}
