# Prompt 8 — I6 Camera-ON FAST Visual Closure and Gate Ablation

## Agent State Consensus

- Prompt8 start HEAD: `c4612b55b6e336da029597b7295f2094f13276f6`.
- Branch: `prob-livo`; reference remains `/home/lc/super_livo/ref/Super-LIO`
  at `9fc949f46291c0fa76e5b7cdb372c940eb4b3f6e` and was not modified.
- Registered prompt:
  `prompts/prob_livo/prompt8_i6_camera_on_visual_gate_ablation.md`.
  SHA256: `cb2bfb52a4304a0c69b37dc9c997a7ea582793f9a610e8ed1d2948555beb65be`.
- The implementation frontier was clean at `fdddd2e` before the final
  evidence edits. Final clean state, commit, and push are recorded below.
- Online/offline parity was verified once with H1: both runners produced the
  same 3979-row trajectory, same SHA256, identical counters, and zero
  translation/field difference. Per the user decision, H0 and H2 were then
  run only through the repository-owned offline runner.

## Source Audit

FAST scheduler ownership remains in `LIVMapper::sync_packages()`
(`src/LIVMapper.cpp:1105-1305`). In LIVO mode it cuts a camera epoch, owns
IMU consumption through the image time, rebases current/next LiDAR points, and
then emits the LIO phase followed by the VIO phase. The offline reader
(`src/prob_livo/offline_reader.cpp`) dispatches `/imu/imu`,
`/os1_cloud_node1/points`, and `/left/image_raw` in rosbag record order and
invokes one scheduler step after every relevant message. The online runner
uses the same production callbacks and enables
`/common/prob_livo_one_callback_step` for the single parity check.

The production visual order in `VIOManager::processFrame()` is preserved:

```text
updateFrameState → resetGrid → retrieveFromVisualSparseMap
→ computeJacobianAndUpdateEKF → generateVisualMapPoints
→ updateVisualMapPoints → updateReferencePatch
```

The visual update retains FAST's exposure-aware residual, pyramid and
coarse-to-fine iteration, inverse-composition option, relinearization,
rollback, covariance/state update, and reference-patch ownership. Only the
I4 current-scan source, I5 arbitrary-plane source, and second visual
point-plane gate are adapted.

## Architecture Wiring

- `ProbLioBackend` owns the ProbESKF19 wrapper, P0–P4 buffers, the sole active
  Prob OctVox LiDAR geometry map, and `ProbPlaneProvider`.
- The backend operates directly on the host `StatesGroup`/x19/P19 reference.
  `LIVMapper::handleProbVio()` obtains the LIO posterior, passes the same
  state/covariance to FAST VIO, and then calls `FinalizeCameraEpoch()`.
- The I4 adapter supplies current `pointWithVar` values with preserved source
  order and IDs: `point_i` IMU/body, `point_b` LiDAR/body, `point_w` world,
  `body_var` canonical `Sigma_L`, `var_nostate` sensor-only world covariance,
  `var` FAST full visual covariance, and accepted current-scan normals.
- The I5 callback calls `ProbPlaneProvider::QueryAtWorldPoint()` on the same
  backend-owned OctVox/HKNN/QR map for raycast/reference-plane needs. It does
  not refit current-scan normals.
- FAST `feat_map` remains the one visual `VisualPoint` map. In the Prob path
  the legacy FAST LiDAR map is neither queried nor updated; all active LiDAR
  geometry comes from Prob OctVox. No pose-copy bridge or second filter is
  introduced.

## Visual Gates

### Radius Gate

The unchanged first gate is FAST's local-support condition:

```text
radial_distance <= 3 * plane_radius
```

It is evaluated identically in A and B before the second gate. The H1/H2
counter evidence shows the same radius gate remains active; only the second
gate mode is changed.

### A — LIVO2 Probabilistic 3σ

For `r = nᵀp_W + d`, the implementation uses the native Prob 4×4
`Sigma_nd` ordered `[n_x,n_y,n_z,d]` and the FAST `VisualPoint` covariance
`Sigma_VP`:

```text
J_nd          = [p_Wᵀ, 1]
sigma_plane²  = J_nd Sigma_nd J_ndᵀ
sigma_point²  = nᵀ Sigma_VP n
sigma_total²  = sigma_plane² + sigma_point²
accept        = abs(r) < 3 * sqrt(sigma_total²)
```

The inequality is strict. Geometry and uncertainty validity are tracked
separately. Invalid/non-PSD covariance rejects A; no fake 6×6 FAST plane
covariance or P5 result is synthesized.

### B — Super Legacy

The source-audited Super deterministic point-to-plane gate is:

```text
length > 81 * residual * residual
```

Here `residual = nᵀp_W + d`, while `length` is the norm of the sensor-
relative LiDAR point, not the world-origin norm. The implementation transforms
the world `VisualPoint` through the shared pose and the inverse IMU/LiDAR
extrinsic before taking that range. B does not use covariance, P4 weight, or
disable the common radius gate. Its strict inequality and sensor-frame range
are covered by the independent gate fixture.

## Gate Tests

- `G-I6.GA1`: PASS — inside, threshold, outside, plane-dominant,
  VisualPoint-dominant, invalid covariance, strict inequality, and negative
  covariance-field/fake-plane mutations.
- `G-I6.GB1`: PASS — exact `length > 81 * error²` oracle, strict boundary,
  covariance independence, sensor-relative transform, and negative world-norm,
  P4-weight, and radius-disabled mutations.
- `G-I6.G2`: PASS — one switch changes only the second visual decision; the
  common radius decision and all LiDAR/P0–P5 paths remain unchanged. H1/H2
  use identical effective visual parameters apart from the gate name and
  output/run identity.
- `G-I6.1`: PASS — one shared x19/P19 is used sequentially; LIO posterior,
  visual update, and next epoch are connected through the same host state.
- `G-I6.2`: PASS — existing production LIVO camera-cut/continuity tests
  cover current/next partition, IMU ownership, no point duplication, and
  single anchor advancement.
- `G-I6.3`: PASS — real H1/H2 epochs report candidates, valid normals, and
  VisualPoints; the I4 source-index path is used and no FAST LiDAR-map query
  occurs in the Prob visual callback.
- `G-I6.4`: PASS — arbitrary visual-plane queries route to I5 provider over
  the backend-owned OctVox; the runtime has no FAST LiDAR-plane query count.
- `G-I6.5`: PASS — geometry-only provider results and uncertainty validity are
  separate; A requires valid point and plane uncertainty, while B remains
  deterministic without covariance.
- `G-I6.6`: PASS — FAST exposure state and P19 cross-covariance remain in the
  visual update; LiDAR P0–P4 does not observe exposure directly.
- `G-I6.7`: PASS — source pyramid, relinearization, rollback and commit order
  are retained; real runs record photometric accepts and rollbacks.
- `G-I6.8`: PASS — reference-patch counters distinguish attempts and accepted
  updates, and A/B share the same radius/reference-patch path.
- `G-I6.9`: PASS — H1/H2 have nonzero visual calls, valid normals, provider
  queries, VisualPoints, patch attempts, photometric accepts, and state commits.

The focused executable result is:

```text
[PASS] G-I6 visual plane gate policy and sensor-range oracle checks=14
```

## Dataset / Effective Config

All runs use:

```text
bag: /home/lc/super_livo/bag/NTU/eee_01/eee_01.bag
bag SHA256: 7ea43946cffdd49c88d993ad3f192a4e90a8f6826eddc2ef1a9d4f5343ca6c17
input: super_ntu_legacy
topics: /imu/imu, /os1_cloud_node1/points, /left/image_raw
overlay: config/prob_livo/NTU_eee01_super_legacy.yaml
overlay SHA256: a1d775f552a3d13dd6750a6099a8ea80503e2510d08052d38c2e92c0218e2587
camera config: config/camera_NTU_VIRAL.yaml
downsample: Super VoxelGridClosest, voxel size 0.5
LiDAR P5: OFF
evaluator: NTU VIRAL dataset-author evaluator
```

The offline runner uses TBB `max_allowed_parallelism=32`; it does not impose
an OpenMP cap of four. The user-requested offline extension is the same
FAST-LIVO2 reader/callback/scheduler/backend path with image dispatch and H0,
H1, and H2 controls, not `super_ntu_legacy` runtime reuse.

## H0

```text
run: results/prob_livo/runs/prompt8_i6_h0_camera_scheduler_offline_final
classification: VISUAL_INACTIVE_CONTROL_VALID
completion: PASS (node/counters/GT/evaluator all RC 0)
rows/matched GT: 3979 / 3326
ATE: 0.09138258970792523 m
trajectory SHA256: 9f9eae6fe119d23260d23c4c10a0fba900ba32798d0a58239861432a11d3c53f
runtime: 52 s
camera_epochs/images_received: 3985 / 3985
visual_process_calls: 0
current_scan_candidates/normal_valid: 0 / 0
plane_queries/geometry_valid/uncertainty_valid: 0 / 0 / 0
radius pass/reject: 0 / 0
second gate pass/reject: 0 / 0
VisualPoints/reference attempts/accepted: 0 / 0 / 0
photometric attempts/accepted: 0 / 0
visual_state_commits: 0
visual_rollbacks: 0
```

H0 has camera scheduler and image flow ON but visual state update OFF.

## H1

```text
run: results/prob_livo/runs/prompt8_i6_h1_prob3sigma_offline_deterministic1
classification: VISUAL_ACTIVE_VALID
completion: PASS (node/counters/GT/evaluator all RC 0)
rows/matched GT: 3979 / 3326
ATE: 0.08862454637627792 m
trajectory SHA256: 315c8b0ec74e409bb1a22ac08de6725d1ee042c563e546ec1591b52ef80825aa
runtime: 89 s offline
camera_epochs/images_received/visual_calls: 3985 / 3985 / 3979
current_scan_candidates/normal_valid: 2173 / 1613
plane_queries/geometry_valid/uncertainty_valid: 45155 / 36991 / 36991
radius pass/reject: 36991 / 0
second gate pass/reject: 36909 / 82
VisualPoints/reference attempts/accepted: 222604 / 45155 / 36909
photometric attempts/accepted/commits/rollbacks: 3977 / 3977 / 3977 / 5274
```

The online one-callback parity run used the same source and configuration:
`results/prob_livo/runs/prompt8_i6_h1_prob3sigma_online_one_callback_deterministic`.
It also produced 3979 rows, ATE `0.08862454637627792 m`, SHA256
`315c8b0ec74e409bb1a22ac08de6725d1ee042c563e546ec1591b52ef80825aa`, and
identical visual counters. Strict comparison pairs all 3979 rows with zero
timestamp/translation/field difference; the quaternion self-angle diagnostic
is only `2.98e-08 rad` from `acos` roundoff. This is the one online/offline
verification; later H0/H2 evidence is offline-only as requested.

## H2

```text
run: results/prob_livo/runs/prompt8_i6_h2_superlegacy_offline_deterministic1
classification: VISUAL_ACTIVE_VALID
completion: PASS (node/counters/GT/evaluator all RC 0)
rows/matched GT: 3979 / 3326
ATE: 0.08793104514326024 m
trajectory SHA256: 812d1bb9de1abeba0471632fa7515ee7afff682cbcad87bbab2786b109f5f6ca
runtime: 84 s
camera_epochs/images_received/visual_calls: 3985 / 3985 / 3979
current_scan_candidates/normal_valid: 2173 / 1618
plane_queries/geometry_valid/uncertainty_valid: 44856 / 36820 / 36820
radius pass/reject: 36817 / 3
second gate pass/reject: 36817 / 3
VisualPoints/reference attempts/accepted: 222590 / 44856 / 36817
photometric attempts/accepted/commits/rollbacks: 3977 / 3977 / 3977 / 5363
```

## Ablation Table

| variant | ATE (m) | rows | matched GT | camera epochs | visual calls | plane valid | radius pass | second-gate pass | VisualPoints | reference updates accepted | photometric accepted | visual commits | rollbacks | runtime |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| H0 camera scheduler, visual OFF | 0.091382590 | 3979 | 3326 | 3985 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 52 s |
| H1 A: LIVO2 Prob 3σ | 0.088624546 | 3979 | 3326 | 3985 | 3979 | 36991 | 36991 | 36909 | 222604 | 36909 | 3977 | 3977 | 5274 | 89 s |
| H2 B: Super legacy | 0.087931045 | 3979 | 3326 | 3985 | 3979 | 36820 | 36817 | 36817 | 222590 | 36817 | 3977 | 3977 | 5363 | 84 s |

The H1/H2 difference is the intended visual second-gate ablation. The small
later-epoch counter and trajectory differences are the expected consequence
of the two accepted/rejected visual decisions feeding the same sequential
state; no other parameter was changed.

## G-I6.10–G-I6.16

- `G-I6.10 H0`: PASS — complete offline run, finite trajectory, 3979 rows,
  3326 matches, and `visual_state_commits=0`.
- `G-I6.11 H1`: PASS — complete A-mode run, active visual counters, finite
  trajectory, official evaluation, and fixed config.
- `G-I6.12 H2`: PASS — complete B-mode run with equivalent evidence.
- `G-I6.13 ablation controls`: PASS — same bag/hash, source HEAD, scheduler,
  input semantics, downsample, camera config, P0–P4 and radius gate; only
  `prob_livo/visual_plane_gate` differs between offline H1/H2.
- `G-I6.14 downsample`: PASS — all variants use only copied Super
  `VoxelGridClosest`; no FAST/PCL centroid comparison or hidden mode change.
- `G-I6.15 P5 exclusion`: PASS — LiDAR association remains Super legacy and
  P4 probabilistic weighting remains enabled; LiDAR P5 is not wired. The A
  gate is visual/reference-patch-only.
- `G-I6.16 shared-state ABI`: PASS — visual uses the host x19/P19 layout,
  including the exposure index and physical covariance blocks; no second
  state or covariance matrix is introduced.

## Scope Audit

- Super VoxelGridClosest is frozen and shared by H0/H1/H2; downsample ablation
  is cancelled.
- LiDAR P5 remains OFF. The visual A 3σ decision is not used by LiDAR
  association or P4 weighting.
- I4 pointWithVar and I5 ProbPlaneProvider are the only Prob geometry seams;
  current normals are not recomputed through the provider.
- No second active LiDAR geometry map, second filter, FAST LiDAR lookup, PCL
  KD-tree, alternate plane estimator, fallback covariance, tuning sweep, or
  threshold sweep was added.
- The project-owned offline runner is now the default regression path after
  the single H1 online/offline parity verification. H0/H2 and repeated H1
  determinism checks used offline record-order replay.
- I8 has not started. No unrelated editor/IDE diagnosis is part of this
  report.

## Build/Test

Final build:

```bash
cmake --build /home/lc/super_livo/build --target all -- -j4
```

Return code: `0`. Focused executables all returned `0`:

```text
prob_livo_i1_tests
prob_livo_i2_tests
prob_livo_i3_tests
prob_livo_p4_tests
prob_livo_i4_tests
prob_livo_i5_tests
prob_livo_i6_visual_gate_tests
```

The focused counts include I1 through I5 prior gates and
`G-I6 visual plane gate policy and sensor-range oracle checks=14`.

## Files / Commits

Prompt8 implementation and evidence files include:

```text
include/prob_livo/visual_plane_gate.h
src/prob_livo/visual_plane_gate.cpp
tests/prob_livo/test_i6_visual_plane_gate.cpp
include/vio.h
src/vio.cpp
include/visual_point.h
src/visual_point.cpp
include/LIVMapper.h
src/LIVMapper.cpp
include/prob_livo/offline_reader.h
src/prob_livo/offline_reader.cpp
include/prob_livo/prob_lio_backend.h
src/prob_livo/prob_lio_backend.cpp
tools/prob_livo/prob_livo_offline.cpp
tools/prob_livo/run_eee01_camera_off.sh
tools/prob_livo/run_eee01_camera_offline.sh
CMakeLists.txt
prompts/prob_livo/prompt8_i6_camera_on_visual_gate_ablation.md
spec/prob_livo/PROMPT8_EVIDENCE.md
spec/prob_livo/SPEC.md
spec/prob_livo/HISTORY.md
spec/prob_livo/EVIDENCE_INDEX.md
```

Implementation commits:

```text
4b45a5f feat(prob-livo): wire camera-on offline visual gates
fdfb627 test(prob-livo): parameterize online camera ablations
469139a fix(prob-livo): include ROS callback queue definition
a354513 fix(prob-livo): commit deterministic callback replay switch
fdddd2e fix(prob-livo): make visual convergence reduction deterministic
```

The final evidence/docs commit and final HEAD are recorded at handoff; the
worktree is clean and `origin/prob-livo` is fast-forwarded to that HEAD.

## Final State

```text
I0–I5 = CLOSED / OWNER VERIFIED
I6 = CLOSED/PASS — Owner audit pending
I7 visual-gate ablation = completed inside I6
I7 downsample ablation  = CANCELLED
I8 = NOT STARTED

camera-on FAST visual integration = operational
visual plane-gate ablation        = complete
online/offline runner parity      = verified once on H1, then offline-only
Super VoxelGridClosest             = frozen canonical downsample
```
