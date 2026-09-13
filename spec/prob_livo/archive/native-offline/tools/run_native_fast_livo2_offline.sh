#!/usr/bin/env bash
# Rosbag event source for the unified native FAST-LIVO2 runner.
# Only transport changes: each bag record invokes the native callback and then
# the same ProcessAvailableNativeEpochs() seam used by normal ROS online.

set -u
set -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/native_runner_status.sh"

NATIVE_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOST_ROOT="${FAST_LIVO_EVAL_ROOT:-/home/lc/super_livo/src/FAST-LIVO2}"
NATIVE_WS="${FAST_LIVO_NATIVE_WS:-/tmp/prompt11_native_ws}"
NATIVE_DEVEL="${FAST_LIVO_NATIVE_DEVEL:-$NATIVE_WS/devel}"
BAG="${1:-/home/lc/super_livo/bag/NTU/eee_01/eee_01.bag}"
MODE="${FAST_LIVO_MODE:-lio}"
RUN_ROOT="${FAST_LIVO_RUN_ROOT:-$HOST_ROOT/results/prob_livo/runs}"
RUN_ID="${FAST_LIVO_RUN_ID:-native_${MODE}_offline_$(date +%Y%m%d_%H%M%S)}"
CONFIG="${FAST_LIVO_CONFIG:-$NATIVE_ROOT/config/NTU_VIRAL.yaml}"
CAMERA_CONFIG="${FAST_LIVO_CAMERA_CONFIG:-$NATIVE_ROOT/config/camera_NTU_VIRAL.yaml}"
CPUSET="${FAST_LIVO_CPUSET:-0,2,4,6}"
CAMERA_STRIDE="${FAST_LIVO_CAMERA_STRIDE:-1}"
VISUAL_MEMORY_STAGE="${FAST_LIVO_VISUAL_MEMORY_STAGE:-current}"
CROSS_MODAL_FEJ="${FAST_LIVO_CROSS_MODAL_FEJ:-0}"

if [[ ! "$CAMERA_STRIDE" =~ ^[1-9][0-9]*$ ]]; then
  echo "ERR: FAST_LIVO_CAMERA_STRIDE must be a positive integer" >&2
  exit 2
fi
case "$VISUAL_MEMORY_STAGE" in
  current|leak_fix) ;;
  *) echo "ERR: FAST_LIVO_VISUAL_MEMORY_STAGE must be current or leak_fix" >&2; exit 2 ;;
esac
export FAST_LIVO_CAMERA_STRIDE="$CAMERA_STRIDE"
case "$CROSS_MODAL_FEJ" in
  0|1|false|true|FALSE|TRUE|on|off) ;;
  *) echo "ERR: FAST_LIVO_CROSS_MODAL_FEJ must be boolean" >&2; exit 2 ;;
esac

NATIVE_OFFLINE_BINARY="${FAST_LIVO_NATIVE_BINARY:-$NATIVE_DEVEL/lib/fast_livo/fastlivo_native_offline}"
if [[ ! -x "$NATIVE_OFFLINE_BINARY" ]]; then
  NATIVE_OFFLINE_BINARY="$NATIVE_DEVEL/lib/fast_livo/fastlivo_native_offline"
fi

case "$MODE" in lio|livo) ;; *) echo "ERR: FAST_LIVO_MODE must be lio or livo" >&2; exit 2 ;; esac
if [[ ! -f "$BAG" || ! -f "$CONFIG" || ! -f "$CAMERA_CONFIG" ||
      ! -x "$NATIVE_OFFLINE_BINARY" ]]; then
  echo "ERR: missing bag/config/camera/native offline binary" >&2
  exit 2
fi
if [[ ! "$RUN_ID" =~ ^[A-Za-z0-9_.-]+$ ]]; then echo "ERR: invalid run id" >&2; exit 2; fi
if [[ -n "$(git -C "$NATIVE_ROOT" status --short)" ]]; then
  echo "ERR: native worktree must be clean" >&2
  exit 3
fi
RUN_DIR="$RUN_ROOT/$RUN_ID"
if [[ -e "$RUN_DIR" ]]; then echo "ERR: refusing to overwrite $RUN_DIR" >&2; exit 2; fi
mkdir -p "$RUN_DIR/ros_log" "$RUN_DIR/ros_home"
export ROS_HOME="$RUN_DIR/ros_home"
export ROS_LOG_DIR="$RUN_DIR/ros_log"
source /opt/ros/noetic/setup.bash
source "$NATIVE_DEVEL/setup.bash"

CORE_PID=""
cleanup() { [[ -n "$CORE_PID" ]] && kill "$CORE_PID" 2>/dev/null || true; }
trap cleanup EXIT
MASTER_PORT=$((11311 + RANDOM % 200))
export ROS_MASTER_URI="http://localhost:$MASTER_PORT"
roscore -p "$MASTER_PORT" >"$RUN_DIR/roscore.log" 2>&1 &
CORE_PID=$!
for _ in $(seq 1 30); do rosnode list >/dev/null 2>&1 && break; sleep 1; done
if ! rosnode list >/dev/null 2>&1; then echo "ERR: roscore did not start" >&2; exit 2; fi

rosparam load "$CONFIG"
rosparam load "$CAMERA_CONFIG" /laserMapping
rosparam set /common/img_en "$([[ "$MODE" == "livo" ]] && echo 1 || echo 0)"
rosparam set /common/lidar_en 1
rosparam set /imu/imu_en true
rosparam set /evo/pose_output_en true
rosparam set /pcd_save/pcd_save_en false
rosparam set /image_save/img_save_en false
rosparam set /publish/dense_map_en false
rosparam set /common/prob_livo_visual_memory_stage "$VISUAL_MEMORY_STAGE"
rosparam set /common/prob_livo_cross_modal_fej "$CROSS_MODAL_FEJ"
rosparam set /common/cross_modal_fej_diagnostics_path "$RUN_DIR/fej_diagnostics.csv"
rosparam dump "$RUN_DIR/effective_rosparams.yaml"
{
  echo "repository_root: $NATIVE_ROOT"
  echo "native_branch: $(git -C "$NATIVE_ROOT" branch --show-current)"
  echo "native_git_head: $(git -C "$NATIVE_ROOT" rev-parse HEAD)"
  echo "native_git_dirty: no"
  echo "bag: $BAG"
  echo "bag_sha256: $(sha256sum "$BAG" | cut -d' ' -f1)"
  echo "config: $CONFIG"
  echo "config_sha256: $(sha256sum "$CONFIG" | cut -d' ' -f1)"
  echo "camera_config: $CAMERA_CONFIG"
  echo "camera_config_sha256: $(sha256sum "$CAMERA_CONFIG" | cut -d' ' -f1)"
  echo "backend: FAST-LIVO2 native"
  echo "mode: $MODE"
  echo "camera: $([[ "$MODE" == "livo" ]] && echo ON || echo OFF)"
  echo "logical_cpu_affinity: $CPUSET"
  echo "worker_limit: 4"
  echo "camera_stride: $CAMERA_STRIDE"
  echo "visual_memory_stage: $VISUAL_MEMORY_STAGE"
  echo "cross_modal_fej: $CROSS_MODAL_FEJ"
  echo "build_type: Release"
  echo "build_flags: -O3 -march=native -mtune=native -mno-avx -funroll-loops EIGEN_MAX_ALIGN_BYTES=16 FAST_LIVO_MP_PROC_NUM=4"
  echo "event_source: offline_rosbag_record_order"
  echo "trajectory: $RUN_DIR/trajectory.tum"
  echo "effective_rosparams: $RUN_DIR/effective_rosparams.yaml"
} >"$RUN_DIR/meta.txt"

RUN_START=$(date +%s)
FAST_LIVO_VISUAL_MEMORY_STAGE="$VISUAL_MEMORY_STAGE" \
FAST_LIVO_CROSS_MODAL_FEJ="$CROSS_MODAL_FEJ" \
taskset -c "$CPUSET" "$NATIVE_OFFLINE_BINARY" \
  "$BAG" "$RUN_DIR" "$MODE" >"$RUN_DIR/node.log" 2>&1
NODE_RC=$?
RUN_END=$(date +%s)
RUN_STATUS="$(native_runner_classify "$NODE_RC" 0 \
  "$RUN_DIR/processing_complete.sentinel" 1)"
if [[ "$RUN_STATUS" == CLEAN_SUCCESS ||
      "$RUN_STATUS" == PROCESSING_COMPLETE_WITH_SHUTDOWN_FAULT ]]; then
  if [[ ! -s "$RUN_DIR/trajectory.tum" ||
        ! -s "$RUN_DIR/trajectory.tum.counters.yaml" ||
        ! -s "$RUN_DIR/trajectory.tum.timing.yaml" ||
        ! -s "$RUN_DIR/trajectory.tum.visual_counters.yaml" ||
        ! -s "$RUN_DIR/offline_source.yaml" ]]; then
    RUN_STATUS=INCOMPLETE_CRASH
  fi
fi
RC="$(native_runner_exit_code "$RUN_STATUS")"
NODE_RC_ACCEPTED=0
if [[ "$RUN_STATUS" == CLEAN_SUCCESS ||
      "$RUN_STATUS" == PROCESSING_COMPLETE_WITH_SHUTDOWN_FAULT ]]; then
  NODE_RC_ACCEPTED=1
fi
{
  echo "node_rc: $NODE_RC"
  echo "node_rc_accepted: $NODE_RC_ACCEPTED"
  echo "run_status: $RUN_STATUS"
  echo "trajectory_rows: $(wc -l < "$RUN_DIR/trajectory.tum" 2>/dev/null || echo 0)"
  echo "trajectory_sha256: $(sha256sum "$RUN_DIR/trajectory.tum" 2>/dev/null | cut -d' ' -f1)"
  echo "selected_camera_timestamps: $RUN_DIR/selected_camera_timestamps.txt"
  echo "selected_camera_timestamps_count: $(wc -l < "$RUN_DIR/selected_camera_timestamps.txt" 2>/dev/null || echo 0)"
  echo "selected_camera_timestamps_sha256: $(sha256sum "$RUN_DIR/selected_camera_timestamps.txt" 2>/dev/null | cut -d' ' -f1)"
  echo "processing_complete_sentinel: $RUN_DIR/processing_complete.sentinel"
  echo "runtime_seconds: $((RUN_END - RUN_START))"
  echo "run_rc: $RC"
} >>"$RUN_DIR/meta.txt"
echo "run_dir: $RUN_DIR"
echo "__FAST_LIVO_NATIVE_OFFLINE_DONE_RC=$RC"
exit "$RC"
