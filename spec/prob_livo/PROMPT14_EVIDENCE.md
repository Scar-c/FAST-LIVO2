# Prompt14 Evidence

## Final status

`PROMPT14 CLOSED — ACCEPTED NATIVE LIVO NONDETERMINISM`

The native LIVO result is accepted under the Prompt14 contract. Native LIO
remains an exact online/offline contract. No multi-dataset, I8, P5, H1, or H2
benchmark was started.

Prompt14 SHA256:

`7da0adeb8f499450649a01c0e7b62e7262d930dc59261238d9b376c72f37337c`

All experiments used `/home/lc/super_livo/bag/NTU/eee_01/eee_01.bag`, the
native `FAST-LIVO2` implementation on branch `prompt11-native-offline`, and
compile-time `FAST_LIVO_MP_PROC_NUM=1`. The primary `prob-livo` worktree was
not modified.

## Phase A — sanitizer and one allowed correctness fix

An isolated ASAN/UBSAN visual build covered the first 60 seconds of eee_01.
The first visual-path fault was an invalid `bool` load at
`PointToPlane::is_valid_` in `VoxelMapManager::BuildResidualListOMP`; it was
not a photometric or shutdown fault. The red regression filled the struct
storage with a nonzero byte pattern and failed before the fix. The local fix
is the in-class initialization `bool is_valid_ = false` in
`include/voxel_map.h`.

After the fix, the sanitizer test suite passed:

- `native_offline_reader_tests`
- `native_runner_status_tests`
- `cached_mean_oracle_tests`
- `point_to_plane_init_tests`

The post-fix 60-second ASAN/UBSAN run reported no in-flight visual
ASAN/UBSAN fault. Its remaining sanitizer termination was the previously
isolated PCL teardown path, which was handled separately in Phase B.
No second estimator correctness patch was chained after this fix.

## Image-boundary hard audit

The audit was source-based and checked the actual byte/index footprint. With
`patch_size_half=4`, pyramid scale `s=2^level`, a patch starts at
`floor(pc/s)*s - 4s`, reaches the patch endpoint plus bilinear neighbours,
and the gradient path reaches both `-s` and `+2s` relative to the current
sample. The initial retrieval/generation guards use the configured border,
but the iterative optimizer reprojection paths do not re-run that guard.

| Function | Iterative reprojection | Patch/gradient access | Current reprojection check | Result |
|---|---|---|---|---|
| `retrieveFromVisualSparseMap` | no optimizer iteration | depth neighbourhood around `pc` | `isInFrame(..., border)` before access | conditionally safe under border |
| `getImagePatch` | no | direct bilinear bytes | caller-dependent | no local guard |
| `precomputeReferencePatches` | no current-state reprojection | reference patch gradients | stored-reference invariant only | no runtime OOB reproduced |
| `updateStateInverse` | yes, `pc = cam->world2cam(pf)` each iteration | direct bilinear bytes | no current-iteration check | latent boundary risk |
| `updateState` | yes, `pc = cam->world2cam(pf)` each iteration | direct bilinear/gradient bytes | no current-iteration check | latent boundary risk |

The direct accesses are at `src/vio.cpp:1671-1683`, `1763-1767`, and
`1919-1931`; the iterative projections are at `src/vio.cpp:1746-1747` and
`1895-1897`. The tested ASAN trace and the adversarial/index-range review did
not establish a concrete out-of-bounds input on this dataset, so this was not
patched after the one allowed Phase-A fix. A future fix, if independently
proven, must reject a measurement whose current projected footprint is out of
range; no clamping, patch-size, residual, or extrapolation change was made.

## Diagnostic signatures and first divergence

Diagnostic-only signatures use semantic values and FNV hashes; no pointer
addresses are hashed. Each camera epoch emits S0 (VIO entry), S1 (retrieval),
S2 (level/iteration optimizer), and S3 (reference/map update).

The final four diagnosis runs were:

- O1: `prompt14_contract_livo_online_1`
- O2: `prompt14_contract_livo_online_2`
- F1: `prompt14_contract_livo_offline_1`
- F2: `prompt14_contract_livo_offline_2`

All were full eee_01 runs with camera ON and MP=1. Signatures were aligned by
`(epoch, stage, level, iteration)`, not by CSV row number, because the number
of optimizer iterations itself can vary after divergence.

| Pair | First epoch | Stage | First differing field(s) | Previous epoch identical |
|---|---:|---|---|---|
| O1/O2 | 601 | S0 | `state_hash`, `covariance_hash`, `lidar_input_hash` | yes, epoch 600 |
| F1/F2 | 1579 | S2, level 0, iteration 0 | `error`, `state_hash`, `compact_htz_hash`, `branch` | yes, epoch 1578 |
| O1/F1 | 601 | S0 | `state_hash`, `covariance_hash`, `lidar_input_hash` | yes, epoch 600 |

The online pair diverges at the visual-seam entry because the native state and
the semantic `pointWithVar` input already differ; the offline pair later
diverges in the optimizer. This is not an offline queue/event mismatch:
callbacks, timestamps, scheduler counts, LiDAR/camera epochs, and IMU
coverage are exact across all final samples. The repeated online drift and
repeated offline drift establish intrinsic native execution nondeterminism,
with an order-sensitive native map/point path and subsequent visual numerical
branching. The compile-time MP=1 result rules out an OpenMP-thread-count-only
explanation.

Classification:

`NATIVE_INTRINSIC_LIVO_NONDETERMINISM`

The accepted contract therefore uses `ACCEPTED_NATIVE_LIVO_NONDETERMINISM`;
it does not claim byte-identical native LIVO trajectories.

## Accepted Native LIVO contract: Online x3 / Offline x3

Final run directories are under `/tmp/prompt14_contract_runs` and all have
`RC=0`, `CLEAN_SUCCESS`, `eof_drained=1`, `input_queues_drained=1`,
`no_processable_epoch_remaining=1`, `trajectory_flushed=1`,
`counters_flushed=1`, and `expected_final_epoch_reached=1`.

| Run | Source | Rows | ATE RMSE (m) | Trajectory SHA256 |
|---|---|---:|---:|---|
| online_1 | normal online | 3983 | 0.028585596165 | `1ac14df81020a61e92baafc04d54725c30dfe8ae02c2f264f2ae9468625f1198` |
| online_2 | normal online | 3983 | 0.028387730224 | `1801f43d6860321a8c1698d5a26ccdd90c86e21040117ed57e5a3cbe3742b0d0` |
| online_3 | normal online | 3983 | 0.028510174212 | `51ad69f5b793a4a7580caa14d51a29083ec5242a1cdd503c59e97e5909f92bad` |
| offline_1 | native offline | 3983 | 0.028717055215 | `8fb4de2d607e9a43ac0b0bfb9fbaac5f9abd8f606f24836725578906342b4ffc` |
| offline_2 | native offline | 3983 | 0.028718576782 | `ae764b920dd0fed78fe2df939e405b20c8eb51e2f2cf97199b6f984317945625` |
| offline_3 | native offline | 3983 | 0.028745528936 | `abf336bd5df4784a220b3fd1dddecdf364e8d4457ecc8b74874cd6d122f88e41` |

Pairwise translation results:

- maximum online-online RMSE: `0.005898058791 m`; maximum online-online
  pointwise translation difference: `0.031328668708 m`;
- maximum offline-offline RMSE: `0.001586025836 m`;
- online-offline RMSE median: `0.006646360464 m`;
- online-offline RMSE maximum: `0.006897768543 m`;
- maximum online-offline pointwise translation difference:
  `0.060157675703 m`.

The fixed envelope was checked before interpretation:

```text
median online-offline RMSE 0.006646360464
  <= 2 * max online-online RMSE 0.011796117582       PASS

cross-group ATE spread 0.000357798712
  <= 2 * online-online ATE spread 0.000395731883      PASS
```

ATE spreads were: online-online `0.000197865942 m`, offline-offline
`0.000028473721 m`, and cross-group range `0.000357798712 m`.

Fine visual counter repeat spreads were also bounded at native scale. Online
ranges were: created points `377`, candidate replacements `7537`, reference
commits `635`, plane queries `1194`, and final map size `3`. Offline ranges
were: created points `486`, candidate replacements `1280`, reference commits
`120`, plane queries `464`, and final map size `2`. The offline created-point
spread is slightly larger numerically (`486` vs `377`) but is only `0.14%` of
the roughly 360k total and is not a material envelope excess; all other fine
counter spreads are below the online repeat envelope. The final-map values
range from 143 to 148 across all six runs.

Hard invariants were exact across all six LIVO runs:

```text
IMU callbacks / enqueued       153347 / 153323
LiDAR callbacks / enqueued       3987 / 3987
image callbacks / enqueued       3986 / 3985
ignored input messages              25
scheduler steps / sync packages 7967 / 7967
LiDAR epochs / camera epochs    3984 / 3983
IMU processing calls                7967
LiDAR updates / map queries       3983 / 3983
map updates                         3983
VIO calls / visual commits       3983 / 3982
trajectory rows                     3983
```

All six timestamp sequences are pairwise exact. Each online run discarded 42
unprocessable EOF-tail messages after all processable epochs; each offline
run recorded the same 42-message discard, with source accounting showing
3987 LiDAR, 153347 IMU, and 3986 image records read.

## Native LIO exact contract

Final canonical runs:

- `prompt14_final_lio_online`
- `prompt14_final_lio_offline`

Both returned `RC=0 / CLEAN_SUCCESS`, 3985 rows, exact timestamps, and the
same trajectory SHA:

`8593887f73c2485287c649dd92a7aa282e60d08595aa370734838e9f0cdb71be`

Both had IMU callbacks/enqueues `153347/153323`, LiDAR callbacks/enqueues
`3987/3987`, ignored messages `24`, scheduler steps/sync `3987/3987`, LiDAR
epochs `3987`, LiDAR updates/map queries/map updates `3985/3985/3985`, and
trajectory rows `3985`. Both completion sentinels had EOF, queues drained, no
processable epoch remaining, expected final epoch, trajectory flush, and
counter flush set to 1. The LIO trajectory files are byte-identical.

## Phase B — shutdown-139 root cause and fix

The original controlled gdb trace reached:

`LIVMapper::~LIVMapper()` -> Boost shared-pointer disposal ->
`pcl::PointCloud<PointXYZINormal>` vector destruction -> Eigen aligned free.

The exact member was `LIVMapper::feats_down_body`; gdb printed its PCL object
and its populated vector storage before the failing free. The temporary gdb
script was corrected to ignore normal `free(nullptr)` and to inspect `$rdi`
for the x86-64 allocator argument. After the fix, the 20-second controlled
LIVO gdb run ended with `gdb_rc=0` and `[Inferior ... exited normally]`, with
all PCL cloud destructors observed and no non-null low-address free.

| Object/member | Type/allocator | Owner and alias | Destruction observation | Fault |
|---|---|---|---|---|
| `LIVMapper::feats_down_body` | `boost::shared_ptr<PointCloudXYZI>`, PCL vector/Eigen allocation | mapper owns the Boost control block; `VoxelMapManager::feats_down_body_` is a shared-pointer alias to the same object | exact cloud destructor observed in mapper teardown | allocator ABI mismatch before fix |
| `ImuProcess::cur_pcl_un_` | separate Boost PCL shared pointer | independent owner | normal destructor | none |
| `LidarMeasures.lidar`, `pcl_proc_cur`, `pcl_proc_next` | separate Boost PCL shared pointers | independent owners | normal cloud destructors | none |
| `visual_sub_map`, `feats_undistort`, `feats_down_world`, `pcl_w_wait_pub`, `pcl_wait_pub`, `pcl_wait_save_intensity` | separate Boost PCL shared pointers | no duplicate raw ownership found | normal cloud destructors | none |

This was not double free, UAF, an alive worker, or duplicate ownership. The
system Debian PCL binaries use Eigen's 16-byte allocation ABI. The project
was compiling with `-march=native`, which enabled AVX and made Eigen select a
32-byte default allocator in the application translation units. Setting only
`EIGEN_MAX_ALIGN_BYTES=16` was insufficient because Eigen still selected its
ideal 32-byte default. The minimal production fix is in `CMakeLists.txt`:

- x86 release flags retain `-march=native`/SSE but add `-mno-avx`;
- `EIGEN_MAX_ALIGN_BYTES=16` is defined globally for the native targets.

The compile-time check reports `EIGEN_IDEAL_MAX_ALIGN_BYTES=16`,
`EIGEN_DEFAULT_ALIGN_BYTES=16`, and `EIGEN_MALLOC_ALREADY_ALIGNED=1`, while
SSE vectorization remains enabled. The fix is commit `4a57b10`.

## Completion sentinel and runner integrity

The production seam now emits and the runner validates:

- source EOF (`eof_drained`);
- callbacks consumed/accounted (offline source report plus exact runtime
  callback counters; exact bag counts are recorded in this report);
- input queues empty (`input_queues_drained`);
- no processable epoch remaining;
- expected final epoch reached;
- trajectory flushed and counters flushed;
- actual node RC preserved in metadata.

The online drain waits after rosbag EOF and stops only after bounded idle
processing; unprocessable EOF tails are counted and cleared. The offline
runner uses the same production scheduler seam and performs the same bounded
tail handling. Runner status tests cover the required sentinel fields and
preserve nonzero shutdown faults; final production runs now demonstrate
clean RC=0 rather than masking 139.

## Commits

- `9f346a3` — initialize point-to-plane validity and add visual diagnostics;
- `211627d` — drain online input before completion;
- `e937ab9` — detect an exited mapper during online drain;
- `6500070` — discard unprocessable EOF tails;
- `4a57b10` — match PCL Eigen allocation alignment;
- final documentation commit — this Prompt14 evidence report.

Final state is accepted native LIVO nondeterminism under the frozen contract.
STOP. Do not automatically start the next benchmark family.
