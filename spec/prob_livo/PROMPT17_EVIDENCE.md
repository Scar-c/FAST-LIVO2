# Prompt17 Native Late-Point Deskew Corrective Evidence

## Final status

`PROMPT17 CLOSED — NATIVE LATE-POINT SEMANTICS RESTORED`

Prompt17 is complete. FAST-LIVO2 native late points are now deskewed by a
temporary terminal-motion extrapolation in the shared `ProbImuAdapter` path
for both P-LIO and P-LIVO. The scheduler endpoint remains authoritative for
the shared filter, covariance, lifecycle, and next epoch anchor. Oxford P-LIO
Church, Palace, and Quarter complete without backend rejection; Oxford P-LIVO
Church, College, Palace, and Quarter also complete without backend rejection.
The required online/offline parity check on a true late-point Oxford sequence
is exact. No P-LIVO tuning, visual stride change, or unrelated estimator/map
change was made.

## Scope and pre-edit gate

The scope was the FAST-LIVO2-native `fast_native` input contract in the
project-owned Prob-LIO P0-P4 backend. The native oracle audit established that
the terminal state is used to extrapolate a point pose temporarily and that
the filter is not advanced to the point timestamp.

Before any code edit, the requested online H0 gate was run on NTU `eee_01`:

| Check | Result |
|---|---|
| Run | `results/prob_livo/runs/prompt17_pre_online_h0_eee01` |
| Backend attempted / success / rejected | `3984 / 3983 / 1` |
| LiDAR callbacks | `3987` |
| Trajectory rows / SHA256 | `3979` / `12d1ee915834f8ac672f9170f7dff504a4850c4cf0a657b4bbbbf5ca9f952a90` |
| ATE RMSE | `0.049152820043009145 m` |
| Runtime | `400 s` |

This was recorded as the pre-fix online H0 observation. No source change was
made before it completed.

## Corrective semantics

The production implementation is in
`src/prob_livo/prob_imu_adapter.cpp`:

1. `T_e = timing.epoch_end` remains the scheduler-owned endpoint.
2. For a point with `t_i > T_e`, copy the terminal propagation snapshot and
   compute, temporarily,
   `R_i = R_e ExpSO3(omega_e (t_i-T_e))`,
   `p_i = p_e + v_e (t_i-T_e) + 0.5 a_e (t_i-T_e)^2`.
3. Transform the point through the configured nonidentity LiDAR-to-IMU
   extrinsic and back into the `T_e` frame.
4. Never write the temporary snapshot to the filter, covariance, lifecycle,
   or next epoch anchor.

The shared implementation is used by both `SchedulerMode::kOnlyLio` and
`SchedulerMode::kLivo`. Normal points, endpoint-equal points, intensity, and
curvature provenance retain their prior behavior. The legacy Super timing
branch still retains its separately documented raw-frame fallback; no
Super-only fallback is used by native FAST-LIVO2 semantics.

The corrected code is anchored at:

```text
src/prob_livo/prob_imu_adapter.cpp:27-39   terminal extrapolation
src/prob_livo/prob_imu_adapter.cpp:265-305 IMU endpoint authority
src/prob_livo/prob_imu_adapter.cpp:423-483 native point-time selection
src/prob_livo/prob_imu_adapter.cpp:485-495 transform into T_e frame
```

## Red-to-green regression and mutation gates

The independent fixture in `tests/prob_livo/test_i2_late_point.cpp` uses
nonzero angular velocity, velocity, and acceleration; a nonidentity rotation
and translation extrinsic; points before `T_e`, equal to `T_e`, at `T_e+0.2
us`, at `T_e+1.4 us`, and a final source-order point that is not the maximum
time. The pre-fix implementation failed the red test with eight failures,
including complete-epoch rejection, scan-cardinality change, shared-state
delta `0.1767098346`, and timestamp delta `0.1`.

After the corrective, G-P17.1/G-P17.2 passes all 27 checks:

| Gate | Evidence |
|---|---|
| M1 whole-scan rejection | late packet succeeds and preserves all points |
| M2 tolerance expansion | exact `time_tolerance = 2e-8 s` retained |
| M3 endpoint clamp | expected terminal-motion errors `8.50e-8` and `2.04e-7`; endpoint-clamp discrimination `1.08e-6` and `1.03e-5` |
| M4 Super-only extrinsic fallback | both late outputs differ from the raw extrinsic fallback |
| M5 shared-state/covariance/lifecycle mutation | state/covariance delta `0`; filter timestamp delta `0`; lifecycle anchor unchanged |

The same fixture reports lateness `0.198364 us` and `1.403808 us` and passes
both native LIO and native LIVO scheduler paths. No scheduler endpoint,
covariance, or lifecycle writeback was added.

## Oxford 32-worker P-LIO regression

All diagnostic runs used the official Prompt16 overlay over
`config/OXFORD_SPIRES.yaml`, `camera_mode=off`, `fast_native`, TBB/OMP worker
cap `32`, affinity `0-31`, and the project-owned in-process offline reader.
The corrected configuration hashes are base
`87bd926073ed49c68ab330617e7ca52c0e0b0470ab2c63cdba89aac2312aca92` and
overlay `b5917b705e856bdfa1f949201b57daada9be3645ceb677c0d03f40481405ce61`.

| Sequence | Backend attempted / success / rejected | Rows | ATE RMSE (m) | Late epochs / points | Median / max lateness (us) | SHA256 prefix |
|---|---:|---:|---:|---:|---:|---|
| Church_05 | `8007 / 8007 / 0` | 8001 | 0.2440 | `0 / 0` | `nan / nan` | `a85400ba437e` |
| Palace_01 | `4052 / 4052 / 0` | 4047 | 0.1272 | `2630 / 4810` | `1.78814 / 39.8159` | `0a6016f413dc` |
| Quarter_01 | `2894 / 2894 / 0` | 2889 | 0.0510 | `1719 / 2541` | `1.43051 / 44.1074` | `38f9f0c7b6c8` |

Palace and Quarter are true late-point cases. Their complete trajectories and
zero rejection counters directly close the recurrent Prompt16 producer-level
failure (`scheduler point time is outside the LIO epoch`).

## Oxford 32-worker P-LIVO regression

These runs used the same official configuration, `camera_mode=h1`,
`camera_OXFORD_SPIRES.yaml`, `fast_native`, TBB/OMP cap `32`, and the native
visual gate. Every run returned `run_rc=0` and backend rejection `0`.

| Sequence | Backend attempted / success / rejected | Rows | Visual process / commit / rollback | ATE RMSE (m) | Late epochs / points | Median / max lateness (us) |
|---|---:|---:|---:|---:|---:|---:|
| Church_05 | `3960 / 3960 / 0` | 3956 | `3956 / 3954 / 6362` | 0.2278 | `0 / 0` | `nan / nan` |
| College_03 | `5680 / 5680 / 0` | 5673 | `5673 / 2874 / 4751` | 0.1318 | `2820 / 37761895` | `12939 / 50721.4` |
| Palace_01 | `8038 / 8038 / 0` | 8031 | `8031 / 4054 / 6648` | 0.2280 | `3983 / 56539219` | `14911.2 / 50242.2` |
| Quarter_01 | `5744 / 5744 / 0` | 5737 | `5737 / 2893 / 4850` | 0.0680 | `2846 / 42205770` | `14678.7 / 50237.7` |

The large late-point counts in College, Palace, and Quarter are measured
source-order point times, not a tolerance change. The native terminal
extrapolation remains temporary and does not advance the shared filter.

## Online/offline parity closure

Palace_01 was selected because the offline trace showed real late points. The
online runner was extended to accept `PROB_LIVO_DATASET_FAMILY=OXFORD` and
`PROB_LIVO_GT_PATH` while preserving its NTU default behavior. The online and
offline runs used identical official configuration, `fast_native`, camera off,
affinity `0-31`, and worker cap `32`.

| Artifact | Online | Offline |
|---|---|---|
| Run directory | `results/prob_livo/runs/prompt17_online_oxford_p_lio_Palace_01_trace` | `results/prob_livo/runs/prompt17_post_oxford_p_lio_Palace_01_trace2` |
| Trajectory rows | 4047 | 4047 |
| Trajectory SHA256 | `0a6016f413dc715c97ef668c5aedb1de41aa8431bf2537c2c0745762461875c3` | identical |
| Trace rows / SHA256 | `4053` / `95181ef2b978c01da461f06e132f99f978cbc08f42771709c2a62ec7ef71de0f` | identical |
| Backend counters | identical | identical |
| Late epochs / points | `2630 / 4810` | identical |
| Median / max lateness | `1.78814 / 39.8159 us` | identical |
| ATE RMSE | `0.1272 m` | `0.1272 m` |

The first online run without trace had already produced the same trajectory
SHA; the trace-enabled rerun additionally proves exact online/offline
late-point telemetry. Online runtime was `405 s`; offline runtime was `77 s`.

## Required 4-core canonical verification

The final resource-constrained verification used affinity `0,2,4,6` and
`PROB_LIVO_WORKERS=4`; no `-j32` or `-j16` build was used. All five required
checks returned `RC0`, with complete output and backend rejection `0`:

| Run | Rows | Backend attempted / success / rejected | Camera/visual evidence | ATE RMSE (m) |
|---|---:|---:|---|---:|
| P-LIO Church_05 | 8001 | `8007 / 8007 / 0` | camera off | 0.2440 |
| P-LIO Palace_01 | 4047 | `4052 / 4052 / 0` | camera off; 2630 late epochs | 0.1272 |
| P-LIO Quarter_01 | 2889 | `2894 / 2894 / 0` | camera off; 1719 late epochs | 0.0510 |
| P-LIVO Church_05 | 3956 | `3960 / 3960 / 0` | 3961 images, 3956 process, 3953 commits | 0.2285 |
| P-LIVO College_03 | 5673 | `5680 / 5680 / 0` | 5681 images, 5673 process, 2872 commits | 0.1115 |

P-LIVO visual counters vary in accepted commit count between independent
4-core and 32-worker executions, consistent with the accepted native visual
runtime nondeterminism contract; no backend rejection, trajectory truncation,
or late-point failure occurred.

## Build and focused tests

Build command, restricted to the requested safe parallelism:

```bash
source /opt/ros/noetic/setup.bash
source devel/setup.bash
catkin_make -C /home/lc/super_livo --pkg fast_livo \
  -DCMAKE_BUILD_TYPE=Release -j4
```

Result: `RC0`, including `prob_livo_offline`, `fastlivo_mapping`, and all
focused test targets.

Final focused suite result:

```text
G-I1.1–G-I1.SO3: PASS
G-I2.1–G-I2.9: PASS, including G-P17.1/G-P17.2 (27 checks)
G-I3: PASS (79 checks)
G-I4: PASS (57 checks)
G-I5: PASS (115 checks)
G-I6: PASS (19 checks)
G-P4: PASS (203 checks)
Total reported focused checks: 835
```

Static closure checks passed: the native tolerance remains `2e-8 s`, no
native point-time endpoint clamp was introduced, no scheduler/filter/lifecycle
writeback to a late point exists, and no `[DEBUG-*]` tags remain.

## Reproducibility and commit record

Prompt17 source commits:

```text
64e5023 fix(prob-livo): extrapolate native late points at terminal motion
6b4eb9c test(prob-livo): record late-point runtime evidence
1a39d1a fix(prob-livo): align late-point trace schema
c4d07eb feat(prob-livo): evaluate Oxford online runs
```

The final evidence-close commit and push are recorded after this report is
committed. The Native oracle repository was not modified. The final worktree
is required to be clean before pushing `prob-livo` to `origin`.
