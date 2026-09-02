#!/usr/bin/env bash
# Canonical Prompt-3 camera-OFF Prob-LIO P0-P4 runner.
# One FAST scheduler/node, one Prob OctVox backend, and only the NTU IMU/LiDAR
# topics are replayed.  Every invocation writes a unique, self-describing run.

set -u

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
CATKIN_WS="${PROB_LIVO_CATKIN_WS:-$(cd "$REPO_ROOT/../.." && pwd)}"
BAG="${1:-$REPO_ROOT/../../bag/NTU/eee_01/eee_01.bag}"
RUN_ROOT="${PROB_LIVO_RUN_ROOT:-$REPO_ROOT/results/prob_livo/runs}"
RUN_ID="${PROB_LIVO_RUN_ID:-run_$(date +%Y%m%d_%H%M%S)}"
RATE="${PROB_LIVO_BAG_RATE:-1.0}"
CONFIG="$REPO_ROOT/config/NTU_VIRAL.yaml"

if [[ ! -f "$BAG" || ! -f "$CONFIG" ]]; then
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
export ROS_LOG_DIR="$RUN_DIR/ros_log"
source /opt/ros/noetic/setup.bash
source "$CATKIN_WS/devel/setup.bash"

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
for _ in $(seq 1 30); do
  rosnode list >/dev/null 2>&1 && break
  sleep 1
done
if ! rosnode list >/dev/null 2>&1; then
  echo "ERR: roscore did not start" >&2
  exit 2
fi

rosparam load "$CONFIG"
rosparam set /common/img_en 0
rosparam set /common/lidar_en 1
rosparam set /common/prob_livo_backend true
rosparam set /common/prob_livo_trajectory_path "$RUN_DIR/trajectory.tum"
rosparam set /imu/imu_en true
rosparam set /evo/pose_output_en false
rosparam dump "$RUN_DIR/effective_rosparams.yaml"

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
  echo "backend: ProbLioBackend P0-P4"
  echo "camera: OFF"
  echo "replayed_topics: /imu/imu,/os1_cloud_node1/points"
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

rosbag play "$BAG" --clock --rate "$RATE" --topics \
  /imu/imu /os1_cloud_node1/points >"$RUN_DIR/play.log" 2>&1
PLAY_RC=$?
sleep 3
kill -INT "$NODE_PID" 2>/dev/null || true
wait "$NODE_PID" 2>/dev/null
NODE_RC=$?
NODE_PID=""

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
      "$GT_RC" -eq 0 && "$EVAL_RC" -eq 0 ]]; then
  RC=0
else
  RC=1
fi
{
  echo "play_rc: $PLAY_RC"
  echo "node_rc: $NODE_RC"
  echo "ground_truth_rc: $GT_RC"
  echo "evaluation_rc: $EVAL_RC"
  echo "trajectory_rows: $(wc -l < "$RUN_DIR/trajectory.tum" 2>/dev/null || echo 0)"
  echo "end_utc: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "run_rc: $RC"
} >>"$RUN_DIR/meta.txt"
echo "run_dir: $RUN_DIR"
echo "__I3_RUN_DONE_RC=$RC"
exit "$RC"
