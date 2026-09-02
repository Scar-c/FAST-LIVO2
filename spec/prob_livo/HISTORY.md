# Prob-LIVO History

## Prompt 0 / I0 — bootstrap and architecture freeze

FAST-LIVO2 is the host because its public scheduler, scan recombination, camera
epoch handling, and visual estimator are the semantics to preserve. The
Prob-LIO repository remains a clean, read-only implementation oracle so the
canonical Super-native P0–P4 backend can be migrated deliberately. P5 is
excluded because it is experimental/non-canonical in the oracle and is not
part of the frozen integration target.

The camera-OFF Prob-LIO backend baseline precedes camera-ON closure so LIO
replacement and visual preservation can be verified separately. A second
LiDAR map is forbidden: Prob OctVox is the sole LiDAR geometry authority and
FAST-LIVO2 `feat_map` owns only visual patches/VisualPoints. The shared-state
contract is one host-layout x19/P19, not two filters with copied pose.

Prompt 0 created the project-owned documentation namespaces, recorded exact
host/reference identities, audited production source seams, inventoried the
local NTU/OXFORD files, and built the untouched host with the existing VIKIT
dependency overlay. No legacy runtime/evaluator asset was imported. Future
imports follow COPY-ON-DEMAND with source commit/path provenance.

## State-operation authority decision

FAST `StatesGroup` operators are retained for FAST visual semantics and ABI.
`ProbESKF19` owns canonical Super semantics for the future LIO path. FAST's
additive gravity operator differs from Super's gravity normalization, so direct
use of `StatesGroup::operator+=` in LIO is forbidden. I6 will later observe
visual-induced gravity correction rather than changing VIO in this stage.

## Prompt 1 / I1 — shared ProbESKF19 core

I1 adds a non-wired `ProbESKF19` library over the caller-owned FAST
`StatesGroup`, using a centralized non-contiguous host/Super mapping. Predict
and iterated LiDAR update follow the canonical Super ESKF source; inverse
exposure is frozen by default and has an explicit host-compatible random walk
option. A separately implemented test-only Super oracle covers dense covariance
and adversarial negative fixtures. The host runtime remains untouched, and the
next stage remains I2 scheduler-owned IMU/undistortion work.

## Prompt 2 / I1 corrective and I2 — Super-native IMU seam

The I1 close exposed one semantic defect: the production observation callback
did not carry Super's `need_converge` phase into the measurement producer.
`ProbESKF19::UpdateObserve` now passes that bool directly before every
callback. The corrective test records `[F,F]` and `[F,F,F,T]` against an
independent Super oracle and keeps the prior I1 state/covariance parity
fixtures unchanged.

The FAST scheduler remains H0 authority. ONLY_LIO ends at the scan end and
LIVO ends at the image capture time; LIVO point curvature is rebased by the
production cut helper, and `last_lio_update_time` is advanced only after a
successful adapter epoch. The camera cut and current/next partition therefore
remain FAST semantics.

H2 now has a production-ready, non-wired `ProbImuAdapter`. It replaces FAST's
IMU initialization, propagation, and backward-undistortion authority for the
future Prob-LIO path with Super's mean initialization, gravity/yaw/robot
alignment, midpoint ESKF, endpoint clipping, propagation trace, quaternion
slerp, and translational interpolation. Because FAST consumes IMUs only up to
the endpoint, the adapter makes the required Super look-ahead explicit and
rejects incomplete final coverage instead of silently stopping early.

The adapter's output is deliberately named `prob_scan_undistort_imu` and is
expressed in the scan-end IMU/body frame. It is not connected to FAST's
LiDAR-frame `feats_undistort` or `VoxelMapManager`; the default runtime stays
on FAST `Process2` until I3 supplies the matching Prob-LIO map backend. No
OctVox/P1–P4, VIO change, camera runtime, bag run, or I3 work belongs to this
stage.

## Prompt 3 / I3 — camera-OFF Prob-LIO P0–P4 baseline

Prompt 3 first closed the I2 lifecycle seam in commit `27854c0`: one explicit
`ProbLioLifecycle` authority now owns `IMU_INIT → MAP_INIT → RUN`, the
scheduler epoch anchor is separate from the filter clock, and the anchor is
advanced once only after a successful epoch. The production scheduler helper
and IMU-buffer look-ahead behavior are covered by the I2 lifecycle and buffer
tests.

The camera-OFF backend was then added in `30e5e3e`. `ProbLioBackend` owns the
shared `ProbESKF19`, Super IMU adapter, Prob OctVox map, scan buffers, and
trajectory. It uses raw LiDAR for four map-init scans, then runs Super
downsample, HKNN, QR/P3, P1/P2 covariance, Super legacy association, P4
weighting, ProbESKF19 LiDAR update, and covariance-aware map update. FAST
remains the ROS/scheduler shell. Camera loading/subscription, VIO processing,
and the FAST LiDAR voxel map are disabled in this baseline; P5 remains
excluded.

The canonical runner is
`tools/prob_livo/run_eee01_camera_off.sh`. It requires a clean worktree,
starts one local ROS master and one FAST node, replays only EEE01 IMU/LiDAR,
and stores an ignored, self-describing run directory. The path-fix and isolated
ROS-home corrections are `e1c63cb` and `ce805bb`. The complete run
`results/prob_livo/runs/eee01_camera_off_p0_p4_correction/` returned zero for
bag playback, node shutdown, counters, GT extraction, evaluation, and the
overall run. It recorded 3602 successful epochs: 3 IMU_INIT epochs, 4
MAP_INIT epochs, and 3595 RUN epochs. Runtime authority counters recorded
17017 raw map-init inserts, 12485822 map-update inserts, 14702670 undistorted
points, 12485822 downsampled points, 36666902 HKNN queries, 181435218 HKNN
returns, 30034406 valid QR attempts, and 37765549 weighted measurements.

Using the NTU VIRAL dataset-author evaluator, the baseline achieved
translation ATE RMSE `0.05290159739482509 m` over 3016 matched estimates. The
same evaluator was applied to the legacy P4 reference artifact
`p11_smoke_eee_p4_lc` (`0.08883155405698266 m`, 3329 matched). The required
primary comparison uses raw world frames without mutual SE(3) alignment: 3594
matched rows, timestamp mean/max absolute delta
`0.02349526841042104 / 0.06304597854614258 s`, translation RMSE/median/max
`0.1272755745726123 / 0.10450333931631896 / 0.5874476253736715 m`, rotation
RMSE/median/max `0.01831955642234057 / 0.0092050820666131 /
0.08735832181275227 rad`, classified exactly once as
`I3_TRAJECTORY_CLOSE_NONIDENTICAL`.

I3 is `CLOSED/PASS — Owner audit pending`. The next stage is I4, the
`pointWithVar`-compatible current-scan adapter; camera-ON visual closure is
still reserved for I6.
