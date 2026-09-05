#!/usr/bin/env bash
# Run the predeclared Prompt15 offline matrix in ledger order.
# Ledger rows remain immutable during execution; the collector updates their
# status only after artifacts have been inspected.
set -u
set -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
LEDGER="${PROMPT15_LEDGER:-$REPO_ROOT/spec/prob_livo/PROMPT15_RUN_LEDGER.csv}"
RUN_ROOT="${PROMPT15_RUN_ROOT:-$REPO_ROOT/results/prob_livo/prompt15/formal}"
NATIVE_ROOT="${PROMPT15_NATIVE_ROOT:-/home/lc/super_livo/p15_native_FAST-LIVO2}"
NATIVE_WS="${PROMPT15_NATIVE_WS:-/tmp/prompt11_native_ws}"
CPUSET="${PROMPT15_CPUSET:-0,2,4,6}"
TIMEOUT_SECONDS="${PROMPT15_TIMEOUT_SECONDS:-1800}"
SLOT_RUNNER="$SCRIPT_DIR/run_prompt15_slot.sh"
DRIVER_LOG="$RUN_ROOT/matrix_driver.log"

if [[ ! -s "$LEDGER" || ! -x "$SLOT_RUNNER" ]]; then
  echo "ERR: missing Prompt15 ledger or slot runner" >&2
  exit 2
fi
mkdir -p "$RUN_ROOT"

{
  echo "prompt15_matrix_start_utc: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "ledger: $LEDGER"
  echo "run_root: $RUN_ROOT"
  echo "cpuset: $CPUSET"
  echo "timeout_seconds: $TIMEOUT_SECONDS"
} >>"$DRIVER_LOG"

completed=0
failed=0
while IFS=, read -r execution_index slot_id dataset sequence variant repetition \
    order_block position_in_block status replaces_run_id actual_run_id notes; do
  [[ "$execution_index" == execution_index ]] && continue
  [[ "$status" == PENDING ]] || continue
  echo "slot_start index=$execution_index slot=$slot_id dataset=$dataset sequence=$sequence variant=$variant repetition=$repetition" \
    | tee -a "$DRIVER_LOG"
  env PROMPT15_NATIVE_ROOT="$NATIVE_ROOT" \
      PROMPT15_NATIVE_WS="$NATIVE_WS" \
      PROMPT15_RUN_ROOT="$RUN_ROOT" \
      PROMPT15_CPUSET="$CPUSET" \
      PROMPT15_TIMEOUT_SECONDS="$TIMEOUT_SECONDS" \
      timeout --signal=INT --kill-after=30s "$TIMEOUT_SECONDS" \
      "$SLOT_RUNNER" "$dataset" "$sequence" "$variant" "$slot_id" \
      >>"$DRIVER_LOG" 2>&1
  rc=$?
  if [[ "$rc" -eq 0 ]]; then
    ((completed += 1))
  else
    ((failed += 1))
  fi
  echo "slot_end index=$execution_index slot=$slot_id rc=$rc" | tee -a "$DRIVER_LOG"
done <"$LEDGER"

{
  echo "prompt15_matrix_end_utc: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "completed_valid_slots: $completed"
  echo "failed_slots: $failed"
} | tee -a "$DRIVER_LOG"

[[ "$failed" -eq 0 ]]
