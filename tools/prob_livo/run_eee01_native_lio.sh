#!/usr/bin/env bash
# Prompt10 runner for the untouched FAST-LIVO2 native LIO baseline.
# The source and build are supplied by a detached worktree; this script only
# launches ROS, replays the bag, samples /proc, and collects the pose file.

set -u
set -o pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
NATIVE_ROOT="${PROB_LIVO_NATIVE_ROOT:-/tmp/prompt10_fast_livo2_native}"
NATIVE_WS="${PROB_LIVO_NATIVE_WS:-/tmp/prompt10_native_ws}"
NATIVE_HEAD="0d2c0346107b75b59934975adec9a6eeeb913c64"
BAG="${1:-$REPO_ROOT/../../bag/NTU/eee_01/eee_01.bag}"
RUN_ROOT="${PROB_LIVO_RUN_ROOT:-$REPO_ROOT/results/prob_livo/runs}"
RUN_ID="${PROB_LIVO_RUN_ID:-native_$(date +%Y%m%d_%H%M%S)}"
RATE="${PROB_LIVO_BAG_RATE:-1.0}"
MEMORY_CSV="${PROB_LIVO_MEMORY_CSV:-}"
MEMORY_LABEL="${PROB_LIVO_MEMORY_LABEL:-N0}"
MEMORY_INTERVAL="${PROB_LIVO_MEMORY_INTERVAL:-2}"
CONFIG="$NATIVE_ROOT/config/NTU_VIRAL.yaml"
CAMERA_CONFIG="$NATIVE_ROOT/config/camera_NTU_VIRAL.yaml"
NATIVE_RESULT="$NATIVE_ROOT/Log/result/$RUN_ID.txt"

if [[ ! -f "$BAG" || ! -f "$CONFIG" || ! -f "$CAMERA_CONFIG" || \
      ! -x "$NATIVE_WS/devel/lib/fast_livo/fastlivo_mapping" ]]; then
  echo "ERR: missing bag, native config, or native binary" >&2
  exit 2
fi
if [[ ! "$RUN_ID" =~ ^[A-Za-z0-9_.-]+$ ]]; then
  echo "ERR: invalid run id" >&2
  exit 2
fi
if [[ "$(git -C "$NATIVE_ROOT" rev-parse HEAD 2>/dev/null || true)" != "$NATIVE_HEAD" ]]; then
  echo "ERR: native worktree is not at the Prompt10 host baseline" >&2
  exit 3
fi
if [[ -n "$(git -C "$NATIVE_ROOT" status --short)" ]]; then
  echo "ERR: native worktree is dirty" >&2
  exit 3
fi
if [[ -e "$NATIVE_RESULT" ]]; then
  echo "ERR: refusing to overwrite existing native result $NATIVE_RESULT" >&2
  exit 2
fi

RUN_DIR="$RUN_ROOT/$RUN_ID"
if [[ -e "$RUN_DIR" ]]; then
  echo "ERR: refusing to overwrite $RUN_DIR" >&2
  exit 2
fi
mkdir -p "$RUN_DIR/ros_log" "$RUN_DIR/ros_home"
mkdir -p "$NATIVE_ROOT/Log/result" "$NATIVE_ROOT/Log/pcd" "$NATIVE_ROOT/Log/image"
export ROS_HOME="$RUN_DIR/ros_home"
export ROS_LOG_DIR="$RUN_DIR/ros_log"
source /opt/ros/noetic/setup.bash
source /home/lc/design_ws/devel/setup.bash
source "$NATIVE_WS/devel/setup.bash"

CORE_PID=""
NODE_PID=""
MEMORY_MONITOR_PID=""
cleanup() {
  [[ -n "$NODE_PID" ]] && kill -INT "$NODE_PID" 2>/dev/null || true
  [[ -n "$MEMORY_MONITOR_PID" ]] && kill "$MEMORY_MONITOR_PID" 2>/dev/null || true
  [[ -n "$CORE_PID" ]] && kill "$CORE_PID" 2>/dev/null || true
}
trap cleanup EXIT

MASTER_PORT=$((11311 + RANDOM % 200))
export ROS_MASTER_URI="http://localhost:$MASTER_PORT"
roscore -p "$MASTER_PORT" >"$RUN_DIR/roscore.log" 2>&1 &
CORE_PID=$!
for _ in $(seq 1 30); do
  rosnode list >/dev/null 2>&1 && break
  sleep 1
done
if ! rosnode list >/dev/null 2>&1; then
  echo "ERR: roscore did not start" >&2
  exit 2
fi

rosparam load "$CONFIG"
rosparam load "$CAMERA_CONFIG" /laserMapping
rosparam set /common/img_en 0
rosparam set /common/lidar_en 1
rosparam set /imu/imu_en true
rosparam set /evo/pose_output_en true
rosparam set /evo/seq_name "$RUN_ID"
rosparam set /publish/dense_map_en true
rosparam set /publish/pub_effect_point_en false
rosparam set /publish/pub_plane_en false
rosparam set /pcd_save/pcd_save_en false
rosparam set /pcd_save/colmap_output_en false
rosparam set /image_save/img_save_en false
rosparam dump "$RUN_DIR/effective_rosparams.yaml"
if [[ -n "$MEMORY_CSV" ]]; then
  mkdir -p "$(dirname "$MEMORY_CSV")"
fi

{
  echo "repository_root: $REPO_ROOT"
  echo "native_worktree: $NATIVE_ROOT"
  echo "native_workspace: $NATIVE_WS"
  echo "native_git_head: $NATIVE_HEAD"
  echo "native_git_dirty: no"
  echo "bag: $BAG"
  echo "bag_sha256: $(sha256sum "$BAG" | cut -d' ' -f1)"
  echo "config: $CONFIG"
  echo "config_sha256: $(sha256sum "$CONFIG" | cut -d' ' -f1)"
  echo "camera_config: $CAMERA_CONFIG"
  echo "camera_config_sha256: $(sha256sum "$CAMERA_CONFIG" | cut -d' ' -f1)"
  echo "backend: FAST-LIVO2 native LIO"
  echo "camera_mode: off"
  echo "visual_state: OFF"
  echo "input_semantics: native"
  echo "replayed_topics: /imu/imu,/os1_cloud_node1/points"
  echo "bag_rate: $RATE"
  echo "ros_master_uri: $ROS_MASTER_URI"
  echo "effective_rosparams: $RUN_DIR/effective_rosparams.yaml"
  echo "native_result: $NATIVE_RESULT"
  echo "memory_csv: ${MEMORY_CSV:-none}"
  echo "memory_label: ${MEMORY_CSV:+$MEMORY_LABEL}"
  echo "memory_interval_seconds: ${MEMORY_CSV:+$MEMORY_INTERVAL}"
  echo "start_utc: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
} >"$RUN_DIR/meta.txt"

rosrun fast_livo fastlivo_mapping __name:=laserMapping >"$RUN_DIR/node.log" 2>&1 &
NODE_PID=$!
sleep 3
if ! kill -0 "$NODE_PID" 2>/dev/null; then
  echo "ERR: fastlivo_mapping exited during startup" >&2
  exit 4
fi
if [[ -n "$MEMORY_CSV" ]]; then
  python3 "$REPO_ROOT/tools/prob_livo/memory_monitor.py" \
    --pid "$NODE_PID" --csv "$MEMORY_CSV" --label "$MEMORY_LABEL" \
    --interval "$MEMORY_INTERVAL" >"$RUN_DIR/memory_monitor.log" 2>&1 &
  MEMORY_MONITOR_PID=$!
fi

RUN_START_EPOCH=$(date +%s)
rosbag play "$BAG" --clock --rate "$RATE" --topics \
  /imu/imu /os1_cloud_node1/points >"$RUN_DIR/play.log" 2>&1
PLAY_RC=$?
RUN_END_EPOCH=$(date +%s)
sleep 3
kill -INT "$NODE_PID" 2>/dev/null || true
wait "$NODE_PID" 2>/dev/null
NODE_RC=$?
NODE_PID=""
MEMORY_RC=0
if [[ -n "$MEMORY_MONITOR_PID" ]]; then
  wait "$MEMORY_MONITOR_PID" 2>/dev/null
  MEMORY_RC=$?
  MEMORY_MONITOR_PID=""
fi

if [[ -s "$NATIVE_RESULT" ]]; then
  cp -- "$NATIVE_RESULT" "$RUN_DIR/trajectory.tum"
  TRAJECTORY_RC=0
else
  TRAJECTORY_RC=2
fi
NODE_RC_ACCEPTED=0
if [[ "$NODE_RC" -eq 0 || "$NODE_RC" -eq 130 || "$NODE_RC" -eq 139 ]]; then
  NODE_RC_ACCEPTED=1
fi
python3 "$REPO_ROOT/eval/prob_livo/pose_bag_to_tum.py" \
  --bag "$BAG" --topic /leica/pose/relative --output "$RUN_DIR/ground_truth.tum" \
  >"$RUN_DIR/ground_truth.log" 2>&1
GT_RC=$?
if [[ -s "$RUN_DIR/trajectory.tum" && "$GT_RC" -eq 0 ]]; then
  python3 "$REPO_ROOT/eval/prob_livo/eval_ntu_viral_official.py" \
    "$RUN_DIR/trajectory.tum" "$RUN_DIR/ground_truth.tum" \
    --out "$RUN_DIR/evaluation.yaml" >"$RUN_DIR/evaluation.log" 2>&1
  EVAL_RC=$?
else
  EVAL_RC=2
fi

if [[ "$PLAY_RC" -eq 0 && "$NODE_RC_ACCEPTED" -eq 1 && \
      "$TRAJECTORY_RC" -eq 0 && "$MEMORY_RC" -eq 0 && "$GT_RC" -eq 0 && \
      "$EVAL_RC" -eq 0 ]]; then
  RC=0
else
  RC=1
fi
{
  echo "play_rc: $PLAY_RC"
  echo "node_rc: $NODE_RC"
  echo "node_rc_accepted: $NODE_RC_ACCEPTED"
  echo "trajectory_rc: $TRAJECTORY_RC"
  echo "memory_rc: $MEMORY_RC"
  echo "ground_truth_rc: $GT_RC"
  echo "evaluation_rc: $EVAL_RC"
  echo "trajectory_rows: $(wc -l < "$RUN_DIR/trajectory.tum" 2>/dev/null || echo 0)"
  echo "trajectory_sha256: $(sha256sum "$RUN_DIR/trajectory.tum" 2>/dev/null | cut -d' ' -f1)"
  echo "runtime_seconds: $((RUN_END_EPOCH - RUN_START_EPOCH))"
  echo "end_utc: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "run_rc: $RC"
} >>"$RUN_DIR/meta.txt"
echo "run_dir: $RUN_DIR"
echo "__PROMPT10_NATIVE_RUN_DONE_RC=$RC"
exit "$RC"
