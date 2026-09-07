#!/usr/bin/env bash
# Admit a Prompt22 P/N pair only after shared config and, for LIVO, camera
# timestamp identity both pass.
set -u
set -o pipefail

if [[ $# -ne 3 ]]; then
  echo "usage: validate_prompt22_pair.sh NATIVE_RUN_DIR PROB_RUN_DIR lio|livo" >&2
  exit 2
fi

NATIVE_DIR="$1"
PROB_DIR="$2"
MODE="$3"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

python3 "$REPO_ROOT/tools/prob_livo/validate_prompt22_config.py" \
  --native "$NATIVE_DIR" --prob "$PROB_DIR" --mode "$MODE"
RC=$?
if [[ "$RC" -ne 0 ]]; then
  exit "$RC"
fi

if [[ "$MODE" == livo ]]; then
  if ! cmp -s "$NATIVE_DIR/selected_camera_timestamps.txt" \
           "$PROB_DIR/selected_camera_timestamps.txt"; then
    echo "STRIDE_INPUT_IDENTITY_FAIL: selected camera timestamp streams differ" >&2
    exit 1
  fi
  NATIVE_COUNT=$(wc -l < "$NATIVE_DIR/selected_camera_timestamps.txt")
  PROB_COUNT=$(wc -l < "$PROB_DIR/selected_camera_timestamps.txt")
  if [[ "$NATIVE_COUNT" -ne "$PROB_COUNT" ]]; then
    echo "STRIDE_INPUT_IDENTITY_FAIL: selected camera counts differ" >&2
    exit 1
  fi
  echo "PROMPT22 selected camera identity: PASS count=$NATIVE_COUNT"
fi

for run_dir in "$NATIVE_DIR" "$PROB_DIR"; do
  {
    echo "prompt22_config_identity: PASS"
    if [[ "$MODE" == livo ]]; then
      echo "prompt22_stride_input_identity: PASS"
    fi
    echo "prompt22_gate_status: PASS"
  } >"$run_dir/prompt22_gate.txt"
done
echo "PROMPT22 pair admission: PASS"
