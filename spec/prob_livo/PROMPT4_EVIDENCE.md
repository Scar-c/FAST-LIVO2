# Prompt 4 Evidence — Super-input parity on eee_01

## Agent State Consensus

- Start frontier: `df83f635134a1b1915568dcfa491eaa7d37dc22a`.
- User-requested action: implement and use a repository-owned in-process
  offline production runner first, with the current FAST-LIVO2/LIVO2-native
  interfaces; later user direction fixed offline acceleration to TBB32.
- Current implementation frontier: `3eec40cf89d8359bc80cce81979b3d73b6553a67`.
- Final online verification was run from clean registered-prompt HEAD `210e2ead7829dab3e9f8eeed2223f0ababc07db9`.
- Branch: `prob-livo`; reference: `prob-lio` at `9fc949f46291c0fa76e5b7cdb372c940eb4b3f6e`.
- Exact Prompt 4 is registered at `prompts/prob_livo/prompt4_super_input_parity_eee01.md`.
- Prompt 4 source/registration SHA256: `78903883fe3ebaefcbfdc8dcc15b46e534253781e34735110bc00aba9e53b6c0`.
- Host and reference worktrees were clean during the final runs; the reference was not modified.

## Why Prompt 4 Exists

`0.05290159739482509 m` is historical FAST-native-input evidence from the
Prompt 3 canonical runner. It is not a valid Super-input migration-parity
comparison. It is preserved separately and is not claimed to be the current
online/offline result.

## Legacy vs FAST Source Audit

| semantic | legacy Super | FAST-native |
|---|---|---|
| LiDAR timestamp origin | PointCloud2 header | PointCloud2 header |
| `lidar_time_offset` | `0.0 s` | NTU config `-0.1 s` |
| point relative time | Ouster `t` ns to curvature ms | Ouster `t` ns to curvature ms |
| point ordering | source order | curvature/time sort |
| blind | strict `range > 2.0 m` | existing FAST `range >= 1.0 m` behavior |
| max range | strict `range < 150.0 m` | no equivalent max-range cut |
| stride/filter rate | source indices `0,3,6,...` | native `point_filter_num=3` |
| invalid points | non-finite XYZ rejected | native handler has different finite/range behavior |
| scan end | last accepted source-ordered point | last native-ordered point |

The compatibility mode is implemented by
`Preprocess::super_ntu_legacy_oust64_handler`; it is not an external
`super_ntu_legacy` runtime.

## Implementation

- Added explicit `fast_native` and `super_ntu_legacy` modes.
- Added `config/prob_livo/NTU_eee01_super_legacy.yaml`.
- Added the in-process runner `tools/prob_livo/prob_livo_offline.cpp`, which
  feeds the current FAST-LIVO2 `LIVMapper`/scheduler/backend interfaces.
- Added TBB hot-loop parallelism with scoped offline
  `max_allowed_parallelism=32`; OpenMP is no longer capped at four.
- Added Super first-epoch pre-LiDAR IMU retention and map-init observation-time
  advancement.
- Replaced the Prob backend `std::vector<bool>` effect mask with
  `std::vector<uint8_t>`.
- Isolated Super post-endpoint/early-point undistortion behavior from
  `fast_native`, preserving the native endpoint contract.

The camera-OFF Prob-LIO path returns before FAST `processImu()`; its hot loops
use TBB. No duplicate filter, frontend, LiDAR map, camera path, or P5 path was
added.

## G-P4.1

PASS. `prob_livo_p4_tests` reports 201 checks and verifies the real
compatibility helper: source identities `3,9,18,24`, source order,
curvatures `9,1,7,2 ms`, strict blind/max-range cuts, finite handling, and
stride 3. Negative mutations distinguish the native path, missing 150 m cut,
1 m blind, and off-by-one stride.

## G-P4.2

PASS. The same synthetic PointCloud2 header produces the same legacy scan time
and point acquisition times. The overlay sets `lidar_time_offset=0.0`; the
`-0.1 s` mutation fails the focused test.

## G-P4.3

PASS. Final Super-input counters close as:

```text
LiDAR callbacks                 3987
  scheduler emitted             3986
    backend attempted           3986
      backend success            3986
      backend rejected               0
  pending at shutdown               1
  scheduler discarded              0

successful lifecycle epochs     3986 = IMU_INIT 1 + MAP_INIT 4 + RUN 3981
trajectory rows                 3981
```

The one pending callback is the final LiDAR record with no later IMU endpoint.
It is explicitly counted and is not a rejected epoch. There is no unexplained
remainder.

## G-P4.4

PASS. The timing fixture reports legacy `first_run_time=0.545` and observation
boundary `0.55`: Super does not consume the first IMU after the endpoint but
does advance the map-init observation boundary. The fixture also verifies that
the Super exception does not leak into native mode.

## Super-Input Effective Config

```text
base:             config/NTU_VIRAL.yaml
overlay:          config/prob_livo/NTU_eee01_super_legacy.yaml
base SHA256:      c8f94f130e599b928c3f02c3f3d3b2009ae01df76aec32f6ac96b6a987311ef3
overlay SHA256:   a1d775f552a3d13dd6750a6099a8ea80503e2510d08052d38c2e92c0218e2587
input mode:       super_ntu_legacy
lidar offset:     0.0 s
blind/maxrange:   2.0 m / 150.0 m, strict
filter rate:      3
camera:           OFF
backend:          ProbLioBackend P0-P4
P5:               disabled
offline TBB:      32
```

## Canonical eee_01 Run

```text
run dir:          results/prob_livo/runs/eee01_camera_off_p0_p4_offline_super_input_tbb32_final
runner:           tools/prob_livo/run_eee01_camera_offline_super_input.sh
run HEAD:         3eec40cf89d8359bc80cce81979b3d73b6553a67
bag SHA256:       7ea43946cffdd49c88d993ad3f192a4e90a8f6826eddc2ef1a9d4f5343ca6c17
completion:       run/node/GT/evaluator RC = 0/0/0/0
trajectory rows:  3981
matched GT:       3329
runtime:          57 s wall-clock, TBB32
trajectory SHA:   d06e472b04f7d304d1462b30b2077766f62bf7047cd4247f909c96b3ca277f03
ATE:              0.090995748 m
```

## Legacy Reference

```text
workspace:        /home/lc/prob_lio/src/Super-LIO
algorithm SHA:    621acbd8d9a67634d3782fe8ab56e8a49ec821a9
trajectory SHA:   259d3fbc16e5b918a75d5517c4f5feac0b29e40b7c6d5464f881185704595199
rows/matched:     3981 / 3329
ATE:              0.08883155405698266 m
```

## Strict Trajectory Comparison

Primary comparator:
`tools/prob_livo/compare_trajectories_strict.py`; monotonic near-exact
timestamp pairing, no interpolation, no mutual SE(3) alignment.

```text
paired rows:                3981
unmatched new/old:          0 / 0
timestamp dt RMSE/max:     0.0 / 0.0 s
translation RMSE/med/max:   0.03319535524213125 / 0.03112573966899626 / 0.07739101605452314 m
rotation RMSE/med/max:      0.0019429684341007508 / 0.002017233514502395 / 0.009015083200112437 rad
```

Timestamps, lifecycle, and accounting are exact. The remaining residual is
the identified FAST-host/backend numerical seam; map-update inserts are
`13787674` versus the old host's `13787631`.

## ATE Comparison

```text
Legacy Super-host Prob-P4:  0.08883155405698266 m
FAST-host + Super-input:    0.090995748 m
absolute delta:             +0.0021641939430173396 m
percent delta:              +2.436289633781569 %
matched GT old/new:         3329 / 3329
rows old/new:               3981 / 3981
```

## Three-Way Result Table

| result | input semantics | rows | matched | ATE |
|---|---|---:|---:|---:|
| A legacy Super-host | Super source order, strict 2--150 m, offset 0 | 3981 | 3329 | `0.08883155405698266 m` |
| B FAST-host + Super input | `super_ntu_legacy`, audited compatibility mode | 3981 | 3329 | `0.090995748 m` |
| C FAST-host + FAST-native input | historical canonical, offset -0.1, native sort/no max cut | 3595 | 3016 | `0.05290159739482509 m` |

For the online/offline reliability control, current `fast_native` was run
again after the mode fix:

```text
online run:       results/prob_livo/runs/eee01_camera_off_p0_p4_online_native_final
offline run:      results/prob_livo/runs/eee01_camera_off_p0_p4_offline_native_tbb32_fixed
current ATE:      0.054502750 m in both
rows/matched:     3980 / 3327 in both
trajectory SHA:   7149297f46df10ce895fe564dc689b05b3356e6b7c58c03ff92bffd761b93410
strict:           translation max/rmse 0.0 / 0.0 m; rotation max 4.214684851e-08 rad
```

Thus the historical `0.052901597` value is not the current online result,
but current online and offline are exactly consistent.

## Classification

`SUPER_INPUT_TRAJECTORY_NEAR_PARITY`

This is exactly one classification. Input semantics, timestamps, lifecycle,
map-init timing, and accounting are closed; the small but material trajectory
residual is attributed to the identified FAST-host/backend numerical seam,
not an unexplained input mismatch.

## G-P4.5–G-P4.7

- G-P4.5: PASS — complete Super-input eee_01 run, camera OFF, same evaluator,
  all runtime return codes zero.
- G-P4.6: PASS — strict comparison, ATE comparison, and one classification;
  residual seam identified.
- G-P4.7: PASS — no tuning, sweep, visual processing, P5, duplicate mapper, or
  parameter contamination; offline uses TBB32.

## Scope Audit

I4 was not started. No visual/VIO runtime, camera extension, or P5 path was
used. The historical FAST-native result remains separate. No bag, generated
run output, or large log is tracked. The offline runner uses current
FAST-LIVO2 interfaces and does not invoke the old Super runtime.

## Files / Commits

```text
cdba474  feat(prob-livo): add in-process offline production runner
eb3340c  fix(prob-livo): align legacy Super startup timing
478b9e7  perf(prob-livo): use TBB for offline hot loops
15d2c9c  fix(prob-livo): retain pre-lidar IMU for Super init
3eec40c  fix(prob-livo): isolate Super endpoint handling from native
210e2ea  docs(prob-livo): register Prompt4 parity specification
```

Final build and focused tests passed: `prob_livo_i2_tests` G-I2.1--G-I2.9,
`prob_livo_p4_tests` 201 checks, backend/offline/fastlivo_mapping build, and
`git diff --check`.

## Final State

```text
I0 = CLOSED / OWNER VERIFIED
I1 = CLOSED / OWNER VERIFIED
I2 = CLOSED / OWNER VERIFIED
I3 = CLOSED/PASS — Owner audit pending
I4–I8 = NOT STARTED

Legacy Super-host Prob-P4 baseline      = recorded
FAST-host + Super-input parity baseline = recorded as SUPER_INPUT_TRAJECTORY_NEAR_PARITY
FAST-host + FAST-native-input result    = preserved separately
```
