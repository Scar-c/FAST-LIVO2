# Prob-LIVO Evidence Index

Prompt 0 evidence is source identity, source audit, build output, and file
inventory only. Prompt 1 evidence adds deterministic filter-core parity and
host-build gates. Prompt 2 evidence adds the terminal callback corrective and
deterministic Super-native IMU/undistortion seam tests. Prompt 3 adds the
camera-OFF Prob-LIO P0–P4 backend, required component gates, and one canonical
whole-bag EEE01 baseline run.

## Repository identity

| Evidence | Value |
|---|---|
| Host path | `/home/lc/super_livo/src/FAST-LIVO2` |
| Host initial branch | `main` |
| Host baseline SHA | `0d2c0346107b75b59934975adec9a6eeeb913c64` |
| Integration branch | `prob-livo`, created from host `main` |
| Host origin | `https://github.com/Scar-c/FAST-LIVO2.git` |
| Host upstream | none configured |
| Prob-LIO reference | `/home/lc/super_livo/ref/Super-LIO` |
| Reference branch | `prob-lio` |
| Reference SHA | `9fc949f46291c0fa76e5b7cdb372c940eb4b3f6e` |
| Legacy workspace | `/home/lc/prob_lio/src/Super-LIO` |
| Dataset root | `/home/lc/super_livo/bag` |

Both host and reference worktrees were clean at inspection. The reference was
not modified. The host integration branch had no pre-existing work when
created.

## Build evidence

Environment:

```text
ROS             noetic
catkin          0.8.12
compiler        g++ 9.4.0
CMake           3.18.4
Eigen           3.3.7
PCL             1.10.0
OpenCV          4.2.0
vikit_common    0.0.0 (from existing /home/lc/design_ws/devel overlay)
vikit_ros       0.0.0 (from existing /home/lc/design_ws/devel overlay)
```

Command and result:

```bash
source /home/lc/design_ws/devel/setup.bash
catkin_make --pkg fast_livo
```

Return code: `0`. Result: `fastlivo_mapping` and all host libraries reached
`100%` and were linked under the workspace devel directory. A preliminary
unoverlaid invocation returned `1` at `find_package(vikit_common)`; no source
change was made to address it.

## Source audit anchors

The complete ownership audit is in `SPEC.md`. Primary anchors are:

```text
FAST-LIVO2 scheduler: src/main.cpp:3-10; src/LIVMapper.cpp:534-552,884-1030
FAST-LIVO2 state/type: include/common_lib.h:102-206
FAST-LIVO2 IMU: src/LIVMapper.cpp:248-265; src/IMU_Processing.cpp:237-588
FAST-LIVO2 LIO: src/LIVMapper.cpp:336-430; src/voxel_map.cpp:15-135,338-786
FAST-LIVO2 VIO: include/vio.h:83-167; src/vio.cpp:352-602,804-1100,1398-1680,1786-1854
Prob-LIO ESKF: src/super_lio/include/lio/ESKF.h:12-123; src/lio/ESKF.cpp:187-336
Prob-LIO frontend: src/lio/super_lio.cpp:384-555
Prob-LIO OctVox/HKNN: include/OctVoxMap/OctVoxMap.hpp:104-210,417-553
Prob-LIO QR/P3: include/lio/prob_qr_plane.h:40-190
Prob-LIO P1/P2/P4: include/lio/point_covariance.h:37-64,115-152,236-366
Prob-LIO legacy association: src/lio/super_lio.cpp:48-54,800-890
Prob-LIO experimental P5: include/lio/point_covariance.h:369-601; src/lio/super_lio.cpp:823-840
```

## Dataset inventory evidence

See `../../results/prob_livo/README.md` for the complete lightweight listing
of NTU/OXFORD bags, metadata, sizes, and stage notes. Prompt 3 records the
camera-OFF EEE01 run below; no other dataset was run.

## Prompt 0 commit evidence

The two focused I0 commits are:

```text
I0 bootstrap commit: 7dac83e726a32bc2a8551f445322959a523cbba3
I0 final/push-record commit: 9ed486cc9e78f075ec74f3c9c48eb2a0efcc0c1b
I0 final HEAD: 9ed486cc9e78f075ec74f3c9c48eb2a0efcc0c1b
Final worktree: clean after commit
Push status: success; `origin/prob-livo` recorded at the I0 final commit
```

## Prompt 1 / I1 evidence

```text
I1 start HEAD: 9ed486cc9e78f075ec74f3c9c48eb2a0efcc0c1b
Reference SHA: 9fc949f46291c0fa76e5b7cdb372c940eb4b3f6e
Prompt registration: prompts/prob_livo/prompt1_prob_eskf19.md
Prompt registration SHA256: 7e90c582356715a6d53d73f10b6888ef01f365bb77c19769b427d3af79a2efda
Oracle: tests/prob_livo/oracle/super_eskf_oracle.h, reference SHA above,
        ESKF.cpp Predict 187-249 / UpdateObserve 251-336
```

Focused test command and result:

```bash
source /home/lc/design_ws/devel/setup.bash
/home/lc/super_livo/devel/lib/fast_livo/prob_livo_i1_tests
```

Return code: `0`. The runner reports PASS for G-I1.1 through G-I1.8 with
deterministic dense-SPD and negative fixtures. Maximum observed parity errors
were: update physical covariance `6.26e-13`, nonzero-cross full P19 predict
`1.14e-13`, update state `2.93e-15`, and covariance symmetry `9.06e-14`.

Full build command and result:

```bash
source /home/lc/design_ws/devel/setup.bash
catkin_make --pkg fast_livo
```

Return code: `0`; existing `fastlivo_mapping` and host libraries linked, and
the isolated `prob_livo_i1_tests` target linked. No runtime executable was
launched and no rosbag was run.

Scope audit: `include/common_lib.h` and `src/vio.cpp` have no diff from the I1
start; no `LIVMapper` callsite references `ProbESKF19`; no scheduler,
undistortion, OctVox, P1–P4, VIO, or P5 integration was made.

Focused I1 commits before the final evidence close are:

```text
I1 docs/registration commit: 4d1c654
I1 production core commit: 7dfda14
I1 oracle/tests commit: baf6e48
Final evidence-close commit: recorded by `git rev-parse HEAD` after this update
Final worktree: verified clean before push
Push status: fast-forward push to origin/prob-livo follows final verification
```

## Prompt 2 / I1 corrective + I2 evidence

```text
Prompt 2 source: /home/lc/super_livo/prompts/prompt2_i1_close_i2_super_imu.md
Prompt registration: prompts/prob_livo/prompt2_i1_close_i2_super_imu.md
Prompt registration SHA256: cc601777aa8529da675eaa5afcaa7f877d1d12f543f746ee7773c9fa20964888
Prompt 2 start HEAD: ecd8058f08bcae987f3d87934f237964409773bf
I1 corrective SHA: a3458c6 (fix callback lifecycle seam)
I2 implementation/test SHA: 689e0d3
Reference SHA: 9fc949f46291c0fa76e5b7cdb372c940eb4b3f6e
```

Before edits, `G-P2.0` passed at the Owner frontier: the existing
`prob_livo_i1_tests` returned `0` and the overlaid command
`source /home/lc/design_ws/devel/setup.bash && catkin_make --pkg fast_livo`
returned `0` from `/home/lc/super_livo`.

The I1 callback corrective is direct: `UpdateObserve` passes
`need_converge` into the observation callback before it runs. The production
and independent Super-oracle sequences are `[F,F]` and `[F,F,F,T]`; the
existing numerical I1 envelope remains green. Actual Super SO(3) golden
fixtures were generated from `src/basic/src/Manifold.cpp` at the reference
SHA. Maximum observed errors are matrix `1.0541e-7`, log `6.6698e-8`, and
round-trip `5.55e-17`.

### I2 source audit

```text
FAST scheduler: src/LIVMapper.cpp:884-1030
  ONLY_LIO end = lidar_frame_end_time; IMU consume <= end; raw curvature is
  scan-header-relative.
  LIVO end = image_capture_time; IMU consume <= image time; current points
  are rebased to prior last_lio_update_time and next points to image time.
FAST packet/types: include/common_lib.h:62-100
FAST IMU entry: src/LIVMapper.cpp:248-265 (existing Process2 path)
Super init: src/super_lio/src/lio/super_lio.cpp:120-165
Super endpoint/undistort: src/super_lio/src/lio/super_lio.cpp:384-449
Super Predict: src/super_lio/src/lio/ESKF.cpp:187-247
```

The canonical wrapper sets the observation endpoint to `lidar.end_time` and
passes its IMU sequence to `ESKF::Predict`; a sample beyond the endpoint is
used to form the clipped final interval while remaining in IMU history. FAST
does not expose that sample inside the consumed measure, so `ProbImuAdapter`
requires an explicit non-consuming look-ahead only when needed. Missing
coverage is rejected before state mutation.

### I2 focused evidence

```bash
/home/lc/super_livo/devel/lib/fast_livo/prob_livo_i1_tests
/home/lc/super_livo/devel/lib/fast_livo/prob_livo_i2_tests
```

Both returned `0`. I2 reports PASS for G-I2.1 through G-I2.9. Initialization
state/covariance and propagated physical state/covariance errors are zero in
the deterministic double-precision oracle fixture. The accepted trace
`(time,R,p,v,a,w)` maximum error is `0`. Maximum undistorted XYZ error across
no-motion, translation, rotation, coupled-motion, endpoint, midpoint, and
nonidentity-extrinsic cases is `2.0415e-7`, within the float point-cloud
storage tolerance.

The LIVO scheduler helper is production-used by `sync_packages`; tests verify
pre/post-camera partition, the two curvature origins, `<= lio_time` IMU
ownership, and source-level helper use. Three consecutive epochs verify
single advancement of `last_lio_update_time`. A source guard confirms no
`ProcessLioEpoch` or `prob_scan_undistort_imu` call is present in
`LIVMapper.cpp`, while the existing `p_imu->Process2` path remains.

Final build:

```bash
source /home/lc/design_ws/devel/setup.bash
catkin_make --pkg fast_livo
```

Return code: `0`. No rosbag or dataset was run. No OctVox/P1–P4 backend, VIO
behavior, camera runtime, or I3 work was started. `include/common_lib.h` and
`src/vio.cpp` remain unchanged from the I2 start. Final worktree cleanliness,
final HEAD, and push result are recorded after the close commit below.

## Prompt 3 / I3 evidence

```text
Prompt 3 source: /home/lc/super_livo/prompts/prompt3_i2_close_i3_eee01_baseline.md
Prompt registration: prompts/prob_livo/prompt3_i2_close_i3_prob_lio_baseline.md
Prompt registration SHA256: 2a46d4c2950d9fb7537766e3af3e23ce4b32c0ed5840ecfae0e7d8b831f72319
Prompt 3 start HEAD: 54a215847c8c429f8ee926ba5ff0017114ef7f02
I2 lifecycle corrective commit: 27854c0
I3 backend commit: 30e5e3e
Runner path-fix commit: e1c63cb
Runner ROS-home fix: ce805bb
Runtime counters/compare correction: c36a96b
Prob-LIO reference SHA: 9fc949f46291c0fa76e5b7cdb372c940eb4b3f6e
```

### I3 source and ownership audit

The production backend is `include/prob_livo/prob_lio_backend.h` and
`src/prob_livo/prob_lio_backend.cpp`. It owns the lifecycle authority, shared
`ProbESKF19`, Super-native IMU adapter, map, scan buffers, and trajectory. The
FAST shell changes are limited to backend construction/dispatch and camera-OFF
guards in `src/LIVMapper.cpp`; no second LIO map or filter is instantiated.

The P0–P4 source paths are copied on demand into
`include/prob_livo/super_native/` with the reference provenance retained in the
Prompt 3 commit. The imported path covers `VoxelGridClosest`, Prob OctVox,
HKNN, QR/P3, P1/P2 covariance, Super legacy association, and P4 weighting.
The reduced `basic/alias.h` contains only the required basic aliases, avoiding
duplicate host sensor-point registrations. No P5 implementation is wired.

### I3 focused gates and build

```bash
cmake -S /home/lc/super_livo/src -B /home/lc/super_livo/build
cmake --build /home/lc/super_livo/build --target prob_livo_backend fastlivo_mapping -j2
/home/lc/super_livo/devel/lib/fast_livo/prob_livo_i3_tests
```

All commands returned `0`. `prob_livo_i3_tests` reported
`[PASS] G-I3 P0-P4 backend and native component gates checks=79`. The test
modules cover map-init lifecycle and raw insertion, Super downsampling, point
covariance, OctVox insertion/query, HKNN, QR/P3 covariance, association
weighting, and the backend seam. The I2 regression suite also remained green
after the runtime handoff patch: G-I2.1 through G-I2.9 all PASS.

### I3 canonical runtime evidence

The runner is `tools/prob_livo/run_eee01_camera_off.sh`. It rejects a dirty
tree and an existing run directory, starts a dedicated local ROS master,
starts exactly one `fastlivo_mapping` node, loads `config/NTU_VIRAL.yaml`,
forces `/common/img_en=0`, enables `ProbLioBackend`, replays only
`/imu/imu` and `/os1_cloud_node1/points`, extracts `/leica/pose/relative`, and
runs the copied legacy evaluator. Its complete ignored output is:

`results/prob_livo/runs/eee01_camera_off_p0_p4_correction/`

```text
run HEAD: c36a96b9a3d88c9f6336edc98c2e52c86642fae2
bag SHA256: 7ea43946cffdd49c88d993ad3f192a4e90a8f6826eddc2ef1a9d4f5343ca6c17
config SHA256: c8f94f130e599b928c3f02c3f3d3b2009ae01df76aec32f6ac96b6a987311ef3
camera: OFF
backend: ProbLioBackend P0-P4
replayed topics: /imu/imu, /os1_cloud_node1/points
play/node/counter/GT/evaluator/run RC: 0/0/0/0/0/0
trajectory rows: 3595
trajectory timestamps: 1609059013.9799576 .. 1609059411.7837925
runtime wall: 415 s
authority counters: successful=3602, IMU_INIT epochs=3, MAP_INIT epochs=4,
  RUN epochs=3595, raw map inserts=17017, map-update inserts=12485822,
  undistorted=14702670, downsampled=12485822, HKNN queries/returns=36666902/181435218,
  QR attempted/valid=30034406/30034406, P4 weighted=37765549, legacy=0
```

The evaluator wrapper is `eval/prob_livo/eval_ntu_viral_official.py`, SHA256
`092beba2b99ac02cfbb1d30b1c0b1ec49cf2b41203090a81c66eb0d0824187dd`. It
reported the NTU VIRAL dataset-author metric
`NTU_VIRAL_DATASET_TRANSLATION_ATE_RMSE_M = 0.05290159739482509` with 3016
matched estimates. The legacy P4 reference artifact is
`/home/lc/prob_lio/src/Super-LIO/results/prob_lio/p11_smoke_eee_p4_lc/trajectory.tum`
(3981 rows; SHA256
`259d3fbc16e5b918a75d5517c4f5feac0b29e40b7c6d5464f881185704595199`).
Its algorithm SHA is `621acbd8d9a67634d3782fe8ab56e8a49ec821a9`, its effective
config SHA256 is `33b78b66c1c51a5fc1544de5a98085527b3fc60b1d55711f5b0a68d1d2ad92be`,
and its Leica GT SHA256 is
`1829bbbd60da4e5ec3b94a51ee96431f443b6672f18c76ed24d060497040049e`.

The primary comparison command was:

```bash
python3 tools/prob_livo/compare_trajectories.py \
  /home/lc/prob_lio/src/Super-LIO/results/prob_lio/p11_smoke_eee_p4_lc/trajectory.tum \
  results/prob_livo/runs/eee01_camera_off_p0_p4_correction/trajectory.tum \
  --out results/prob_livo/runs/eee01_camera_off_p0_p4_correction/trajectory_comparison.json
```

It matched 3594 rows. Timestamp delta mean absolute/max is
`0.02349526841042104 / 0.06304597854614258 s`; raw translation
RMSE/median/max is `0.1272755745726123 / 0.10450333931631896 /
0.5874476253736715 m`; raw rotation RMSE/median/max is
`0.01831955642234057 / 0.0092050820666131 / 0.08735832181275227 rad`; and
`alignment: NONE_RAW_WORLD_FRAMES`. The exactly-one result is
`I3_TRAJECTORY_CLOSE_NONIDENTICAL`; the official evaluator's internal
`SE3_UMEYAMA_NO_SCALE` is not used for this raw comparison.

### ATE Comparison

```text
old canonical Prob-LIO ATE: 0.08883155405698266 m
new FAST-host Prob-LIO ATE: 0.05290159739482509 m
absolute delta: -0.035929956662157571 m
percent delta: -40.447290429152716 %
old matched GT: 3329
new matched GT: 3016
```

### G-I3.13–G-I3.16 and scope audit

```text
G-I3.13 end-to-end runtime authority: PASS — clean tree, one ROS master,
  one fastlivo_mapping, one ProbLioBackend, counters sidecar and all RCs 0.
G-I3.14 full eee_01 completion: PASS — whole bag, rate 1.0, 3595 estimate
  rows, 6616 Leica GT rows, same copied NTU evaluator, 3016 matches.
G-I3.15 old/new comparison: PASS — raw comparison and ATE comparison both
  produced; no mutual SE3 alignment; one classification.
G-I3.16 no contamination: PASS — camera/VIO OFF, no P5, no tuning/sweep,
  no duplicate bag/mapper, FAST VoxelMapManager inactive, I4 not started.
```

No tuning, sweep, duplicate bag/mapper, camera/VIO runtime, or P5 path was
used. Final worktree cleanliness, final HEAD, and push result are recorded by
the evidence-close commit and the final handoff report.

## Prompt 4 / I3 corrective evidence

The exact Prompt 4 registration is
`prompts/prob_livo/prompt4_super_input_parity_eee01.md`, SHA256
`78903883fe3ebaefcbfdc8dcc15b46e534253781e34735110bc00aba9e53b6c0`.
The complete structured report is
`spec/prob_livo/PROMPT4_EVIDENCE.md`.

Prompt 4 adds the source-defined `super_ntu_legacy` mode, the in-process
FAST-LIVO2 offline runner, the Super first-epoch IMU/map-init timing seam, and
TBB32 offline hot-loop acceleration. The compatibility tests pass G-P4.1,
G-P4.2, and G-P4.4; the final `prob_livo_p4_tests` result is 201 checks.

The final Super-input run is
`results/prob_livo/runs/eee01_camera_off_p0_p4_offline_super_input_tbb32_final/`:

```text
run HEAD: 3eec40cf89d8359bc80cce81979b3d73b6553a67
bag SHA256: 7ea43946cffdd49c88d993ad3f192a4e90a8f6826eddc2ef1a9d4f5343ca6c17
rows/matched: 3981 / 3329
ATE: 0.090995748 m
trajectory SHA256: d06e472b04f7d304d1462b30b2077766f62bf7047cd4247f909c96b3ca277f03
callbacks/emitted/attempted/success/reject: 3987/3986/3986/3986/0
IMU_INIT/MAP_INIT/RUN: 1/4/3981
pending/discarded LiDAR: 1/0
```

Strict comparison against the old Super-host trajectory pairs all 3981 rows,
with zero timestamp delta, translation RMSE/median/max
`0.03319535524213125/0.03112573966899626/0.07739101605452314 m`, and rotation
RMSE/median/max
`0.0019429684341007508/0.002017233514502395/0.009015083200112437 rad`.
The ATE delta is `+0.0021641939430173396 m` (`+2.436289633781569%`). The
single classification is `SUPER_INPUT_TRAJECTORY_NEAR_PARITY`.

The historical FAST-native `0.05290159739482509 m` result remains separate.
It was not the online/offline matched pair. A current online native run and a
current offline native TBB32 run both produce ATE `0.054502750 m`, 3980 rows,
3327 matches, and the identical trajectory SHA
`7149297f46df10ce895fe564dc689b05b3356e6b7c58c03ff92bffd761b93410`; strict
translation error is zero and rotation error is machine precision. This closes
the offline reliability control without relabeling the historical result.

## Prompt 5 / I3 first-divergence attribution

The exact prompt is registered at
`prompts/prob_livo/prompt5_i3_divergence_attribution.md`, SHA256
`63f7e02b17ac8d87d61977458dbf000fae71532d58fdc773ffd05fc22a953ac7`. The
structured report is `spec/prob_livo/PROMPT5_EVIDENCE.md`.

The renamed original legacy workspace `/home/lc/prob_lio/src/Super-LIO` was
not modified. A detached diagnostic worktree at
`/tmp/prob_lio_diag/legacy_621acbd` was checked out at
`621acbd8d9a67634d3782fe8ab56e8a49ec821a9` and rebuilt in clean diagnostic
build/devel directories. Its live eee_01 oracle exactly reproduced the
historical 3981 rows, 3329 matches, ATE `0.08883155405698266 m`, and trajectory
SHA `259d3fbc16e5b918a75d5517c4f5feac0b29e40b7c6d5464f881185704595199`.

The clean current Super-input run is
`results/prob_livo/runs/eee01_camera_off_p0_p5_migrated_clean/`: 3981 rows,
3329 matches, ATE `0.09099574805341126 m`, trajectory SHA
`d06e472b04f7d304d1462b30b2077766f62bf7047cd4247f909c96b3ca277f03`, and
3987/3986/3986/3986/0 callback/emitted/attempted/success/reject accounting.
It uses the repository-owned in-process offline runner with TBB maximum
parallelism 32; it does not invoke the old Super runtime.

The first mismatch is RUN index 0 (trajectory row 1), at filter timestamp
`1609059013.7636957` and epoch end `1609059013.7657526`. A selected-epoch
signature trace found matching scheduler/input and first-point observables
(3479 undistorted, 2433 downsampled), then a differing predicted state and
measurement system. The legacy initialization mean acceleration is
`0.84424549341201782 0.02315736748278141 -9.5785360336303711`; the migrated
double result is `0.84424540194971809 0.02315736831374595 -9.5785373819285429`.
This follows from legacy `scalar=float` versus the host double
`StatesGroup`/`ProbESKF19`. The float-initialization-only control moved the
first pose toward legacy but failed to reproduce it, providing the bounded
causal negative control.

The strict raw comparison remains 3981 pairs with timestamp delta RMSE/max
`0/0 s`, translation RMSE/median/max
`0.03319535524213128/0.03112573966899626/0.07739101605452314 m`, and rotation
RMSE/median/max
`0.00194296843409943/0.002017233514502395/0.009015083200112439 rad`.
The first-divergence classification is `NUMERIC_ONLY`; the final Prompt5
decision is `I3_DIVERGENCE_NUMERIC_ACCEPTED`. No precision downgrade, tuning,
OMP limit, P5 association, visual runtime, or permanent diagnostic logging
was added. I4 remains not started and I3 remains Owner-audit pending.

## Prompt 6 Evidence

Prompt 6 is registered at
`prompts/prob_livo/prompt6_i3_numeric_close_i4_pointwithvar_adapter.md`.
The bounded numeric fixture is
`tools/prob_livo/attribute_i3_init_numeric.py`; its exact 58-sample JSON
output was captured during the Prompt-6 run. I3 is closed as
`NUMERIC_IMPLEMENTATION_DIFFERENCE_CONFIRMED` / `CLOSED / OWNER VERIFIED`.
The I4 source audit and final gate report are recorded in
`spec/prob_livo/PROMPT6_EVIDENCE.md`. The production adapter is
`include/prob_livo/prob_point_with_var_adapter.h` plus
`src/prob_livo/prob_point_with_var_adapter.cpp`; its focused gates are
`tests/prob_livo/test_i4_point_with_var_adapter.cpp` and executable
`prob_livo_i4_tests`.
