#!/usr/bin/env bash
# Normal ROS source for the unified native FAST-LIVO2 runner.
# The estimator sees the original subscriber callbacks and spinOnce loop.

set -u
set -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/native_runner_status.sh"

NATIVE_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOST_ROOT="${FAST_LIVO_EVAL_ROOT:-/home/lc/super_livo/src/FAST-LIVO2}"
NATIVE_WS="${FAST_LIVO_NATIVE_WS:-/tmp/prompt11_native_ws}"
BAG="${1:-/home/lc/super_livo/bag/NTU/eee_01/eee_01.bag}"
MODE="${FAST_LIVO_MODE:-lio}"
RUN_ROOT="${FAST_LIVO_RUN_ROOT:-$HOST_ROOT/results/prob_livo/runs}"
RUN_ID="${FAST_LIVO_RUN_ID:-native_${MODE}_online_$(date +%Y%m%d_%H%M%S)}"
RATE="${FAST_LIVO_BAG_RATE:-1.0}"
CONFIG="$NATIVE_ROOT/config/NTU_VIRAL.yaml"
CAMERA_CONFIG="$NATIVE_ROOT/config/camera_NTU_VIRAL.yaml"

case "$MODE" in lio|livo) ;; *) echo "ERR: FAST_LIVO_MODE must be lio or livo" >&2; exit 2 ;; esac
if [[ ! -f "$BAG" || ! -f "$CONFIG" || ! -f "$CAMERA_CONFIG" ||
      ! -x "$NATIVE_WS/devel/lib/fast_livo/fastlivo_mapping" ]]; then
  echo "ERR: missing bag/config/camera/native binary" >&2
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
source "$NATIVE_WS/devel/setup.bash"

CORE_PID=""
NODE_PID=""
cleanup() {
  [[ -n "$NODE_PID" ]] && kill -INT "$NODE_PID" 2>/dev/null || true
  [[ -n "$CORE_PID" ]] && kill "$CORE_PID" 2>/dev/null || true
}
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
rosparam set /evo/trajectory_output_path "$RUN_DIR/trajectory.tum"
rosparam set /evo/runtime_report_directory "$RUN_DIR"
rosparam set /pcd_save/pcd_save_en false
rosparam set /image_save/img_save_en false
rosparam set /publish/dense_map_en true
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
  echo "event_source: online_ros_subscribers"
  echo "replayed_topics: /imu/imu,/os1_cloud_node1/points$([[ "$MODE" != "livo" ]] || echo ,/left/image_raw)"
  echo "bag_rate: $RATE"
  echo "trajectory: $RUN_DIR/trajectory.tum"
  echo "effective_rosparams: $RUN_DIR/effective_rosparams.yaml"
} >"$RUN_DIR/meta.txt"

rosrun fast_livo fastlivo_mapping __name:=laserMapping >"$RUN_DIR/node.log" 2>&1 &
NODE_PID=$!
sleep 3
if ! kill -0 "$NODE_PID" 2>/dev/null; then echo "ERR: mapper exited during startup" >&2; exit 4; fi
RUN_START=$(date +%s)
TOPICS=(/imu/imu /os1_cloud_node1/points)
if [[ "$MODE" == "livo" ]]; then TOPICS+=(/left/image_raw); fi
rosbag play "$BAG" --clock --rate "$RATE" --topics "${TOPICS[@]}" >"$RUN_DIR/play.log" 2>&1
PLAY_RC=$?
RUN_END=$(date +%s)
if [[ "$PLAY_RC" -eq 0 ]]; then
  rosparam set /laserMapping/stop_after_input_drain true
fi
for _ in $(seq 1 900); do
  if ! kill -0 "$NODE_PID" 2>/dev/null; then break; fi
  sleep 1
done
if kill -0 "$NODE_PID" 2>/dev/null; then
  kill -INT "$NODE_PID" 2>/dev/null || true
fi
wait "$NODE_PID" 2>/dev/null
NODE_RC=$?
NODE_PID=""

RUN_STATUS="$(native_runner_classify "$NODE_RC" "$PLAY_RC" \
  "$RUN_DIR/processing_complete.sentinel" 1)"
if [[ "$RUN_STATUS" == CLEAN_SUCCESS ||
      "$RUN_STATUS" == PROCESSING_COMPLETE_WITH_SHUTDOWN_FAULT ]]; then
  if [[ ! -s "$RUN_DIR/trajectory.tum" ||
        ! -s "$RUN_DIR/trajectory.tum.counters.yaml" ||
        ! -s "$RUN_DIR/trajectory.tum.timing.yaml" ||
        ! -s "$RUN_DIR/trajectory.tum.visual_counters.yaml" ]]; then
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
  echo "play_rc: $PLAY_RC"
  echo "node_rc: $NODE_RC"
  echo "node_rc_accepted: $NODE_RC_ACCEPTED"
  echo "run_status: $RUN_STATUS"
  echo "trajectory_rows: $(wc -l < "$RUN_DIR/trajectory.tum" 2>/dev/null || echo 0)"
  echo "trajectory_sha256: $(sha256sum "$RUN_DIR/trajectory.tum" 2>/dev/null | cut -d' ' -f1)"
  echo "processing_complete_sentinel: $RUN_DIR/processing_complete.sentinel"
  echo "runtime_seconds: $((RUN_END - RUN_START))"
  echo "run_rc: $RC"
} >>"$RUN_DIR/meta.txt"
echo "run_dir: $RUN_DIR"
echo "__FAST_LIVO_NATIVE_ONLINE_DONE_RC=$RC"
exit "$RC"
