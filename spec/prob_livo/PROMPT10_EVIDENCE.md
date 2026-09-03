# Prompt 10 Evidence — Dead-Code Hygiene and eee_01 Native-LIO/Prob-LIO Characterization

**Final status: `PROMPT10 CHARACTERIZATION CLOSED`**

实验边界已关闭在 N0/H0/H0-LIOONLY；本轮没有进入 LIVO/H1/H2 算法对比或 I8。

## 1. State consensus

Prompt10 entry state was verified as follows:

| Item | Evidence |
|---|---|
| Branch | `prob-livo` |
| Expected frontier | `9802d8f2222b9c4e95c5e0a4129ee70d056e7457` |
| Entry HEAD / `origin/prob-livo` | both `9802d8f2222b9c4e95c5e0a4129ee70d056e7457` |
| Worktree | clean |
| `git merge-base --is-ancestor 9802d8f HEAD` | rc `0` |
| Host baseline | `0d2c0346107b75b59934975adec9a6eeeb913c64`, locally resolvable and recorded by `SPEC.md`/`EVIDENCE_INDEX.md` |

The final frontier is a descendant containing only the Prompt10 cleanup,
runner, and evidence changes. No reset, rebase, checkout of the native
baseline in the main worktree, or native-source modification was performed.

## 2. Prompt registration

| Item | Value |
|---|---|
| Original source | `/home/lc/super_livo/prompts/Prompt10 — Dead-Code Hygiene Closure + eee_01 Native-LIO vs Prob-LIO ATE & Memory Characterization.md` |
| Original SHA256 | `429834b82c0be07fb16f936d3d2fe3a832033f2c2e5210f0b07ee2978156bffd` |
| Registered copy | `prompts/prob_livo/prompt10_native_lio_memory.md` |
| Registered SHA256 | `87fc9ae6c6432b501070d6177f966d2961ac35be3ad9d93ec1e79c2f00491c9d` |
| Registration commit | `df6f19a` |

The workspace tracker `/home/lc/super_livo/prompts/README.md` and the repository
index `prompts/prob_livo/README.md` both reference Prompt10 and its stop boundary.

## 3. Phase A — bounded dead-helper audit

The audit was limited to `src/prob_livo/**`, `include/prob_livo/**`, direct
Prob-LIO adapters, and the Prompt8/9 visual-plane seam. No repository-wide
historical cleanup was attempted.

| Helper | Definition | Live callers | Current status | Action |
|---|---|---:|---|---|
| `InputSemanticsName` | `include/prob_livo/input_semantics.h`, removed definition was at the helper block after line 22 | 0 | obsolete formatting helper; not API/virtual and no symbol references | remove |
| `IsFinitePositiveSemidefinite` | `src/prob_livo/visual_plane_gate.cpp:9` | 1 production caller: `src/vio.cpp:915` | live upstream visual-point covariance producer validation and public header declaration at `include/prob_livo/visual_plane_gate.h:35` | keep |
| `ParseInputSemantics` | `include/prob_livo/input_semantics.h:12` | live runner/parameter path | active input contract | keep |
| `LidarHeaderTime`, `LidarEndTime` | `include/prob_livo/input_semantics.h:25,31` | live input/scheduler path | active timestamp contract | keep |
| `FiniteImu`, `SetFailure`, `WallTimeSeconds`, `RecordSensorTime` | Prob-LIO production seams | live | active validation/error/timing paths | keep |
| `MakeFilterOptions`, `ToImuSample`, ESKF/SO3 helpers | Prob-LIO production seams | live | active estimator/adapter paths | keep |
| `ProbPlaneProvider` and backend methods | `prob_plane_provider.*`, `prob_lio_backend.*` | live | active provider/backend contract | keep |

`IsFinitePositiveSemidefinite` was not deleted: its `SelfAdjointEigenSolver`
is still required by the upstream producer validation. The repeated query
hot-path solver removed in Prompt9 is a separate, already-closed change.
After cleanup, `InputSemanticsName` has no declaration or reference; no public
ABI, struct layout, floating-point order, parameter, or production control
flow was changed.

### Phase A compile/test gate

After cleanup commit `3cba6f07622b24459580b62a482d2ee132aa5b2e`:

```text
cmake --build /home/lc/super_livo/build --target all -- -j4     PASS
prob_livo_i1_tests                                          PASS
prob_livo_i2_tests                                          PASS
prob_livo_i3_tests   [PASS] checks=79                       PASS
prob_livo_p4_tests   [PASS] checks=201                      PASS
prob_livo_i4_tests   [PASS] checks=57                       PASS
prob_livo_i5_tests   [PASS] checks=115                      PASS
prob_livo_i6_visual_gate_tests [PASS] checks=19              PASS
```

The cleanup diff is five deleted lines in
`include/prob_livo/input_semantics.h`; static search found no dangling helper
declaration, new warning, or new error.

### Phase A zero-effect trajectory regression

Both canonical offline regressions were run after cleanup. Each comparison
used `cmp` on trajectory, authority counters, and visual counters; timestamps
are included in the byte comparisons.

| Regression | Rows | Official ATE (m) | Trajectory SHA256 | Sidecars | Result |
|---|---:|---:|---|---|---|
| H1 corrected cleanup | 3979 | `0.08795092773331592` | `9939bb6bc4688d9685cc156e6a473e2f7413e79a15585b8f72aba7170184f53a` | authority + visual byte-identical | PASS |
| H2 cleanup | 3979 | `0.08793104514326024` | `812d1bb9de1abeba0471632fa7515ee7afff682cbcad87bbab2786b109f5f6ca` | authority + visual byte-identical | PASS |

Evidence runs:

```text
results/prob_livo/runs/prompt10_h1_cleanup_regression
results/prob_livo/runs/prompt10_h2_cleanup_regression
```

This closes Phase A before the native benchmark.

## 4. Phase B authorities and execution

### Dataset/evaluator

| Item | Value |
|---|---|
| Bag | `/home/lc/super_livo/bag/NTU/eee_01/eee_01.bag` |
| Bag SHA256 | `7ea43946cffdd49c88d993ad3f192a4e90a8f6826eddc2ef1a9d4f5343ca6c17` |
| Bag duration | `398.687010 s` (`592523` messages) |
| Relevant topic counts | `/imu/imu` 153347; `/os1_cloud_node1/points` 3987; `/left/image_raw` 3986; `/leica/pose/relative` 6616 |
| Evaluator | `eval/prob_livo/eval_ntu_viral_official.py`, NTU official wrapper |

### Native N0 isolation

N0 used the detached worktree `/tmp/prompt10_fast_livo2_native` at exactly
`0d2c0346107b75b59934975adec9a6eeeb913c64`, with a separate build/devel
space at `/tmp/prompt10_native_ws`. The native source was clean before and
after all runs. Its own `config/NTU_VIRAL.yaml` and
`camera_NTU_VIRAL.yaml` were used; the camera model is loaded because the
baseline constructor requires the `laserMapping` camera contract even when
`common/img_en=0`, but no image topic is subscribed or replayed.

Native result collection is external: the baseline writes
`Log/result/<run-id>.txt`, and the wrapper copies that file to the run
directory. No native source or object file was changed.

The first two native attempts were excluded from the formal table: one was
blocked by the sandbox ROS XML-RPC socket policy, and one exposed the required
native camera-model parameter. A third pre-fair native run used
`dense_map_en=false`; it completed and produced the same trajectory, but was
excluded after the publish-policy audit. Formal N0 runs below use the matched
`dense_map_en=true` policy.

### Effective-config parity ledger

| Dimension | N0 native | H0 canonical | H0-LIOONLY |
|---|---|---|---|
| Authority | FAST-LIVO2 baseline `0d2c034…` | current Prob-LIO P0–P4 | current Prob-LIO P0–P4 |
| ROS mode | normal online callbacks | normal online callbacks | normal online callbacks |
| Replay | `/imu/imu`, `/os1_cloud_node1/points` | same + `/left/image_raw` | `/imu/imu`, `/os1_cloud_node1/points` |
| Camera/visual | `img_en=0`, visual OFF | image ingestion/scheduler ON, visual update OFF | image ingestion/scheduler OFF, visual OFF |
| Input semantics | native | `super_ntu_legacy` | `super_ntu_legacy` |
| LiDAR/IMU | enabled | enabled | enabled |
| Extrinsics/calibration | same NTU values (`extrinsic_T/R`, `Pcl/Rcl`) | same | same |
| `blind` / LiDAR time offset | native `1.0` / `-0.1` | canonical Prob overlay `2.0` / `0.0` | same as H0 |
| IMU | `acc_cov=0.5`, `gyr_cov=0.3`, `imu_int_frame=30` | same host values; Prob covariance pipeline active | same |
| LIO map/filter | native voxel/octree semantics, voxel `0.5`, native iterations `5` | Prob OctVox semantics, voxel `0.5`, Prob backend iterations `4` | same as H0 |
| Local map | sliding disabled | sliding disabled | sliding disabled |
| Publishing hygiene | dense map `true`; effect/plane `false` | same | same |
| PCD/image/RViz | PCD and image save OFF; no RViz | same | same |
| Rate/step | bag rate `1.0`; one callback path | bag rate `1.0`; `one_callback_step=false` | same |

The native `blind/time offset` and native-vs-Prob map/iteration semantics are
intentional authority differences, not parameter transplantation. They are
reported as confounds below.

## 5. ATE and trajectory authority

All formal runs used the same bag, evaluator, and normal online replay. The
repeated trajectories are byte-identical within each variant.

| Variant/run | LIO authority | Camera | Visual | Rows | Matched GT | ATE RMSE (m) | Trajectory SHA256 |
|---|---|---|---|---:|---:|---:|---|
| N0 fair 1 | FAST-LIVO2 native | OFF | OFF | 3985 | 3327 | `0.030768325230497` | `8906ba652f3862f9f6d11d7d9bdb785bdfd91557cd9ba010e8aee336b470033a` |
| N0 fair 2 | FAST-LIVO2 native | OFF | OFF | 3985 | 3327 | `0.030768325230497` | same as N0 fair 1 |
| N0 fair 3 | FAST-LIVO2 native | OFF | OFF | 3985 | 3327 | `0.030768325230497` | same as N0 fair 1 |
| H0 run 1 | Prob-LIO P0–P4 | scheduler ON | OFF | 3979 | 3326 | `0.09138258970792523` | `9f9eae6fe119d23260d23c4c10a0fba900ba32798d0a58239861432a11d3c53f` |
| H0 run 2 | Prob-LIO P0–P4 | scheduler ON | OFF | 3979 | 3326 | `0.09138258970792523` | same as H0 run 1/canonical |
| H0-LIOONLY run 1 | Prob-LIO P0–P4 | OFF | OFF | 3981 | 3329 | `0.09099574805341126` | `d06e472b04f7d304d1462b30b2077766f62bf7047cd4247f909c96b3ca277f03` |
| H0-LIOONLY run 2 | Prob-LIO P0–P4 | OFF | OFF | 3981 | 3329 | `0.09099574805341126` | same as H0-LIOONLY run 1 |

The required H0 identity is exact: rows, ATE, and SHA match the Prompt9
canonical reference. H0-LIOONLY is **not** byte-identical to H0, so it remains
diagnostic and is not a new canonical algorithm variant. The ATE deltas are:

```text
H0 - N0       = +0.06061426447742824 m (+197.002% relative to N0; ratio 2.9700)
H0-LIOONLY-H0 = -0.00038684165451397 m (-0.4233% relative to H0)
```

The second delta is not interpreted as a camera-infrastructure effect because
the two trajectories are not equivalent.

## 6. Memory methodology

`tools/prob_livo/memory_monitor.py` is an external monitor. It samples only
the actual mapper PID every 2 seconds, without estimator instrumentation. It
records `/proc/<pid>/status` fields `VmRSS`, `VmHWM`, `VmSize`, `RssAnon`,
`RssFile`, `VmSwap`, and `/proc/<pid>/smaps_rollup` fields `Rss`, `Pss`,
`Private_Clean`, `Private_Dirty`, `Shared_Clean`, `Shared_Dirty`, `Swap`.

`USS = Private_Clean + Private_Dirty`. Values below are MiB (`KiB/1024`).
The early baseline is the median of samples with elapsed time 20–40 s. Curve
points use the nearest recorded sample to each requested elapsed time. RSS,
PSS, and USS peaks are the maxima over the complete CSV, while `VmHWM` is the
kernel high-water mark.

### Per-run memory

| Run | Samples | Duration (s) | Early RSS/PSS/USS | Final RSS/PSS/USS | Peak HWM/PSS/USS |
|---|---:|---:|---|---|---|
| N0 fair 1 | 200 | 401.718 | 224.79 / 204.00 / 201.47 | 2675.70 / 2656.50 / 2655.22 | 2675.70 / 2656.50 / 2655.22 |
| N0 fair 2 | 200 | 401.903 | 238.16 / 217.01 / 214.46 | 2675.08 / 2655.51 / 2654.19 | 2675.08 / 2655.51 / 2654.19 |
| N0 fair 3 | 200 | 401.922 | 202.76 / 182.12 / 179.61 | 2676.73 / 2657.73 / 2656.47 | 2676.73 / 2657.73 / 2656.47 |
| H0 run 1 | 201 | 401.202 | 141.58 / 120.58 / 117.98 | 278.59 / 259.21 / 257.83 | 278.59 / 259.21 / 257.83 |
| H0 run 2 | 201 | 401.225 | 141.36 / 120.83 / 118.20 | 277.57 / 259.29 / 257.91 | 277.57 / 259.29 / 257.91 |
| H0-LIOONLY run 1 | 201 | 401.354 | 132.81 / 110.38 / 107.86 | 321.94 / 301.32 / 300.04 | 321.94 / 301.32 / 300.04 |
| H0-LIOONLY run 2 | 201 | 401.297 | 132.89 / 110.28 / 107.69 | 322.34 / 301.54 / 300.21 | 322.34 / 301.54 / 300.21 |

Raw CSVs and run metadata are under:

```text
results/prob_livo/runs/prompt10_n0_fair_run{1,2,3}/
results/prob_livo/runs/prompt10_h0_run{1,2}/
results/prob_livo/runs/prompt10_h0_lioonly_run{1,2}/
```

### Aggregate and repeatability

Group values are medians across the valid repeated runs.

| Group | n | Early RSS/PSS/USS | Final/peak RSS/PSS/USS | USS growth | Early USS spread | Final USS spread |
|---|---:|---|---|---:|---:|---:|
| N0 | 3 | 224.79 / 204.00 / 201.47 | 2675.70 / 2656.50 / 2655.22 | 2453.75 MiB | 17.300% — `MEMORY_ENVIRONMENT_NOISY` | 0.086% |
| H0 | 2 | 141.47 / 120.70 / 118.09 | 278.08 / 259.25 / 257.87 | 139.78 MiB | 0.185% | 0.033% |
| H0-LIOONLY | 2 | 132.85 / 110.33 / 107.78 | 322.14 / 301.43 / 300.12 | 192.35 MiB | 0.152% | 0.057% |

N0 required a third run because its first two early-window samples exceeded
3%; the third still leaves the early-window spread above 5%, so only that
early baseline is marked noisy. N0 end/peak memory is repeatable to 0.086%,
and all N0 trajectories/ATEs are byte/repeat identical.

### Memory comparison

| Comparison, final/peak median | RSS | PSS | USS |
|---|---:|---:|---:|
| N0 minus H0 | `+2397.62 MiB` (`+862.20%` vs H0) | `+2397.25 MiB` (`+924.69%`) | `+2397.35 MiB` (`+929.67%`) |
| N0 minus H0-LIOONLY | `+2353.56 MiB` (`+730.61%` vs H0-LIOONLY) | `+2355.08 MiB` (`+781.31%`) | `+2355.10 MiB` (`+784.72%`) |
| H0 minus H0-LIOONLY | `-44.06 MiB` (`-13.68%` vs H0-LIOONLY) | `-42.18 MiB` (`-13.99%`) | `-42.25 MiB` (`-14.08%`) |

By the interpretation bands, the observed late-memory reduction from native
N0 to either Prob-LIO run is LARGE. The H0/H0-LIOONLY difference is not
assigned to camera infrastructure because the control is not trajectory
equivalent.

### USS memory curve

Each cell is `RSS / PSS / USS` in MiB, aggregated by group median.

| Elapsed point | N0 | H0 | H0-LIOONLY |
|---:|---|---|---|
| 10 s | 202.76 / 182.02 / 179.50 | 140.57 / 119.94 / 117.32 | 129.93 / 107.59 / 105.04 |
| 25 s | 216.16 / 195.38 / 192.85 | 141.47 / 120.69 / 118.08 | 131.01 / 108.53 / 105.97 |
| 50 s | 264.62 / 243.69 / 241.16 | 142.11 / 121.35 / 118.73 | 137.57 / 115.15 / 112.60 |
| 75 s | 584.03 / 563.01 / 560.46 | 183.91 / 163.47 / 160.86 | 184.32 / 162.12 / 159.56 |
| 90 s | 808.65 / 788.08 / 785.57 | 192.94 / 172.72 / 170.10 | 197.46 / 175.24 / 172.69 |
| End | 2675.70 / 2656.50 / 2655.22 | 278.08 / 259.25 / 257.87 | 322.14 / 301.43 / 300.12 |
| Peak | 2675.70 / 2656.50 / 2655.22 | 278.08 / 259.25 / 257.87 | 322.14 / 301.43 / 300.12 |

## 7. Structural memory explanation

No intrusive map instrumentation was added. The available type sizes are:

| Type | `sizeof` |
|---|---:|
| Native `pointWithVar` | 384 bytes |
| Native `VoxelPlane` | 512 bytes |
| Native `VoxelOctoTree` | 184 bytes, excluding dynamic allocations |
| Native unordered-map entry | 32 bytes |
| Prob `LI2Sup::V3` | 12 bytes |
| Prob `LI2Sup::OctVox<LI2Sup::V3>` | 496 bytes |
| Prob `LI2Sup::OctVoxMap` object | 400 bytes, excluding dynamic map/list storage |
| Prob `KNNHeap<5, LI2Sup::V3>` | 536 bytes |

The native map owns an `unordered_map<VOXEL_LOCATION, VoxelOctoTree*>`; each
tree contains eight child pointers, a plane pointer, and dynamic octree/point
storage. Its `BuildVoxelMap`/`UpdateVoxelMap` path constructs a
`pointWithVar` for each downsampled point and updates tree nodes. Prob-LIO
owns an `OctVoxMap` backed by a robin map/list and stores representative
`V3` points with per-subvoxel counts and packed symmetric covariance. These
are different map representations, so the type sizes are explanatory, not a
strict per-point allocator cost.

Available production counters show the map scale reached by Prob-LIO:

```text
H0:          map_init_inserts=13761,  map_update_inserts=13654637,
             adapted_points=13654637
H0-LIOONLY:  map_init_inserts=13881,  map_update_inserts=13787674,
             adapted_points=13787674
```

The native baseline has no non-intrusive unique-voxel/node count API; that
count is therefore unavailable and was not fabricated. The approximately
2.66 GiB native final PSS/USS versus approximately 0.26–0.30 GiB Prob final
PSS/USS is consistent with the native dynamic octree/point/plane allocation
model, but also includes the intentional native map and input-semantics
differences.

## 8. Confound and hard-gate audit

| Gate/confound | Finding |
|---|---|
| Cleanup zero effect | H1 and H2 trajectory/timestamp/counter sidecars byte-identical; PASS |
| Native authority | detached exact baseline, clean, separate build; PASS |
| H0 identity | exact canonical rows/ATE/SHA; PASS |
| Estimator-only monitor | actual mapper PID only; monitor/roscore/rosbag excluded; PASS |
| Repetition | H0 and H0-LIOONLY <1% memory spread; N0 third run performed; trajectory/ATE repeat exact; PASS with N0 early noise noted |
| Online mode | all formal runs use normal ROS callbacks, `rosbag play`, rate `1.0`, one run at a time; PASS |
| Duplicate process/backlog | no duplicate mapper/bag; Prob runs have `play_rc=0`, `node_rc=0`, counters complete; H0 pending LiDAR 0, H0-LIOONLY pending 1 only at controlled shutdown |
| Thread/runtime | g++ 9.4, ROS Noetic, Eigen 3.3.7, `-O3 -march=native -mtune=native -funroll-loops -fopenmp -std=c++17`, TBB/libgomp, no mimalloc on both builds |
| Thread-count semantic difference | current Prob binary is compiled with `MP_PROC_NUM=32`; native baseline hard-caps its OpenMP macro at `MP_PROC_NUM=4`; recorded, not changed |
| Native shutdown | baseline writes complete result but exits `139` after controlled SIGINT; formal runner accepts this only with complete bag/result/GT/evaluation and records `node_rc_accepted=1` |
| Parameter semantics | native `blind=1.0`, lidar offset `-0.1`; canonical Prob `blind=2.0`, lidar offset `0.0`; intentional authority difference, not tuned |
| H0-LIOONLY role | not byte-identical to H0; diagnostic only, never relabeled canonical |
| Tuning/data structure changes | none in Prompt10 benchmark; no sweep, pruning, voxel/filter/noise/iteration/map-equation change |

The formal runner/tool commits are tool/evidence changes only:

```text
9b1e85f  tools(prob-livo): add Prompt10 memory characterization runners
5f5f541  fix(prob-livo): load native camera contract for LIO runner
1400cff  fix(prob-livo): record native shutdown exit in benchmark
76a50eb  fix(prob-livo): match native publish policy in Prompt10
9fd61eb  chore(prob-livo): harden Prompt10 runner status handling
```

## 9. Final interpretation and stop boundary

1. The bounded hygiene cleanup removed only the proven-dead
   `InputSemanticsName`; the live covariance producer helper was retained.
   Full build/tests and H1/H2 zero-effect regressions pass.
2. On this fixed `eee_01` official evaluation, native N0 has ATE
   `0.030768325230497 m`, while canonical H0 has ATE
   `0.09138258970792523 m`. This is a characterization result, not an ATE
   tuning instruction.
3. Native N0 reaches about `2.66 GiB` final PSS/USS; H0 reaches about
   `0.26 GiB`, and the diagnostic H0-LIOONLY reaches about `0.30 GiB`.
   The late growth difference is large and structurally consistent with the
   different native dynamic octree versus Prob representative OctVox map.
4. N0 early 20–40 s memory is environment-noisy after the required third
   run, but its final/peak memory and trajectory are highly reproducible.
   Because H0-LIOONLY is not byte-equivalent, no camera-constant-memory claim
   is made from H0 versus H0-LIOONLY.

**`PROMPT10 CHARACTERIZATION CLOSED`**

STOP FOR OWNER. Do not enter LIVO/H1/H2/I8 from this Prompt10 run.
