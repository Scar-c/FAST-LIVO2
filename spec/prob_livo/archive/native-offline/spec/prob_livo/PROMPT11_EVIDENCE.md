# Prompt 11 Evidence — FAST-Native Prob-LIO and Unified Native FAST-LIVO2 Offline Runner

**Final status: `PROMPT11 PHASE A CLOSED / NATIVE OFFLINE OWNER DECISION REQUIRED`**

Prompt11 stops here. It does not enter I8, H1/H2, P5, or any multi-dataset
sweep. The native unified offline infrastructure is implemented and its LIO
path is parity-closed. Native LIVO is not declared offline-authoritative:
the unmodified native authority is itself non-deterministic across repeated
normal-online runs, and the post-refactor online/offline LIVO trajectories
therefore cannot be required to be byte-identical without an owner decision
about native numerical/order semantics.

## 1. Entry state and authority

| Item | Evidence |
|---|---|
| Primary repository | `/home/lc/super_livo/src/FAST-LIVO2` |
| Primary branch at entry | `prob-livo` |
| Primary HEAD / `origin/prob-livo` | `9c3eb35ff0da92e774173d357dd82dd3800611ce` / same |
| Primary worktree at entry | clean |
| Native authority | `0d2c0346107b75b59934975adec9a6eeeb913c64` |
| Native worktree | `/tmp/prompt11_fast_livo2_native` |
| Native branch | `prompt11-native-offline` |
| Native starting parent | `0d2c0346107b75b59934975adec9a6eeeb913c64` |
| Native final source HEAD before evidence commit | `92f26dc3c07b750bb07dbc4b53616198a0d4cc1d` |
| Native build/devel | `/tmp/prompt11_native_ws` |
| Prompt source SHA256 | `00ece44769050b01f8ebd629aaffb1c8013e96ba6378ff9ebbb5c0203268c432` |

The main `prob-livo` branch was not used as a native source overlay. The
isolated native branch is based exactly on the native authority, so its source
diff is limited to event-source extraction, runner plumbing, counters/timing,
tests, and the removal of one attempted non-native trajectory-output change.

The native implementation commits are:

```text
50e0d466  feat(native): add unified online offline event source runner
a0d84a57  fix(native): separate scheduler polls from successful steps
92f26dc3  fix(native): preserve native LIVO trajectory output semantics
```

## 2. Phase A — P0-FN FAST-native Prob-LIO characterization

The authoritative FAST-native contract was taken from the existing
`config/NTU_VIRAL.yaml` and canonical runner path. No Super legacy input
overlay was used.

| Effective item | P0-FN value |
|---|---|
| Input semantics | `fast_native` |
| Backend | current Prob-LIO P0–P4 |
| Camera / visual | OFF / OFF |
| LiDAR topic | `/os1_cloud_node1/points` |
| IMU topic | `/imu/imu` |
| Image topic | `/left/image_raw`, not consumed in P0-FN |
| LiDAR time offset | `-0.1 s` |
| IMU time offset | `0.0 s` |
| LiDAR type/order | Ouster (`lidar_type=3`), native relative point-time/order |
| Point filter / blind | `3` / `1.0` |
| Max range | no Super legacy max-range behavior |
| IMU initialization | `imu_int_frame=30` |
| LIO iterations / voxel size | `5` / `0.5` |
| Camera config | none; camera and visual paths disabled |
| P5 | OFF |

Effective parameters are recorded in
`results/prob_livo/runs/prompt11_p0_fn_offline_escalated/effective_rosparams.yaml`.

### P0-FN result and parity

Both runs used the same complete `eee_01` bag, and the online run used normal
ROS `spinOnce()`, `one_callback_step=false`, camera OFF, and 1.0× `rosbag
play`. The offline run used the existing canonical Prob-LIO in-process
runner with its TBB-32 configuration.

| Source | Rows | Matched GT | ATE RMSE (m) | Trajectory SHA256 |
|---|---:|---:|---:|---|
| Offline | 3980 | 3327 | `0.05450275014453893` | `7149297f46df10ce895fe564dc689b05b3356e6b7c58c03ff92bffd761b93410` |
| Normal online | 3980 | 3327 | `0.05450275014453893` | same |

The strict online/offline comparison paired all 3980 rows with zero
unmatched rows, zero timestamp error, zero translation error, and maximum
rotation error `4.2146848510894035e-08 rad`. Input and estimator counters
were byte-identical. This is the required P0-FN event-source parity.

### P0-FN memory

The formal memory boundary is normal-online estimator PID only. The external
`tools/prob_livo/memory_monitor.py` sampled the mapper every second; camera,
RViz, PCD save, and heavy diagnostics were off.

Values are MiB in `RSS / PSS / USS` order. The early baseline is the median
of the 20–40 s window; peak is over the complete monitor trace.

| Run | Samples | Duration (s) | Early RSS/PSS/USS | Peak/final RSS/PSS/USS | USS growth |
|---|---:|---:|---|---|---:|
| `prompt11_p0_fn_online_mem1` | 401 | 402.641 | 132.648 / 110.034 / 107.430 | 303.254 / 281.973 / 280.648 | 173.219 |
| `prompt11_p0_fn_online_mem2` | 401 | 402.313 | 133.051 / 110.428 / 107.854 | 301.531 / 280.379 / 279.074 | 171.221 |
| Median | 2 | — | 132.850 / 110.231 / 107.642 | 302.393 / 281.176 / 279.861 | 172.220 |

Median memory curve (`RSS / PSS / USS`, MiB):

| Elapsed (s) | RSS / PSS / USS |
|---:|---|
| 10 | 135.03 / 112.42 / 109.83 |
| 25 | 203.79 / 181.19 / 178.60 |
| 50 | 254.31 / 231.71 / 229.12 |
| 75 | 280.48 / 257.85 / 255.27 |
| 90 | 294.66 / 271.96 / 269.37 |
| End / peak | 302.39 / 281.18 / 279.86 |

Prompt10's N0 native aggregate is allowed for reuse because the native
authority, config/build, camera-OFF policy, and external monitor boundary
are the same, and its final memory/trajectory was repeatable. The formal
FAST-native characterization is therefore:

| Variant | Backend | Camera | ATE RMSE (m) | Peak/final USS (MiB) |
|---|---|---|---:|---:|
| N0-FN | native FAST-LIVO2 | OFF | `0.030768325230497` | `2655.22` |
| P0-FN | Prob-LIO P0–P4 | OFF | `0.05450275014453893` | `279.861` |

N0-FN memory is approximately `2.66 GiB` final USS and P0-FN approximately
`280 MiB`; these are characterization results under different estimator/map
implementations, not a tuning recommendation. N0's Prompt10 early memory
window was explicitly marked environment-noisy; its late final/peak value is
the reusable stable value.

## 3. Phase B — unified native infrastructure

### Production architecture

The original native production chain was audited as:

```text
ROS callbacks
  -> native IMU/LiDAR/Image queues
  -> sync_packages(LidarMeasures)
  -> handleFirstFrame()
  -> processImu()
  -> stateEstimationAndMapping()
       -> handleLIO() / handleVIO()
       -> native map/state/VIO ownership
  -> native trajectory output
```

`ProcessAvailableNativeEpochs()` is now the single scheduler seam. Native
online execution is `spinOnce() -> ProcessAvailableNativeEpochs()`. Native
offline execution is `rosbag record -> same callback -> same
ProcessAvailableNativeEpochs()`, followed by bounded EOF drain. The offline
reader has no estimator, state, map, VIO, plane, or trajectory-copy path.

The implementation adds:

- `include/native_offline_reader.h` and `src/native_offline_reader.cpp`: a
  transport-only rosbag reader using bag record order and target-topic type
  dispatch;
- `tools/fastlivo_native_offline.cpp`: the LIO/LIVO runner that constructs one
  native `LIVMapper` and invokes its original callbacks;
- `tools/run_native_fast_livo2_online.sh` and
  `tools/run_native_fast_livo2_offline.sh`: reproducible source runners;
- shared native input, scheduler, LIO/map, visual, trajectory, and monotonic
  timing reports;
- `tests/native_offline_reader_test.cpp`: synthetic adversarial event-source
  coverage.

The attempted addition of VIO-epoch trajectory rows was removed after audit:
the native authority writes trajectory in `handleLIO()` only. Thus the final
refactor preserves the native LIVO trajectory-output contract.

### Common counters and timing

`NativeRuntimeCounters` records input callbacks/enqueues, successful native
steps, synchronized packages, LIO/camera epochs, IMU processing, LIO/map
calls, visual calls/commits, and trajectory rows. `scheduler_poll_calls` is
also recorded but is source-specific: normal ROS online polling checks empty
queues many more times than the offline record reader. `scheduler_step_calls`
means successful synchronized native epochs and is therefore comparable.

`NativeRuntimeTiming` uses a monotonic clock for input preprocessing, IMU,
LiDAR association/update, map query/update, visual processing, and total
estimator compute. Offline bag I/O is recorded separately in
`offline_source.yaml` and is not mixed into estimator compute.

### Ownership gate

Static audit of the final native diff found:

```text
offlineNativeLioAlgorithm       no
offlineNativeVioAlgorithm       no
second ESKF/filter/state        no
second native map                no
second VIO manager/visual map    no
offline pose-copy bridge        no
offline plane/raycast authority no
```

The native runtime still owns one `LIVMapper`, one `voxel_map`, one
`VIOManager`, and one `visual_submap`. The new reader only supplies messages
to the existing callback and queue ownership.

## 4. Native LIO parity

The same `eee_01` bag/config and native branch were used for both sources.
Camera and visual paths were disabled.

| Source | Rows | Trajectory SHA256 | Wall (s) | Bag I/O (s) | Estimator compute (s) |
|---|---:|---|---:|---:|---:|
| Offline `prompt11_native_lio_offline_r2` | 3985 | `8906ba652f3862f9f6d11d7d9bdb785bdfd91557cd9ba010e8aee336b470033a` | 68 | 67.2031 | 63.4348 |
| Normal online `prompt11_native_lio_online_post` | 3985 | same | 400 | — | 69.6918 |

The trajectory files are byte-identical. The following counters are exact on
both sources:

```text
imu_callbacks_received       153347
lidar_callbacks_received      3987
image_callbacks_received         0
imu_messages_enqueued       153323
lidar_messages_enqueued       3987
ignored_input_messages          24
scheduler_step_calls          3987
scheduler_sync_packages       3987
lidar_epochs                   3987
camera_epochs                    0
imu_processing_calls          3987
lidar_update_calls            3985
map_query_calls               3985
map_update_calls              3985
visual_process_calls             0
visual_state_commits             0
trajectory_rows               3985
```

All timing counts also match: input preprocess 3987, IMU 3987, LiDAR
association/update 3987, map query/update 3985, visual 0, estimator compute
3987. Offline wall speedup is approximately `400/68 = 5.88x`; this is a
source/runtime characterization, not an algorithmic claim, because offline
includes bag decompression while online is real-time playback.

The native process emits shutdown signal 139 after complete output under this
authority. The runner records `node_rc=139`, `node_rc_accepted=1` only when
trajectory, all runtime sidecars, and `offline_source.yaml` exist; it returns
success in that complete-output case. This matches the accepted native
baseline behavior recorded in Prompt10 and does not accept an in-flight
estimator failure.

## 5. Native LIVO parity and owner decision

The native LIVO offline run has real camera activation:

```text
image records                  3986
image callbacks                3986
images enqueued                3985
camera epochs                  3983
visual process calls           3983
visual state commits           3982
visual points created       356233
reference update attempts  336530
reference updates accepted 542594
plane queries               112534
final visual map points        131
```

The final post-correction offline run is
`results/prob_livo/runs/prompt11_native_livo_offline_post` and has 3983
trajectory rows, SHA
`391f2674df11a8c2b0b3d91a6efdab1ce22c9a015af37dfdf2833283790c74e7`, and
estimator compute `101.173 s` with bag I/O `105.935 s`.

The matching normal-online post-refactor run has the same row count and exact
input/scheduler/LIO/visual activation counts, but its trajectory SHA is
`78f36a5e6b4e7310ed8bac7ccdbccacff464b98e3da62b411ebd7dd947e29d69`.
Strict pairing found all 3983 timestamps with zero timestamp error, but
translation max/RMSE `0.013382747438399874 / 0.001710602650550174 m` and
rotation max/RMSE `0.0011152326126054586 / 0.00008600226132481608 rad`.
Visual counters differ as well:

```text
                         offline       online
visual_points_created    356233        356594
reference attempts        336530        337197
reference accepted        542594        547914
plane queries             112534        112850
final visual map points      131           128
```

Before the refactor, two normal-online runs using the untouched authority
`0d2c034...`, the same config, bag, topics, and 1.0× playback were also
captured:

| Run | Rows | SHA256 |
|---|---:|---|
| `prompt11_native_livo_pre1` | 3983 | `e3e31bd6277da6cdb08b02d7aae63fbaee048ef964330e351d4ff748208b07be` |
| `prompt11_native_livo_pre2` | 3983 | `23e14c7c2fd68d461577c49cc8dda30d087f8c1ab5d619fd378b48cee3843afd` |

The untouched pre1/pre2 strict comparison paired all 3983 rows and had zero
timestamp error, but translation max/RMSE
`0.03709071389175457 / 0.012188208030581705 m` and rotation max/RMSE
`0.003687082319909385 / 0.00034445664089045094 rad`. Therefore native LIVO
is already non-deterministic before this infrastructure refactor. No
residual, pyramid, map, covariance, plane, raycast, or numerical patch was
added to force false parity.

The post-refactor LIVO offline wall speedup is approximately `400/107 = 3.74x`
relative to normal online wall time, with offline bag I/O and estimator
compute recorded separately. The speed figure is informative only while the
native LIVO owner decision remains open.

## 6. Adversarial scheduler tests

`native_offline_reader_tests` passed after the final build. Its fixture
exercises the required source-order cases:

1. IMU burst before LiDAR;
2. image between two LiDAR scans;
3. multiple images in one LiDAR interval;
4. LiDAR before sufficient IMU;
5. image near EOF;
6. equal and near-equal timestamps;
7. irrelevant topic excluded by the reader's target-topic query;
8. camera-disabled mode excludes image records.

The test passed with exit code 0 and reported both camera-enabled and
camera-disabled accounting paths. No dataset-specific estimator branch is
present in the reader or runner.

## 7. Regression and contamination audit

| Gate | Result |
|---|---|
| Native full catkin build | PASS (`catkin_make ... -DCMAKE_BUILD_TYPE=Release -j4`) |
| Native reader adversarial test | PASS |
| Native LIO online/offline trajectory | PASS, byte-identical |
| Native LIO common counters/timing counts | PASS, exact |
| Native LIVO image/camera/visual activation | PASS, nonzero |
| Native LIVO pre-refactor repeatability | FAIL by native authority behavior; owner decision required |
| P0-FN Prob-LIO online/offline | PASS, exact SHA/counters |
| Primary Prompt0–Prompt10 regression set | inherited PASS from primary HEAD `9c3eb35`; primary branch remained untouched |
| Heavy diagnostics/RViz/PCD | OFF for formal runs |
| Offline bag I/O vs estimator timing | separated |
| TBB/OMP semantic contamination | P0-FN uses canonical Prob-LIO TBB32; native authority retains its original OpenMP/build semantics |

The worktree was clean before the final evidence commit. No `[DEBUG-...]`
instrumentation remains, and no temporary debug source was added to the
repository.

## 8. Evidence locations

```text
results/prob_livo/runs/prompt11_p0_fn_offline_escalated/
results/prob_livo/runs/prompt11_p0_fn_online_mem1/
results/prob_livo/runs/prompt11_p0_fn_online_mem2/
results/prob_livo/runs/prompt11_native_lio_offline_r2/
results/prob_livo/runs/prompt11_native_lio_online_post/
results/prob_livo/runs/prompt11_native_livo_offline_post/
results/prob_livo/runs/prompt11_native_livo_online_post/
results/prob_livo/runs/prompt11_native_livo_pre1/
results/prob_livo/runs/prompt11_native_livo_pre2/
```

The pushed native branch and this evidence report are the deliverable for
Prompt11. The branch must not be treated as a deterministic native LIVO
offline owner until the LIVO numerical/order decision is made.

**`PROMPT11 PHASE A CLOSED / NATIVE OFFLINE OWNER DECISION REQUIRED`**

STOP. Do not enter I8, H1/H2, P5, or multi-dataset benchmarking from this
Prompt11 run.
