## Agent State Consensus

- Prompt7 start HEAD: `8464b33d9c936dfa2ed559e2b2f26962ac614439`.
- Branch: `prob-livo`; expected `origin/prob-livo` was the same start SHA.
- Prompt7 final HEAD is the final evidence/docs commit recorded below; the
  worktree and `origin/prob-livo` are clean after the fast-forward push.
- Prompt registration:
  `prompts/prob_livo/prompt7_i4_corrective_i5_plane_provider.md`.
  SHA256: `4d767b929e2d120f8082fdc093dc748398c819eb4cad8a36449d02d378430eeb`.
- Reference implementation remained `/home/lc/super_livo/ref/Super-LIO` at
  `9fc949f46291c0fa76e5b7cdb372c940eb4b3f6e`; it was not modified.
- G-P7.0: PASS. The clean-start full host build and I1/I2/I3/P4/I4 tests
  returned zero before modification.

# PART A — I4 Corrective

## FAST pointWithVar Source Contract

| field | exact FAST meaning | frame | source producer | consumer |
|---|---|---|---|---|
| `point_i` | current undistorted point in the IMU body frame | IMU/body | Prob scan-end undistortion | future visual point path |
| `point_b` | corresponding LiDAR-body point | LiDAR/body | inverse known extrinsic in adapter | FAST point compatibility |
| `point_w` | current point under the shared current pose | world | adapter from `R_WI p_i + t_WI` | visual projection/map candidate |
| `body_var` | LiDAR sensor-point covariance | LiDAR/body | canonical P1 `Sigma_L` sidecar | FAST body covariance contract |
| `var_nostate` | sensor-only covariance in world | world | `R_WI Sigma_I R_WI^T` | visual covariance diagnostics |
| `var` | full FAST visual point covariance | world | sensor term plus FAST pose propagation | `generateVisualMapPoints` |
| `point_crossmat` | skew matrix of `point_i` | IMU/body point | adapter | pose covariance propagation |
| `normal` | accepted current Super QR map-plane normal | world | current LIO `plane_coefficients_` | FAST normal skip/candidate path |

The authoritative FAST declaration is `include/common_lib.h:106-127`. The
native producer in `src/voxel_map.cpp:338-453` computes sensor covariance in
LiDAR coordinates before applying the extrinsic to the IMU point, uses the
IMU point for `point_crossmat`, and builds the visual world covariance with
rotation and translation state covariance. `src/vio.cpp:804-890` confirms that
the visual candidate path skips only a zero normal and later stores `var` as
the visual point covariance.

## Corrections

- `body_var`: now receives canonical P1 `Sigma_L`; `body_covariances_` remains
  the separately carried `Sigma_I = R_LI Sigma_L R_LI^T` for the IMU point.
- `var_nostate`: now rotates `Sigma_I` into world and no longer derives from
  the LiDAR-frame `body_var` field.
- `var`: now uses FAST's source equation
  `R_WI Sigma_I R_WI^T + (-[p_i]x)P_rr(-[p_i]x)^T + P_tt`, with host x19
  physical blocks `Layout::kRot0` and `Layout::kPos0`; inverse exposure is
  not used and no P5 term is present.
- `normal`: now copies the accepted current-scan QR coefficient head at the
  same source index. Rejected or invalid associations keep the zero sentinel;
  `normal_valid` is a per-point sidecar capability bit.
- `point_crossmat`: unchanged semantically; it remains `skew(point_i)`.

## G-I4.C1

PASS. The focused fixture uses nonidentity `R_LI` and independently supplies
`Sigma_L` and `Sigma_I`. It proves `body_var == Sigma_L` and
`Sigma_I == R_LI Sigma_L R_LI^T` as well as inverse-extrinsic point identity.
Negative checks distinguish `body_var = Sigma_I`, wrong covariance rotation,
double rotation, and double extrinsic application.

## G-I4.C2

PASS. The independent test oracle computes the source-faithful sensor,
rotation, and translation terms from a full host `Matrix19`. It covers zero
pose covariance, nonzero rotation/translation covariance, nonzero
rotation-position cross storage, and an exposure covariance sentinel. The
adapter matches the oracle; `var_nostate` remains sensor-only. Negative checks
detect `var = var_nostate`, wrong physical indices, and exposure contamination.
Pose covariance is used only for visual point uncertainty, never for Prob-LIO
P4 association or weighting.

## G-I4.C3

PASS. Accepted and rejected points in the same deterministic observation
fixture retain source order and identity. The accepted `normal` is the exact
world QR coefficient already owned by the LIO observation; the rejected point
is zero. The adapter does not re-query OctVox, run a second QR, or fit a FAST
plane.

## G-I4.C4

PASS. The previous member-function pointer check was replaced by the
production-used `PrepareVisualMapCandidate()` seam in `src/vio.cpp`. The test
passes an adapted accepted point with nonzero normal and valid `point_w/var`
and observes that it reaches the candidate path; the adapted rejected point
is skipped exactly by FAST's native zero-normal condition. The seam copies and
returns the actual visual `point_w`, `var`, and `normal` values. Camera/VIO
runtime was not enabled.

## G-I4.C5

PASS. The clean camera-OFF in-process `super_ntu_legacy` run was compared with
the pre-corrective baseline `results/prob_livo/runs/eee01_camera_off_p0_p5_migrated_clean`:

```text
run: results/prob_livo/runs/prompt7_i4_i5_camera_off_super_clean_host2
rows: 3981
ATE: 0.09099574805341126 m
matched GT: 3329
trajectory SHA256: d06e472b04f7d304d1462b30b2077766f62bf7047cd4247f909c96b3ca277f03
callbacks/emitted/attempted/success/reject: 3987/3986/3986/3986/0
IMU_INIT/MAP_INIT/RUN: 1/4/3981
adapted_scans/adapted_points: 3981/13787674
TBB maximum parallelism: 32
runtime: 57 s
return code: 0
```

The baseline has the same rows, trajectory SHA, ATE, lifecycle counts,
downsampled/undistorted points, HKNN/QR/weight counters, and scheduler/backend
counters. Only the new adapter counters are additional. I4 therefore remains
observational to ProbESKF19, OctVox, association, P4, and trajectory output.

## I4 Decision

```text
I4 = CLOSED / OWNER VERIFIED
```

# PART B — I5

## FAST Visual Plane Need Audit

| consumer | query/input | normal | d | center | radius | covariance | frame |
|---|---|---|---|---|---|---|---|
| `generateVisualMapPoints` | current `pointWithVar` / `point_w` | required for zero-normal skip and VisualPoint initialization | not direct | not direct | not direct | `pointWithVar.var` stored | world input |
| `retrieveFromVisualSparseMap` raycast branch | sampled world point from camera ray | old plane entry supplies it | not directly | plane center creates visual candidate | not used | not used | world |
| `updateReferencePatch` | existing visual point world position | plane normal and previous-normal sign | 3σ residual | 3σ radial term | `3 * radius` radial gate | native FAST 6×6 plane covariance plus point covariance | world |
| `getWarpMatrixAffineHomography` | reference point and normal transformed to reference/camera use | required | implicit through normal/point | not direct | not direct | not used | caller transforms provider world result |

The provider consequently exposes `normal_W`, `d`, `center_W`, `radius`, and
native `plane_cov_nd`; it leaves all frame transforms to the caller and does
not expose FAST's old `VoxelMapManager` objects.

## Provider API

Exact interface:

```cpp
bool ProbPlaneProvider::QueryAtWorldPoint(
    const Eigen::Vector3d &point_W,
    ProbPlaneQueryResult &result,
    std::string &error) const;
```

`ProbLioBackend` owns the sole `ProbPlaneMap` and exposes only a const
`plane_provider()` reference. `ProbPlaneProvider` stores a non-owning const
reference pointer to that same map; it owns no map, cache, filter, state, or
plane estimator. A query converts the explicit world point to the same float
query representation used by the backend, calls authoritative `OctVoxMap::getTopK`,
then uses `SolvePlaneFitQr` and `ComputeProbQrPlane`.

The result contains:

```text
valid, normal_W, d, coeff_nd, center_W, radius, support_count,
support_ids, support_points_W, support_covariances_W,
plane_cov_nd ([n_x,n_y,n_z,d]), qr_rank, qr_condition
```

Validity requires at least four supports, finite/full-rank Super QR, the
canonical legacy geometric support acceptance, and valid canonical P3
covariance. It does not apply the current-scan residual gate to an arbitrary
query point; plane existence and query-point consistency remain separate.
The QR normal sign is preserved. A future view-facing visual consumer may
flip its local copy without mutating the canonical provider result.

## Plane Center

PASS (`G-I5.1`). For the exact HKNN support used by QR, the provider computes:

```text
mu = mean(support points)
center_W = mu - (normal_W^T mu + d) normal_W
```

The deterministic tilted/noisy support fixture proves the center lies on the
plane, `center_W - mu` is parallel to the normal, and rejects raw-centroid,
query-point, and wrong-offset mutations.

## Radius

PASS (`G-I5.2`). Radius is required by FAST's `updateReferencePatch` radial
3σ gate. The provider uses the exact source-grounded support spread from
`VoxelMapManager::init_plane`: `sqrt(max_eigenvalue(C))`, where
`C = mean((p_i - mu)(p_i - mu)^T)` over the same HKNN support. No constant,
nearest point, voxel center, or unrelated support is used.

## Plane Covariance

PASS (`G-I5.3`). It is exposed only as the native canonical Prob QR 4×4
covariance `plane_cov_nd`, ordered `[n_x,n_y,n_z,d]`. The provider does not
fabricate or convert FAST's historical 6×6 `[normal,center]` covariance. The
focused fixture matches the direct `ComputeProbQrPlane` covariance and checks
the scalar query residual variance against canonical `PlaneResidualVariance`.

## Gates

- `G-I5.1 plane center`: PASS — projected support-centroid formula and
  negative mutations.
- `G-I5.2 radius/spread`: PASS — same-support FAST eigen-spread formula.
- `G-I5.3 P3 covariance`: PASS — native 4×4 ordering, exact covariance, and
  query residual variance parity.
- `G-I5.4 HKNN identity parity`: PASS — exact support count, order, points,
  covariances, and `OctVoxSupportId` voxel/local identities match direct
  canonical map query.
- `G-I5.5 QR plane parity`: PASS — rank, validity, `[n,d]`, and covariance
  match direct canonical QR; insufficient/rank-deficient supports reject.
- `G-I5.6 production end-to-end query`: PASS — populated backend fixture
  exercises map insertion, backend-owned provider, HKNN, QR, center, radius,
  and covariance through the production module.
- `G-I5.7 read-only / no LIO side effects`: PASS — repeated populated backend
  queries leave filter state, covariance, LIO counters, support points, and
  support identities unchanged.
- `G-I5.8 integration ownership seam`: PASS — backend constructs the provider
  over its own map and exposes only the provider interface; no second map is
  instantiated and no FAST map pointer is exposed.

## Performance Sanity

The bounded focused test performs 1000 repeated representative queries through
OctVox/HKNN/QR and records `2.760623 ms` on this host. The path is
bounded by the production HKNN search, not an O(N) scan over map points. No
cache or elaborate benchmark infrastructure was added.

## Build/Test

The final required host build:

```bash
cmake --build /home/lc/super_livo/build --target all -- -j4
```

Return code: `0`. Focused commands all returned `0`:

```bash
/home/lc/super_livo/devel/lib/fast_livo/prob_livo_i1_tests
/home/lc/super_livo/devel/lib/fast_livo/prob_livo_i2_tests
/home/lc/super_livo/devel/lib/fast_livo/prob_livo_i3_tests
/home/lc/super_livo/devel/lib/fast_livo/prob_livo_p4_tests
/home/lc/super_livo/devel/lib/fast_livo/prob_livo_i4_tests
/home/lc/super_livo/devel/lib/fast_livo/prob_livo_i5_tests
```

The final focused I4 output is `[PASS] G-I4 pointWithVar corrective semantic
gates checks=57`; the final I5 output is `[PASS] G-I5 ProbPlaneProvider
parity and read-only gates checks=111`.

## Scope Audit

- Camera/VIO runtime remained OFF; no camera subscription or `processFrame`
  invocation was added.
- No `VisualPoint` creation/update, reference-patch update, or photometric
  update was enabled.
- P5 association remains excluded; pose covariance is used only in the FAST
  visual point `var` field.
- Prob OctVox remains the sole LiDAR geometry authority; no second LiDAR map,
  FAST `VoxelMapManager` query, PCL KD-tree, alternate PCA plane, or duplicated
  neighbor table was added.
- The provider is read-only, uncached, and has no fallback geometry or
  per-point production logging. The only repeated computation is the exact
  source-required support spread for the returned radius.
- The user-requested offline verification was performed with the repository's
  in-process runner, `super_ntu_legacy` semantics, camera OFF, and TBB32.

## Files / Commits

Prompt7 files:

```text
prompts/prob_livo/prompt7_i4_corrective_i5_plane_provider.md
include/prob_livo/prob_point_with_var_adapter.h
src/prob_livo/prob_point_with_var_adapter.cpp
include/prob_livo/prob_lio_backend.h
src/prob_livo/prob_lio_backend.cpp
include/prob_livo/super_native/prob_geometry_p0_p4.h
include/vio.h
src/vio.cpp
tests/prob_livo/test_i4_point_with_var_adapter.cpp
include/prob_livo/super_native/OctVoxMap/OctVoxMap.hpp
include/prob_livo/prob_plane_provider.h
src/prob_livo/prob_plane_provider.cpp
tests/prob_livo/test_i5_plane_provider.cpp
CMakeLists.txt
spec/prob_livo/PROMPT7_EVIDENCE.md
spec/prob_livo/SPEC.md
spec/prob_livo/HISTORY.md
spec/prob_livo/EVIDENCE_INDEX.md
```

Commits:

```text
557a9be fix(prob-livo): close pointWithVar visual semantics
74ab89e feat(prob-livo): add OctVox-backed plane provider
final evidence/docs commit: recorded as final HEAD at handoff
```

## Final State

```text
I0 = CLOSED / OWNER VERIFIED
I1 = CLOSED / OWNER VERIFIED
I2 = CLOSED / OWNER VERIFIED
I3 = CLOSED / OWNER VERIFIED
I4 = CLOSED / OWNER VERIFIED
I5 = CLOSED/PASS — Owner audit pending
I6–I8 = NOT STARTED

current-scan pointWithVar semantics = FAST-compatible
ProbPlaneProvider                   = operational
camera/VIO runtime                  = OFF

Next stage = I6 camera-ON / FAST visual sequential closure
```
