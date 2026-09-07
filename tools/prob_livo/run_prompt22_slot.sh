#!/usr/bin/env bash
# Execute one Prompt22 final controlled slot.  The caller pairs Native and
# Prob slots and then runs validate_prompt22_pair.sh before matrix admission.
set -u
set -o pipefail

if [[ $# -ne 5 ]]; then
  echo "usage: run_prompt22_slot.sh DATASET SEQUENCE ARM REP PHASE" >&2
  exit 2
fi

DATASET="$1"
SEQUENCE="$2"
ARM="$3"
REP="$4"
PHASE="$5"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
NATIVE_ROOT="${PROMPT22_NATIVE_ROOT:-/home/lc/super_livo/p15_native_FAST-LIVO2}"
NATIVE_WS="${PROMPT22_NATIVE_WS:-/tmp/prompt11_native_ws}"
NATIVE_DEVEL="${PROMPT22_NATIVE_DEVEL:-/home/lc/super_livo/devel_native}"
RUN_ROOT="${PROMPT22_RUN_ROOT:-$REPO_ROOT/results/prob_livo/prompt22/$PHASE}"
CPUSET="${PROMPT22_CPUSET:-0,2,4,6}"
TIMEOUT_SECONDS="${PROMPT22_TIMEOUT_SECONDS:-1800}"
RUN_TAG="${PROMPT22_RUN_TAG:-}"

case "$DATASET" in
  NTU)
    BAG="$REPO_ROOT/../../bag/NTU/$SEQUENCE/$SEQUENCE.bag"
    CONFIG="$REPO_ROOT/config/NTU_VIRAL.yaml"
    CAMERA_CONFIG="$REPO_ROOT/config/camera_NTU_VIRAL.yaml"
    GT_PATH=""
    STRIDE=1
    CONFIG_AUTHORITY="hku-mars_FAST-LIVO2_0d2c034_NTU_VIRAL"
    ;;
  OXFORD)
    BAG="$REPO_ROOT/../../bag/OXFORD/$SEQUENCE/${SEQUENCE}_LIVO.bag"
    CONFIG="$REPO_ROOT/config/prob_livo/OXFORD_OSD_FINAL_SHARED.yaml"
    CAMERA_CONFIG="$REPO_ROOT/config/camera_OXFORD_SPIRES.yaml"
    GT_PATH="$REPO_ROOT/../../bag/OXFORD/$SEQUENCE/gt-tum.txt"
    case "$SEQUENCE" in
      Church_05) STRIDE=1 ;;
      College_03|Palace_01|Quarter_01) STRIDE=2 ;;
      *) echo "ERR: unsupported Oxford sequence $SEQUENCE" >&2; exit 2 ;;
    esac
    CONFIG_AUTHORITY="ori-drs_FAST-LIVO2_config-used-OSD_f2c9abb_dataset-author"
    ;;
  *) echo "ERR: unsupported dataset $DATASET" >&2; exit 2 ;;
esac

case "$ARM" in
  N-LIO)
    NATIVE_MODE=lio
    CAMERA_MODE=off
    NATIVE=1
    ;;
  P-LIO)
    NATIVE_MODE=lio
    CAMERA_MODE=off
    NATIVE=0
    ;;
  N-LIVO-STRIDE)
    NATIVE_MODE=livo
    CAMERA_MODE=h1
    NATIVE=1
    ;;
  P-LIVO-STRIDE)
    NATIVE_MODE=livo
    CAMERA_MODE=h1
    NATIVE=0
    ;;
  *) echo "ERR: unsupported Prompt22 arm $ARM" >&2; exit 2 ;;
esac

if [[ ! "$REP" =~ ^[1-3]$ || ! "$PHASE" =~ ^(canary|formal)$ ]]; then
  echo "ERR: repetition must be 1..3 and phase must be canary/formal" >&2
  exit 2
fi
if [[ ! -f "$BAG" || ! -f "$CONFIG" || ! -f "$CAMERA_CONFIG" ]]; then
  echo "ERR: missing bag/config/camera for $DATASET $SEQUENCE $ARM" >&2
  exit 2
fi
if [[ ! "$SEQUENCE" =~ ^[A-Za-z0-9_.-]+$ || ! "$ARM" =~ ^[A-Za-z0-9_.-]+$ ]]; then
  echo "ERR: invalid sequence/arm" >&2
  exit 2
fi

RUN_ID="P22-${SEQUENCE}-${ARM}-r${REP}"
if [[ -n "$RUN_TAG" ]]; then
  if [[ ! "$RUN_TAG" =~ ^[A-Za-z0-9_.-]+$ ]]; then
    echo "ERR: invalid run tag" >&2
    exit 2
  fi
  RUN_ID="${RUN_ID}-${RUN_TAG}"
fi
RUN_DIR="$RUN_ROOT/$RUN_ID"
if [[ -e "$RUN_DIR" ]]; then
  echo "ERR: refusing to overwrite $RUN_DIR" >&2
  exit 2
fi

if [[ "$NATIVE" -eq 1 ]]; then
  FAST_LIVO_MODE="$NATIVE_MODE" \
  FAST_LIVO_CPUSET="$CPUSET" \
  FAST_LIVO_NATIVE_WS="$NATIVE_WS" \
  FAST_LIVO_NATIVE_DEVEL="$NATIVE_DEVEL" \
  FAST_LIVO_EVAL_ROOT="$REPO_ROOT" \
  FAST_LIVO_CONFIG="$CONFIG" \
  FAST_LIVO_CAMERA_CONFIG="$CAMERA_CONFIG" \
  FAST_LIVO_CAMERA_STRIDE="$STRIDE" \
  FAST_LIVO_VISUAL_MEMORY_STAGE=leak_fix \
  FAST_LIVO_RUN_ROOT="$RUN_ROOT" \
  FAST_LIVO_RUN_ID="$RUN_ID" \
  timeout --signal=INT --kill-after=30s "$TIMEOUT_SECONDS" \
    "$NATIVE_ROOT/tools/run_native_fast_livo2_offline.sh" "$BAG"
  WRAPPER_RC=$?
else
  PROB_LIVO_INPUT_SEMANTICS=fast_native \
  PROB_LIVO_CAMERA_MODE="$CAMERA_MODE" \
  PROB_LIVO_CAMERA_STRIDE="$STRIDE" \
  PROB_LIVO_WORKERS=4 \
  PROB_LIVO_CPUSET="$CPUSET" \
  PROB_LIVO_CONFIG="$CONFIG" \
  PROB_LIVO_CAMERA_CONFIG="$CAMERA_CONFIG" \
  PROB_LIVO_DATASET_FAMILY="$DATASET" \
  PROB_LIVO_GT_PATH="$GT_PATH" \
  PROB_LIVO_BUCKET_POLICY=native_blind_carry \
  PROB_LIVO_VISUAL_MEMORY_STAGE=parent_owned \
  PROB_LIVO_RUN_ROOT="$RUN_ROOT" \
  PROB_LIVO_RUN_ID="$RUN_ID" \
  timeout --signal=INT --kill-after=30s "$TIMEOUT_SECONDS" \
    "$REPO_ROOT/tools/prob_livo/run_eee01_camera_offline.sh" "$BAG"
  WRAPPER_RC=$?
fi

if [[ ! -d "$RUN_DIR" ]]; then
  echo "ERR: runner did not create $RUN_DIR" >&2
  exit 1
fi

EVAL_RC=0
if [[ "$NATIVE" -eq 1 ]]; then
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
  if [[ "$DATASET" == OXFORD && ! -s "$RUN_DIR/evaluation.txt" ]]; then
    EVAL_RC=2
  elif [[ "$DATASET" == NTU && ! -s "$RUN_DIR/evaluation.yaml" ]]; then
    EVAL_RC=2
  fi
fi

if [[ "$WRAPPER_RC" -eq 0 && "$EVAL_RC" -eq 0 && \
      -s "$RUN_DIR/processing_complete.sentinel" && \
      -s "$RUN_DIR/trajectory.tum" ]]; then
  RUN_STATUS=VALID
else
  RUN_STATUS=EXECUTION_FAIL
fi

TRAJECTORY_ROWS=0
[[ -s "$RUN_DIR/trajectory.tum" ]] && TRAJECTORY_ROWS=$(wc -l < "$RUN_DIR/trajectory.tum")
{
  echo "prompt22_dataset: $DATASET"
  echo "prompt22_sequence: $SEQUENCE"
  echo "prompt22_arm: $ARM"
  echo "prompt22_repetition: $REP"
  echo "prompt22_phase: $PHASE"
  echo "prompt22_stride: $STRIDE"
  echo "prompt22_config_authority: $CONFIG_AUTHORITY"
  echo "prompt22_config: $CONFIG"
  echo "prompt22_config_sha256: $(sha256sum "$CONFIG" | cut -d' ' -f1)"
  echo "prompt22_wrapper_rc: $WRAPPER_RC"
  echo "prompt22_evaluator_rc: $EVAL_RC"
  echo "prompt22_trajectory_rows: $TRAJECTORY_ROWS"
  echo "prompt22_run_status: $RUN_STATUS"
} >>"$RUN_DIR/meta.txt"

echo "run_dir: $RUN_DIR"
echo "prompt22_run_status: $RUN_STATUS"
[[ "$RUN_STATUS" == VALID ]] && exit 0
exit 1
