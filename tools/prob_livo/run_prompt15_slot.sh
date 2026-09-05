#!/usr/bin/env bash
# Execute one Prompt15 formal offline slot.  The caller supplies the already
# predeclared slot id, so failures remain observable and are never replaced in
# place or silently removed.
set -u
set -o pipefail

if [[ $# -ne 4 ]]; then
  echo "usage: run_prompt15_slot.sh DATASET SEQUENCE VARIANT RUN_ID" >&2
  exit 2
fi

DATASET="$1"
SEQUENCE="$2"
VARIANT="$3"
RUN_ID="$4"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
NATIVE_ROOT="${PROMPT15_NATIVE_ROOT:-/home/lc/super_livo/p15_native_FAST-LIVO2}"
NATIVE_WS="${PROMPT15_NATIVE_WS:-/tmp/prompt11_native_ws}"
RUN_ROOT="${PROMPT15_RUN_ROOT:-$REPO_ROOT/results/prob_livo/prompt15/formal}"
CPUSET="${PROMPT15_CPUSET:-0,2,4,6}"
TIMEOUT_SECONDS="${PROMPT15_TIMEOUT_SECONDS:-1800}"

case "$DATASET" in
  NTU)
    BAG="$REPO_ROOT/../../bag/NTU/$SEQUENCE/$SEQUENCE.bag"
    CONFIG="$REPO_ROOT/config/NTU_VIRAL.yaml"
    CAMERA_CONFIG="$REPO_ROOT/config/camera_NTU_VIRAL.yaml"
    GT_PATH=""
    ;;
  OXFORD)
    BAG="$REPO_ROOT/../../bag/OXFORD/$SEQUENCE/${SEQUENCE}_LIVO.bag"
    CONFIG="$REPO_ROOT/config/OXFORD_SPIRES.yaml"
    CAMERA_CONFIG="$REPO_ROOT/config/camera_OXFORD_SPIRES.yaml"
    GT_PATH="$REPO_ROOT/../../bag/OXFORD/$SEQUENCE/gt-tum.txt"
    ;;
  *) echo "ERR: unsupported dataset $DATASET" >&2; exit 2 ;;
esac

if [[ ! "$RUN_ID" =~ ^[A-Za-z0-9_.-]+$ ]]; then
  echo "ERR: invalid run id" >&2
  exit 2
fi
if [[ ! -f "$BAG" || ! -f "$CONFIG" || ! -f "$CAMERA_CONFIG" ]]; then
  echo "ERR: missing bag/config for $RUN_ID" >&2
  exit 2
fi

mkdir -p "$RUN_ROOT"
RUN_DIR="$RUN_ROOT/$RUN_ID"
if [[ -e "$RUN_DIR" ]]; then
  echo "ERR: refusing to overwrite $RUN_DIR" >&2
  exit 2
fi

CAMERA_MODE=off
if [[ "$VARIANT" == *LIVO ]]; then CAMERA_MODE=h1; fi
WRAPPER_RC=1
if [[ "$VARIANT" == N-* ]]; then
  MODE=lio
  [[ "$VARIANT" == N-LIVO ]] && MODE=livo
  FAST_LIVO_MODE="$MODE" \
  FAST_LIVO_CPUSET="$CPUSET" \
  FAST_LIVO_NATIVE_WS="$NATIVE_WS" \
  FAST_LIVO_EVAL_ROOT="$REPO_ROOT" \
  FAST_LIVO_CONFIG="$CONFIG" \
  FAST_LIVO_CAMERA_CONFIG="$CAMERA_CONFIG" \
  FAST_LIVO_RUN_ROOT="$RUN_ROOT" \
  FAST_LIVO_RUN_ID="$RUN_ID" \
  timeout --signal=INT --kill-after=30s "$TIMEOUT_SECONDS" \
    "$NATIVE_ROOT/tools/run_native_fast_livo2_offline.sh" "$BAG"
  WRAPPER_RC=$?
else
  PROB_LIVO_INPUT_SEMANTICS=fast_native \
  PROB_LIVO_CAMERA_MODE="$CAMERA_MODE" \
  PROB_LIVO_WORKERS=4 \
  PROB_LIVO_CPUSET="$CPUSET" \
  PROB_LIVO_CONFIG="$CONFIG" \
  PROB_LIVO_CAMERA_CONFIG="$CAMERA_CONFIG" \
  PROB_LIVO_DATASET_FAMILY="$DATASET" \
  PROB_LIVO_GT_PATH="$GT_PATH" \
  PROB_LIVO_RUN_ROOT="$RUN_ROOT" \
  PROB_LIVO_RUN_ID="$RUN_ID" \
  timeout --signal=INT --kill-after=30s "$TIMEOUT_SECONDS" \
    "$REPO_ROOT/tools/prob_livo/run_eee01_camera_offline.sh" "$BAG"
  WRAPPER_RC=$?
fi

EVAL_RC=0
if [[ "$VARIANT" == N-* ]]; then
  if [[ "$DATASET" == NTU ]]; then
    python3 "$REPO_ROOT/eval/prob_livo/pose_bag_to_tum.py" \
      --bag "$BAG" --topic /leica/pose/relative \
      --output "$RUN_DIR/ground_truth.tum" \
      >"$RUN_DIR/ground_truth.log" 2>&1
    GT_RC=$?
  else
    cp "$GT_PATH" "$RUN_DIR/ground_truth.tum"
    GT_RC=$?
  fi
  if [[ "$GT_RC" -eq 0 && -s "$RUN_DIR/trajectory.tum" ]]; then
    if [[ "$DATASET" == NTU ]]; then
      python3 "$REPO_ROOT/eval/prob_livo/eval_ntu_viral_official.py" \
        "$RUN_DIR/trajectory.tum" "$RUN_DIR/ground_truth.tum" \
        --out "$RUN_DIR/evaluation.yaml" \
        >"$RUN_DIR/evaluation.log" 2>&1
    else
      python3 "$REPO_ROOT/eval/prob_livo/eval_tum_translation.py" \
        "$RUN_DIR/trajectory.tum" "$RUN_DIR/ground_truth.tum" \
        --frame body --max-diff 0.05 --out "$RUN_DIR/evaluation.txt" \
        >"$RUN_DIR/evaluation.log" 2>&1
    fi
    EVAL_RC=$?
  else
    EVAL_RC=2
  fi
else
  if [[ "$DATASET" == OXFORD && -s "$RUN_DIR/trajectory.tum" &&
        ! -s "$RUN_DIR/evaluation.txt" ]]; then
    EVAL_RC=2
  elif [[ "$DATASET" == NTU && -s "$RUN_DIR/trajectory.tum" &&
          ! -s "$RUN_DIR/evaluation.yaml" ]]; then
    EVAL_RC=2
  fi
fi

if [[ -s "$RUN_DIR/processing_complete.sentinel" &&
      -s "$RUN_DIR/trajectory.tum" && "$WRAPPER_RC" -eq 0 &&
      "$EVAL_RC" -eq 0 ]]; then
  FORMAL_STATUS=VALID
else
  FORMAL_STATUS=EXECUTION_FAIL
fi
if [[ -d "$RUN_DIR" ]]; then
  {
    echo "formal_wrapper_rc: $WRAPPER_RC"
    echo "formal_evaluator_rc: $EVAL_RC"
    echo "formal_status: $FORMAL_STATUS"
  } >>"$RUN_DIR/meta.txt"
fi
echo "run_dir: $RUN_DIR"
echo "formal_status: $FORMAL_STATUS"
[[ "$FORMAL_STATUS" == VALID ]] && exit 0
exit 1
