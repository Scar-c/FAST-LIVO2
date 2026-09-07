#!/usr/bin/env bash
# Run the predeclared Prompt22 Oxford 48-slot matrix serially and admit each
# P/N pair only after the shared-config and stride identity gates pass.
set -u
set -o pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ORDER_FILE="${PROMPT22_ORDER_FILE:-$REPO_ROOT/spec/prob_livo/PROMPT22_RUN_ORDER.csv}"
PROFILE="$REPO_ROOT/config/prob_livo/OXFORD_OSD_FINAL_SHARED.yaml"
CAMERA_PROFILE="$REPO_ROOT/config/camera_OXFORD_SPIRES.yaml"
SLOT_RUNNER="$REPO_ROOT/tools/prob_livo/run_prompt22_slot.sh"
PAIR_GATE="$REPO_ROOT/tools/prob_livo/validate_prompt22_pair.sh"

if [[ -n "$(git -C "$REPO_ROOT" status --short)" ]]; then
  echo "ERR: Prompt22 matrix requires a clean Prob worktree" >&2
  exit 3
fi

gate_pair() {
  local seq="$1" arm="$2" rep="$3"
  local native_arm prob_arm mode
  case "$arm" in
    N-LIO) native_arm=N-LIO; prob_arm=P-LIO; mode=lio ;;
    N-LIVO-STRIDE) native_arm=N-LIVO-STRIDE; prob_arm=P-LIVO-STRIDE; mode=livo ;;
    *) return 0 ;;
  esac
  local root="${PROMPT22_RUN_ROOT:-$REPO_ROOT/results/prob_livo/prompt22/formal}"
  local native_dir="$root/P22-${seq}-${native_arm}-r${rep}"
  local prob_dir="$root/P22-${seq}-${prob_arm}-r${rep}"
  if [[ ! -d "$native_dir" || ! -d "$prob_dir" ]]; then
    echo "ERR: pair directories missing for $seq rep $rep" >&2
    return 1
  fi
  bash "$PAIR_GATE" "$native_dir" "$prob_dir" "$mode" "$PROFILE" "$CAMERA_PROFILE"
}

tail -n +2 "$ORDER_FILE" | while IFS=, read -r order dataset sequence arm rep phase workers cpuset status; do
  [[ -z "$order" ]] && continue
  echo "[Prompt22] order=$order $dataset $sequence $arm r$rep"
  PROMPT22_CPUSET="$cpuset" PROMPT22_RUN_ROOT="${PROMPT22_RUN_ROOT:-$REPO_ROOT/results/prob_livo/prompt22/formal}" \
    bash "$SLOT_RUNNER" "$dataset" "$sequence" "$arm" "$rep" "$phase" || exit 1
  case "$arm" in
    N-LIO|N-LIVO-STRIDE)
      gate_pair "$sequence" "$arm" "$rep" || exit 1
      ;;
  esac
done

echo "PROMPT22 Oxford formal matrix execution and pair admission complete"
