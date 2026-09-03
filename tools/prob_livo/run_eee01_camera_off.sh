#!/usr/bin/env bash
# Canonical ROS Prob-LIVO runner for Prompt-3 camera-OFF and Prompt-8/9 H0/H1/H2.
# One FAST scheduler/node and one Prob OctVox backend are used. Every
# invocation writes a unique, self-describing run.

set -u

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
CATKIN_WS="${PROB_LIVO_CATKIN_WS:-$(cd "$REPO_ROOT/../.." && pwd)}"
BAG="${1:-$REPO_ROOT/../../bag/NTU/eee_01/eee_01.bag}"
RUN_ROOT="${PROB_LIVO_RUN_ROOT:-$REPO_ROOT/results/prob_livo/runs}"
RUN_ID="${PROB_LIVO_RUN_ID:-run_$(date +%Y%m%d_%H%M%S)}"
RATE="${PROB_LIVO_BAG_RATE:-1.0}"
CONFIG="$REPO_ROOT/config/NTU_VIRAL.yaml"
CONFIG_OVERLAY="${PROB_LIVO_CONFIG_OVERLAY:-}"
INPUT_SEMANTICS="${PROB_LIVO_INPUT_SEMANTICS:-fast_native}"
CAMERA_MODE="${PROB_LIVO_CAMERA_MODE:-off}"
VISUAL_GATE="${PROB_LIVO_VISUAL_PLANE_GATE:-livo2_prob_3sigma}"
CAMERA_CONFIG="${PROB_LIVO_CAMERA_CONFIG:-$REPO_ROOT/config/camera_NTU_VIRAL.yaml}"
ONE_CALLBACK_STEP="${PROB_LIVO_ONE_CALLBACK_STEP:-false}"
MEMORY_CSV="${PROB_LIVO_MEMORY_CSV:-}"
MEMORY_LABEL="${PROB_LIVO_MEMORY_LABEL:-$CAMERA_MODE}"
MEMORY_INTERVAL="${PROB_LIVO_MEMORY_INTERVAL:-2}"

case "$CAMERA_MODE" in
  off|h0|h1|h2) ;;
  *) echo "ERR: PROB_LIVO_CAMERA_MODE must be off, h0, h1, or h2" >&2; exit 2 ;;
esac
case "$VISUAL_GATE" in
  livo2_prob_3sigma|super_legacy) ;;
  *) echo "ERR: invalid PROB_LIVO_VISUAL_PLANE_GATE" >&2; exit 2 ;;
esac

if [[ ! -f "$BAG" || ! -f "$CONFIG" || \
      ( -n "$CONFIG_OVERLAY" && ! -f "$CONFIG_OVERLAY" ) || \
      ( "$CAMERA_MODE" != "off" && ! -f "$CAMERA_CONFIG" ) ]]; then
  echo "ERR: missing bag or config" >&2
  exit 2
fi
if [[ ! "$RUN_ID" =~ ^[A-Za-z0-9_.-]+$ ]]; then
  echo "ERR: invalid run id" >&2
  exit 2
fi
if [[ -n "$(git -C "$REPO_ROOT" status --short)" ]]; then
  echo "ERR: canonical runner requires a clean worktree" >&2
  exit 3
fi

RUN_DIR="$RUN_ROOT/$RUN_ID"
if [[ -e "$RUN_DIR" ]]; then
  echo "ERR: refusing to overwrite $RUN_DIR" >&2
  exit 2
fi
mkdir -p "$RUN_DIR/ros_log"
mkdir -p "$RUN_DIR/ros_home"
export ROS_HOME="$RUN_DIR/ros_home"
export ROS_LOG_DIR="$RUN_DIR/ros_log"
source /opt/ros/noetic/setup.bash
source "$CATKIN_WS/devel/setup.bash"

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
if [[ -n "$CONFIG_OVERLAY" ]]; then
  rosparam load "$CONFIG_OVERLAY"
fi
if [[ "$CAMERA_MODE" == "off" ]]; then
  rosparam set /common/img_en 0
else
  rosparam load "$CAMERA_CONFIG" /laserMapping
  rosparam set /common/img_en 1
fi
rosparam set /common/lidar_en 1
rosparam set /common/prob_livo_backend true
rosparam set /common/prob_livo_one_callback_step "$ONE_CALLBACK_STEP"
if [[ "$CAMERA_MODE" == "h1" || "$CAMERA_MODE" == "h2" ]]; then
  rosparam set /common/prob_livo_camera_vio true
else
  rosparam set /common/prob_livo_camera_vio false
fi
rosparam set /common/prob_livo_input_semantics "$INPUT_SEMANTICS"
rosparam set /prob_livo/visual_plane_gate "$VISUAL_GATE"
rosparam set /common/prob_livo_trajectory_path "$RUN_DIR/trajectory.tum"
rosparam set /imu/imu_en true
rosparam set /evo/pose_output_en false
rosparam dump "$RUN_DIR/effective_rosparams.yaml"
if [[ -n "$MEMORY_CSV" ]]; then
  mkdir -p "$(dirname "$MEMORY_CSV")"
fi

{
  echo "repository_root: $REPO_ROOT"
  echo "catkin_workspace: $CATKIN_WS"
  echo "git_branch: $(git -C "$REPO_ROOT" branch --show-current)"
  echo "git_head: $(git -C "$REPO_ROOT" rev-parse HEAD)"
  echo "git_dirty: no"
  echo "bag: $BAG"
  echo "bag_sha256: $(sha256sum "$BAG" | cut -d' ' -f1)"
  echo "config: $CONFIG"
  echo "config_sha256: $(sha256sum "$CONFIG" | cut -d' ' -f1)"
  echo "config_overlay: ${CONFIG_OVERLAY:-none}"
  if [[ -n "$CONFIG_OVERLAY" ]]; then
    echo "config_overlay_sha256: $(sha256sum "$CONFIG_OVERLAY" | cut -d' ' -f1)"
  fi
  echo "backend: ProbLioBackend P0-P4"
  echo "camera_mode: $CAMERA_MODE"
  echo "visual_state: $([[ "$CAMERA_MODE" == "h1" || "$CAMERA_MODE" == "h2" ]] && echo ON || echo OFF)"
  echo "visual_plane_gate: $VISUAL_GATE"
  echo "camera_config: $([[ "$CAMERA_MODE" == "off" ]] && echo none || echo "$CAMERA_CONFIG")"
  echo "input_semantics: $INPUT_SEMANTICS"
  echo "one_callback_step: $ONE_CALLBACK_STEP"
  echo "memory_csv: ${MEMORY_CSV:-none}"
  echo "memory_label: ${MEMORY_CSV:+$MEMORY_LABEL}"
  echo "memory_interval_seconds: ${MEMORY_CSV:+$MEMORY_INTERVAL}"
  echo "replayed_topics: /imu/imu,/os1_cloud_node1/points$([[ "$CAMERA_MODE" == "off" ]] || echo ,/left/image_raw)"
  echo "bag_rate: $RATE"
  echo "ros_master_uri: $ROS_MASTER_URI"
  echo "effective_rosparams: $RUN_DIR/effective_rosparams.yaml"
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
TOPICS=(/imu/imu /os1_cloud_node1/points)
if [[ "$CAMERA_MODE" != "off" ]]; then TOPICS+=(/left/image_raw); fi
rosbag play "$BAG" --clock --rate "$RATE" --topics \
  "${TOPICS[@]}" >"$RUN_DIR/play.log" 2>&1
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
COUNTERS_PATH="$RUN_DIR/trajectory.tum.counters.yaml"
if [[ -s "$COUNTERS_PATH" ]]; then
  COUNTER_RC=0
else
  COUNTER_RC=2
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

if [[ "$PLAY_RC" -eq 0 && ( "$NODE_RC" -eq 0 || "$NODE_RC" -eq 130 ) && \
      "$COUNTER_RC" -eq 0 && "$MEMORY_RC" -eq 0 && "$GT_RC" -eq 0 && \
      "$EVAL_RC" -eq 0 ]]; then
  RC=0
else
  RC=1
fi
{
  echo "play_rc: $PLAY_RC"
  echo "node_rc: $NODE_RC"
  echo "authority_counters: $COUNTERS_PATH"
  echo "visual_counters: $RUN_DIR/trajectory.tum.visual_counters.yaml"
  echo "counter_rc: $COUNTER_RC"
  echo "memory_rc: $MEMORY_RC"
  echo "ground_truth_rc: $GT_RC"
  echo "evaluation_rc: $EVAL_RC"
  echo "trajectory_rows: $(wc -l < "$RUN_DIR/trajectory.tum" 2>/dev/null || echo 0)"
  echo "trajectory_sha256: $(sha256sum "$RUN_DIR/trajectory.tum" 2>/dev/null | cut -d' ' -f1)"
  if [[ -n "${RUN_START_EPOCH:-}" && -n "${RUN_END_EPOCH:-}" ]]; then
    echo "runtime_seconds: $((RUN_END_EPOCH - RUN_START_EPOCH))"
  fi
  echo "end_utc: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "run_rc: $RC"
} >>"$RUN_DIR/meta.txt"
echo "run_dir: $RUN_DIR"
echo "__I3_RUN_DONE_RC=$RC"
exit "$RC"
