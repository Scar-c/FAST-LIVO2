# Prob-LIVO Integration Specification

Status: Prompt 3 / I3 camera-OFF Prob-LIO P0–P4 baseline; I0, I1, I2, and I3
are closed and owner-verified. The EEE01 raw-trajectory comparison is
`I3_TRAJECTORY_CLOSE_NONIDENTICAL`.

This is the single current-truth authority for the FAST-LIVO2-hosted Prob-LIVO
integration. Historical notes and future prompts must not redefine these
interfaces or ownership decisions without an explicit Owner decision.

## 1. Objective and frozen architecture

Replace FAST-LIVO2's LIO authority with the canonical Prob-LIO P0–P4 backend
while preserving FAST-LIVO2's public-source scheduler and visual semantics.

```text
HOST authority   = FAST-LIVO2 public source
LIO authority    = canonical Prob-LIO P0–P4
Visual authority = FAST-LIVO2 public source
shared state     = one x19/P19
LiDAR map        = Prob OctVox only
visual map       = FAST-LIVO2 feat_map
P5               = excluded
```

FAST-LIVO2 is the host because it minimizes total modification. The visual
estimator is not to be reimplemented inside Super-LIO. The first integration
target is the FAST-LIVO2 shell with the Prob-LIO backend and camera disabled;
camera/VIO is enabled only after that baseline closes.

Canonical LiDAR semantics are:

```text
Super native downsample
Super compact OctVox
Super HKNN
Super QR plane estimator
P1 LiDAR point covariance
P2 probabilistic compact-map covariance
P3 QR-plane covariance
P4 probabilistic P2P soft weighting
Super legacy association gate
Super ESKF / IMU semantics
```

P5 probabilistic association is present in the read-only oracle only as an
experimental historical path. It is not canonical and must not enter this
integration. The final P4 variance is fixed as:

```text
R_i = 0.001 + sigma^2_QR-plane,i + sigma^2_sensor-point,i
w_i = 1 / R_i
```

Current pose covariance is not an input to final P4 `R_i`.

## 2. Repository authority

| Item | Value | Evidence/status |
|---|---|---|
| Host repository | `/home/lc/super_livo/src/FAST-LIVO2` | active host |
| Host initial branch | `main` | clean at bootstrap |
| Host baseline SHA | `0d2c0346107b75b59934975adec9a6eeeb913c64` | equals `origin/main` |
| Integration branch | `prob-livo` | created from host `main` |
| Host origin | `https://github.com/Scar-c/FAST-LIVO2.git` | fetch/push remote |
| Host upstream | none configured | explicitly checked |
| Prob-LIO reference | `/home/lc/super_livo/ref/Super-LIO` | read-only implementation oracle |
| Prob-LIO reference branch | `prob-lio` | clean at bootstrap |
| Prob-LIO reference SHA | `9fc949f46291c0fa76e5b7cdb372c940eb4b3f6e` | canonical P0–P4 confirmed |
| Prob-LIO origin | `https://github.com/Scar-c/Super-LIO.git` | reference remote |
| Legacy workspace | `/home/lc/prob_lio/src/Super-LIO` | reusable assets only |
| Dataset root | `/home/lc/super_livo/bag` | NTU and OXFORD present |

The host `main` worktree was clean before branch creation; no pre-existing
work was overwritten. The reference worktree was clean, its branch/HEAD and
remote were recorded, and no project write was made there. The reference HEAD
contains the canonical P0–P4 path and also contains explicitly marked P5
experimental code; this SPEC points no migration seam at that code.

## 3. State contract

FAST-LIVO2 retains its 19D state ABI and there is exactly one shared nominal
state and covariance:

```text
0–2    R
3–5    p
6      inverse exposure
7–9    v
10–12  gyro bias
13–15  accel bias
16–18  gravity
```

The canonical Super 18D physical state maps as follows:

```text
Super R   0–2   -> host 0–2
Super p   3–5   -> host 3–5
Super v   6–8   -> host 7–9
Super bg  9–11  -> host 10–12
Super ba 12–14  -> host 13–15
Super g  15–17  -> host 16–18
host exposure  -> host 6
```

The future `ProbESKF19` must preserve the Super physical semantics while
adding only FAST-LIVO2's inverse-exposure random-walk state/covariance in the
host layout. IMU propagation is Super-native semantics under the host
scheduler. A forbidden design is two independent filters followed by pose
copying:

```text
INVALID: Super filter + FAST-LIVO2 filter running independently
REQUIRED: one shared x19 / P19
```

I1 acceptance criterion is fixed now:

> With visual measurement disabled and exposure frozen, the 18D physical
> substate and corresponding covariance blocks of `ProbESKF19` must match
> canonical Super semantics under the same synthetic IMU/LiDAR update
> sequence within an explicit numerical tolerance.

No I1 implementation is part of Prompt 0.

## 4. Ownership and stable seam ledger

The current producer is the source observed at bootstrap. The future producer
is the intended owner after migration. `FROZEN` means the source semantics are
not to change before the listed stage.

| ID | Seam | Current producer | Future producer | Consumer | Authoritative implementation | Stage owner | Status |
|---|---|---|---|---|---|---|---|
| H0 | scheduler / scan recombination | `LIVMapper::run`, `sync_packages` | unchanged FAST-LIVO2 shell | IMU/LIO/VIO handlers | FAST-LIVO2 `src/LIVMapper.cpp:534-552,884-1030` | I0/I2 | FROZEN |
| H1 | shared `StatesGroup` / x19-P19 | `StatesGroup` | `ProbESKF19` in host ABI | scheduler, LIO, VIO | FAST-LIVO2 `include/common_lib.h:126-206` plus I1 contract | I1 | FROZEN ABI |
| H2 | IMU propagation / undistortion | `ImuProcess::Process2`, `UndistortPcl` | `ProbImuAdapter` under H0 | Prob-LIO scan input | FAST-LIVO2 scheduler + Super `ESKF::Predict`/`Propagation_Undistort` | I2 | CLOSED/PASS — owner verified |
| H3 | LiDAR downsample | PCL `VoxelGrid` in `handleLIO` | Super `VoxelGridClosest` | OctVox/backend | `include/prob_livo/super_native/OctVoxMap/VoxelGridFilter.h`; `src/prob_livo/prob_lio_backend.cpp:239-246` | I3 | CLOSED/PASS — owner verified |
| H4 | compact map insertion/storage | FAST `VoxelMapManager` octree | Prob `OctVox` | HKNN, plane provider | `include/prob_livo/super_native/OctVoxMap/OctVoxMap.hpp`; `src/prob_livo/prob_lio_backend.cpp:199-237,365-394` | I3 | CLOSED/PASS — owner verified |
| H5 | HKNN | FAST local voxel lookup/recursion | Super HKNN | QR and association | `include/prob_livo/super_native/OctVoxMap/OctVoxMap.hpp`; `src/prob_livo/prob_lio_backend.cpp:284-299` | I3 | CLOSED/PASS — owner verified |
| H6 | QR plane estimator | FAST PCA/eigen plane | Super QR plane solve | P3, LIO, provider | `include/prob_livo/super_native/prob_qr_plane.h`; `src/prob_livo/prob_lio_backend.cpp:300-316` | I3 | CLOSED/PASS — owner verified |
| H7 | P1 sensor covariance | FAST `calcBodyCov` | Prob P1 frame-correct covariance | P2/P4/current scan | `include/prob_livo/super_native/prob_geometry_p0_p4.h`; `src/prob_livo/prob_lio_backend.cpp:253-268` | I3 | CLOSED/PASS — owner verified |
| H8 | P2 map covariance | FAST pose-aware point covariance | Prob map covariance | OctVox/HKNN | `include/prob_livo/super_native/prob_geometry_p0_p4.h`; `src/prob_livo/prob_lio_backend.cpp:375-394` | I3 | CLOSED/PASS — owner verified |
| H9 | P3 QR covariance | FAST 6x6 PCA covariance | Prob 4x4 QR covariance | P4/provider | `include/prob_livo/super_native/prob_qr_plane.h`; `src/prob_livo/prob_lio_backend.cpp:307-315` | I3/I5 | CLOSED/PASS — owner verified |
| H10 | LiDAR association | FAST `build_single_residual` gate | Super legacy gate | LIO residual construction | `include/prob_livo/super_native/prob_qr_plane.h`; `src/prob_livo/prob_lio_backend.cpp:300-326` | I3 | CLOSED/PASS — owner verified |
| H11 | P4 weighting | FAST active variance weight | Prob P4 `w=1/R_i` | IESKF information update | `include/prob_livo/super_native/prob_geometry_p0_p4.h`; `src/prob_livo/prob_lio_backend.cpp:335-355` | I3 | CLOSED/PASS — owner verified |
| H12 | current scan output | `VoxelMapManager::pv_list_` | `pointWithVar`-compatible adapter | FAST VIO shell | FAST `pointWithVar` ABI, `include/common_lib.h:102-123` | I4 | NOT STARTED |
| H13 | visual plane provider | direct FAST map lookup | `VisualPlanePrior` adapter from OctVox/HKNN/QR/P3 | VIO plane/raycast paths | provider contract in §6 | I5 | NOT STARTED |
| H14 | visual 3σ consistency gate | `updateReferencePatch` | unchanged FAST gate | visual point lifecycle | FAST `src/vio.cpp:969-1021` | I6 | FROZEN UNTIL I6 |
| H15 | VIO update on x19/P19 | `VIOManager::updateState*` | unchanged visual update over shared state | shared estimator | FAST `src/vio.cpp:1398-1680` | I6 | FROZEN UNTIL I6 |
| H16 | visual `feat_map` lifecycle | FAST `VIOManager::feat_map` | unchanged FAST visual map | VIO | FAST `include/vio.h:126-130`, `src/vio.cpp:804-967` | I6 | FROZEN UNTIL I6 |
| H17 | map/cache invalidation | no Prob provider cache yet | derived/versioned/invalidation-only cache | H13/VIO | OctVox version authority, future seam | I5+ | NOT STARTED |

No FAST-LIVO2 LiDAR voxel/plane map may be retained in parallel purely for
VIO. `feat_map` is visual patch/map authority, not LiDAR geometry truth.

## 5. Source audit

### 5.1 FAST-LIVO2 host audit

| Authority/seam | Active source evidence | Audit result |
|---|---|---|
| Process entry and scheduler | `src/main.cpp:3-10`; `src/LIVMapper.cpp:534-552` | `run` performs `spinOnce`, `sync_packages`, first-frame handling, IMU processing, then state/mapping dispatch. |
| Synchronization / epoch ordering | `src/LIVMapper.cpp:884-1030` | `sync_packages` handles `ONLY_LIO` and `LIVO`; LIVO uses a camera epoch and schedules LIO before immediate VIO. |
| IMU entry | `src/LIVMapper.cpp:248-265`; `src/IMU_Processing.cpp:543-588` | `processImu` calls `Process2`, propagates `_state`, and publishes `state_propagat` to the map manager. |
| IMU undistortion | `src/IMU_Processing.cpp:237-537` | Active `UndistortPcl` interpolates IMU propagation and applies the LiDAR/IMU extrinsic chain. `Forward_without_imu` at `:151-234` is an active no-IMU mode, not the Super migration authority. |
| Shared state / scan type | `include/common_lib.h:102-206` | `pointWithVar` carries `point_b`, `point_i`, `point_w`, covariance fields and normal; `StatesGroup` is the 19D host ABI. |
| LIO handler | `src/LIVMapper.cpp:336-430` | `handleLIO` downsamples with the host PCL filter, transforms points, calls `VoxelMapManager::StateEstimation`, and copies `_state` and `_pv_list`. |
| Host map / plane | `src/voxel_map.cpp:15-135,219-326,338-510,532-641` | `calcBodyCov`, PCA/eigen plane initialization, host octree insertion, current `pv_list_`, and host LIO update are active. |
| Association gate | `src/voxel_map.cpp:643-786` | `build_single_residual` uses host geometric radius and sigma gate; this is current host behavior and is replaced for LiDAR by H10 Super legacy semantics. |
| Current scan handoff | `src/LIVMapper.cpp:370-372`; `include/LIVMapper.h:126` | `_pv_list = voxelmap_manager->pv_list_` is the current direct handoff. H12 will preserve the compatible fields through an adapter. |
| VIO entry/order | `src/LIVMapper.cpp:281-334`; `include/vio.h:145-167`; `src/vio.cpp:1786-1854` | `handleVIO` calls `processFrame` after LIO, then publishes; VIO manages its own visual lifecycle. |
| Sparse visual map | `include/vio.h:83-130`; `src/vio.cpp:804-967` | `feat_map`, `VisualPoint`, patch creation, and visual map-point lifecycle are active FAST authority. |
| Retrieval / raycast | `src/vio.cpp:352-602` | Visual sparse-map retrieval, image-pyramid patch preparation, and optional raycast lookup are active; it currently accepts the FAST plane map directly. |
| Reference patches | `src/vio.cpp:969-1100`; `:1102-1330` | Reference-patch selection/update and affine/homography-like patch projection are active. |
| Photometric update / exposure | `src/vio.cpp:1398-1680` | `updateStateInverse`/`updateState` use pyramid photometric residuals and can include inverse exposure. |
| Visual plane gate | `src/vio.cpp:969-1021` | Active baseline is the FAST visual 3σ plane-consistency gate. |

Active-vs-dead notes: the host final LIO weight at `src/voxel_map.cpp:445-449`
is active; pose-covariance alternatives at `:437-442` are commented/dead and
are not authority. The visual normal-orientation alternatives around
`src/vio.cpp:1015-1018` are commented/dead; the active sign selection is
`:1019-1020`. Comments and inactive alternatives do not change the freeze.

### 5.2 Prob-LIO reference audit

| Authority/seam | Active source evidence | Audit result |
|---|---|---|
| Super physical state | `src/super_lio/include/lio/ESKF.h:12-123` | Active `STATE`/`COV` are 18D with flattening `R p v bg ba g`. |
| Super IMU predict / ESKF | `src/super_lio/src/lio/ESKF.cpp:187-249,251-336` | Active native predict and information-form update; update uses `A = Pk^-1 + HᵀR^-1H`, then resets covariance. |
| Super IMU undistortion | `src/super_lio/src/lio/super_lio.cpp:384-450` | Active propagation-state interpolation and LiDAR-to-IMU correction. |
| Super downsample | `src/super_lio/src/lio/super_lio.cpp:452-455`; `include/OctVoxMap/VoxelGridFilter.h:15-80` | Active `VoxelGridClosest` selects the input point nearest each voxel center. |
| OctVox insertion and covariance storage | `include/OctVoxMap/OctVoxMap.hpp:104-210,417-467` | Active compact subvoxel insertion, representative covariance, and 0.1 m acceptance threshold. |
| HKNN | `include/OctVoxMap/OctVoxMap.hpp:470-553`; `HKNN_list60_gem.h` | Active representative-point search feeds the `KNNHeap<5>` path. |
| QR plane | `src/super_lio/src/lio/super_lio.cpp:16-44`; `include/lio/prob_qr_plane.h:40-190` | Active fixed-size QR solve remains the plane authority; P3 propagates its 4x4 covariance. |
| P1 | `include/lio/point_covariance.h:37-64,115-152` | Active sensor covariance and corrected LiDAR→IMU frame chain. |
| P2 | `include/lio/point_covariance.h:236-290` | Active world/map covariance functions use sensor, pose rotation, and position terms. |
| P3 | `include/lio/prob_qr_plane.h:98-190` | Active QR sensitivity and covariance accumulation; rank/finite checks are explicit. |
| P4 | `include/lio/point_covariance.h:292-366`; `src/lio/super_lio.cpp:898-945` | Active final weight is `1/(0.001 + plane + sensor)` with invalid-variance rejection and no current pose covariance. |
| Legacy association | `src/super_lio/src/lio/super_lio.cpp:48-54,800-890` | Active `compute_error`/legacy geometry gate remains the canonical association mode. |
| P5 negative check | `include/lio/point_covariance.h:369-601`; `src/lio/super_lio.cpp:823-840` | P5 association code exists but is experimental/config-gated; it is explicitly excluded from H10/H11 migration authority. |

Reference documentation independently corroborates this audit in
`ref/Super-LIO/spec/prob_lio/SPEC.md:82-160,453-521` and records P5 as
experimental/non-canonical. This project uses the source, not prompt prose,
as the audit basis.

## 6. Map and visual interface contracts

The first current-scan adapter should expose FAST-compatible `pointWithVar`
fields:

```text
point_b / p_L
point_i / p_I
point_w / p_W
sensor/body covariance
posterior world-point covariance
associated QR-plane normal when valid
```

Plane covariance remains in the plane-provider path; it is not added to
`pointWithVar`.

There is one LiDAR geometry authority: Prob OctVox. FAST-LIVO2 `feat_map`
remains the visual patch/VisualPoint authority. Any future cache is derived,
versioned, and invalidatable from OctVox; it cannot become an independent
truth source.

The future H13 provider has this conceptual contract:

```text
VisualPlanePrior {
    normal n
    offset d
    Sigma_nd 4x4
    support_center
    support_radius
}
```

The query is world point → Prob OctVox HKNN → the same Super QR plane → the same
P3 covariance → support descriptor. `[n,d]` alone does not determine a center.
The preferred center is the support centroid projected onto the QR plane:

```text
mu = (1/N) sum_i p_i
c  = mu - (n^T mu + d)n
```

Support radius must come from the same support set and be audited against FAST
semantics before implementation. Do not fabricate a FAST 6x6 covariance from
Prob-LIO's 4x4 `[n,d]` covariance.

LiDAR and visual policies remain separate:

```text
LiDAR correspondence              = Super legacy gate
LiDAR measurement weighting       = Prob-LIO P4
VisualPoint ↔ LiDAR-plane check    = FAST visual 3σ gate
```

The deterministic/Super-style visual plane gate is an ablation only. P5 is not
revived.

## 7. Frozen visual semantics and planned ablations

The following remain `FROZEN UNTIL I6`:

```text
FAST scheduler ordering and camera epoch
LiDAR-camera scan recombination
LIO -> VIO sequential ordering
VIOManager and visual feat_map lifecycle
image pyramid and photometric residual
inverse-exposure measurement semantics
homography/affine warp and reference-patch selection/update
raycast policy
visual normal refinement
visual map-point lifecycle
FAST visual 3σ plane-consistency gate
```

The downsample distribution difference is deliberately deferred until
camera-on closure. Planned ablation:

```text
A: full Prob-LIO packet / Super VoxelGridClosest
B: FAST-LIVO2 visual-source-parity downsample
```

No parameter tuning or dataset sweep is allowed during semantic migration.

## 8. Frozen non-goals

```text
no P5
no visual rewrite
no second LiDAR plane map
no separate Super and FAST filters
no parameter tuning during semantic migration
no dataset sweep before integration closure
no bulk import of the legacy workspace
no reliance on old build/ or devel/
```

## 9. Stage roadmap and current status

| Stage | Meaning | Status |
|---|---|---|
| I0 | Host / contract freeze | CLOSED / OWNER VERIFIED |
| I1 | `ProbESKF19` | CLOSED/PASS — Owner audit pending |
| I2 | Super IMU + undistort under LIVO2 scheduler | CLOSED/PASS — OWNER VERIFIED |
| I3 | Prob-LIO P0–P4 backend, camera OFF | CLOSED/PASS — OWNER VERIFIED; raw comparison `I3_TRAJECTORY_CLOSE_NONIDENTICAL` |
| I4 | `pointWithVar`-compatible current-scan adapter | NOT STARTED |
| I5 | `ProbPlaneProvider` | NOT STARTED |
| I6 | camera ON / FAST-LIVO2 visual sequential closure | NOT STARTED |
| I7 | visual-gate + downsample ablations | NOT STARTED |
| I8 | generalization / multi-dataset validation | NOT STARTED |

Prompt index names are P0–P8 but retain these I0–I8 semantics.

## 10. Workspace and import policy

The project-owned directories are:

```text
spec/prob_livo/
prompts/prob_livo/
tests/prob_livo/
tools/prob_livo/
eval/prob_livo/
results/prob_livo/
```

No old `build/` or `devel/` was copied. No symlink points into
`/home/lc/prob_lio`. Generated run artifacts belong under ignored
`results/prob_livo/runs/`, `logs/`, or `tmp/`, never in production source
directories.

Legacy assets use a `COPY-ON-DEMAND` policy:

1. identify the exact source file and inspect its assumptions;
2. record source commit and path;
3. copy the file, never symlink it, into the matching new namespace;
4. adapt paths/names locally;
5. test it against the FAST-LIVO2-host integration.

Initial import ledger: empty. No runtime/evaluator/tool asset was required for
Prompt 0.

| Source workspace | Source commit | Source path | Destination | Reason | Adapted? |
|---|---|---|---|---|---|
| none | — | — | — | Prompt 0 documentation/bootstrap only | — |

## 11. Dataset inventory

The lightweight inventory is maintained in
`results/prob_livo/README.md`. Prompt 0 only listed files and sizes; it did not
open or run the bags. Current sequences are NTU `eee_01..03`, `nya_01..03`,
`sbs_01..03`, and Oxford `Church_05`, `College_03`, `Palace_01`,
`Quarter_01`, plus Oxford calibration files. NTU has camera calibration,
LiDAR, IMU, Leica prism, and UWB metadata per sequence; camera-topic presence
is pending an explicit rosbag topic audit. Oxford has camera/IMU and LiDAR/IMU
calibration plus TUM ground truth. The legacy official NTU
evaluator and Oxford TUM evaluator are available at the legacy workspace path,
but neither was copied or executed.

## 12. Gate ledger

| Gate | Invariant | Evidence | Result |
|---|---|---|---|
| G-I0.1 | host identity known; clean intended baseline; `prob-livo` starts there | initial `git status`, `branch -vv`, `remote -v`, `rev-parse`, `log`; branch created from `main` at `0d2c034` | PASS |
| G-I0.2 | reference exists, clean, exact identity, canonical P0–P4, no P5 leakage | reference `git status`, branch/remote/HEAD/log; source and reference SPEC audit; H10/H11 point to legacy/P4, not P5 | PASS |
| G-I0.3 | source-grounded ownership map | §5 line-anchored audit and H0–H17 ledger | PASS |
| G-I0.4 | one x19/P19 with explicit Super mapping and I1 criterion | §3 and H1; host `StatesGroup`/reference `ESKF` source evidence | PASS |
| G-I0.5 | Prob OctVox only LiDAR geometry authority; FAST `feat_map` visual authority | §4 and §6; forbidden duplicate map recorded | PASS |
| G-I0.6 | FAST visual semantics remain frozen until I6 | §7 and H14–H16 | PASS |
| G-I0.7 | project dirs/hygiene/import provenance are established | §10, `.gitignore`, no copied legacy runtime artifacts or symlinks | PASS |
| G-I0.8 | untouched host compiles | successful `catkin_make --pkg fast_livo` from `/home/lc/super_livo` with `/home/lc/design_ws/devel/setup.bash`; target `fastlivo_mapping` built at 100% | PASS |
| G-I0.9 | Prompt 0 has no functional production-code change | final diff against host baseline is docs/dirs/`.gitignore` only; no `src/*.cpp` or estimator headers changed | PASS |

The first build attempt without the external VIKIT overlay failed at configure
time because `vikit_common` was not discoverable. The same unmodified host
then built successfully with the existing local dependency overlay. This is
recorded as environment diagnosis, not a source workaround.

## 13. Prompt 0 completion boundary (historical)

Prompt 0 does not implement I1 and does not begin a bag run. The project stops
after documentation, hygiene, baseline build, and audit evidence.

```text
I0 = CLOSED / OWNER VERIFIED
I1 = the next stage after this historical bootstrap boundary
I2–I8 = NOT STARTED

HOST authority   = FAST-LIVO2
LIO authority    = canonical Prob-LIO P0–P4
Visual authority = FAST-LIVO2 public source

Next stage = I1 ProbESKF19
```

## 14. Prompt 1 / I1 — ProbESKF19 filter core

I1 implements the shared-state ESKF seam only. It does not wire the core into
`LIVMapper`, the scheduler, VIO, undistortion, OctVox, or any dataset runner.
The final stage status is `CLOSED/PASS — Owner audit pending` only after the
gate ledger below, the host build, and the fast-forward push are recorded.

### 14.1 State and layout

`include/prob_livo/prob_eskf19.h` centralizes the host layout constants and the
bijective physical index list:

```text
host:     R[0:3] p[3:6] exposure[6] v[7:10] bg[10:13] ba[13:16] g[16:19]
physical: R[0:3] p[3:6] v[6:9] bg[9:12] ba[12:15] g[15:18]
indices:  0 1 2 3 4 5 7 8 9 10 11 12 13 14 15 16 17 18
```

Vector, covariance, `F_X`, and `F_W` embeddings all use that list. Dense
off-diagonal terms are preserved; exposure is an identity/no-direct-noise
state in `F_X`/`F_W` and is handled explicitly in `P_ee` when its configurable
random walk is enabled.

`ProbESKF19` stores a reference to the caller-owned `StatesGroup`. Its only
persistent members beyond options and diagnostics are IMU timing/history
bookkeeping; it does not duplicate x19 or P19. `ApplySuperLioIncrement19` is a
dedicated LIO retraction: right SO(3) exponential, additive Euclidean blocks,
and gravity addition followed by normalization to `gravity_norm`. FAST's
original `StatesGroup` operators remain untouched for visual ABI semantics.

### 14.2 Predict and update authority

Predict ports the active Super equations from
`ref/Super-LIO/src/super_lio/src/lio/ESKF.cpp:187-249`: midpoint IMU values,
pre-rotation nominal acceleration, right Jacobian, exact physical `F_X`/`F_W`,
Super noise-entry convention, and full P19 propagation through the centralized
embedding. Exposure mean is constant. Its default process is frozen; enabling
the option adds `cov_inv_expo * dt^2` to `P_ee`, with no direct physical/exposure
transition.

Update ports `ESKF.cpp:251-336`: frozen predicted snapshot and covariance,
per-iteration right-error prior/reset Jacobian, pose-only 6x6 information
embedded at host indices 0–5, full 19D information solve, Super retraction,
iteration-count/`need_converge` behavior, final rotational covariance reset, and
symmetrization. The 6x6 callback cannot directly measure exposure; exposure can
only respond through existing P_xe coupling.

The production observation callback explicitly receives
`(state, need_converge, HT_Vinv_H, HT_Vinv_r)`. The corrective lifecycle is
`[false,false]` for a two-callback update and
`[false,false,false,true]` for a four-callback update, matching the canonical
Super callback phase before the measurement producer runs.

### 14.3 Independent oracle and tests

`tests/prob_livo/oracle/super_eskf_oracle.h` is a minimal test-only translation
of the canonical Super ESKF equations in a separate namespace. It is derived
from reference commit `9fc949f46291c0fa76e5b7cdb372c940eb4b3f6e` and the exact
source paths/line ranges above; it is not linked to production and does not
call any `ProbESKF19` helper. The test runner is split into layout, retraction,
predict, update, and exposure translation units.

### 14.4 I1 gate ledger

| Gate | Invariant / authority / observable / tolerance | Negative or adversarial fixture and failure class | Result |
|---|---|---|---|
| G-I1.0 | I0 frontier `9ed486c`; pre-I1 host build with the existing VIKIT overlay; RC 0 | no source mutation at baseline; build/configuration failure | PASS |
| G-I1.1 | centralized x19↔18D vector/covariance, dense SPD off-diagonals, F embeddings; exact roundtrip | contiguous `[0:18]` extraction, velocity/bias off-diagonal mutation, exposure overwrite/drop; index/embedding loss | PASS |
| G-I1.2 | `ApplySuperLioIncrement19` vs independent oracle, small and dense increments; state/gravity error ≤2e-12 | FAST `operator+=` gravity path; missing normalization/retraction semantics | PASS |
| G-I1.3 | zero/finite gyro, nonidentity R, biases, 0.01–0.07 s multi-step midpoint nominal parity; state error ≤2e-11 | wrong rotation/acceleration order or timestamp; nominal divergence | PASS |
| G-I1.4 | full dense physical P parity using Super right Jacobian/F/Q; covariance error ≤2e-10 | omit right Jacobian, wrong FAST F, move accelerometer noise, square covariance entries | PASS |
| G-I1.5 | P_xe=0 isolation and nonzero P_xe full-matrix propagation; cross error ≤3e-10 | zero valid cross covariance; exposure process transition leakage | PASS |
| G-I1.6 | 2-iteration, 4-iteration, max-iteration, callback sequence, prior and final reset, dense information; state ≤3e-11/cov ≤3e-10 | omit final reset, converge on first pass, direct exposure/contiguous H mutation; reset/iteration/index error | PASS |
| G-I1.7 | LiDAR callback is 6x6 pose-only at host 0–5; zero-cross exposure unchanged, nonzero-cross indirect response | host `[0:7]` direct exposure row; forbidden measurement coupling | PASS |
| G-I1.8 | finite symmetric posterior and near-PSD eigenvalue; symmetry ≤1e-12, min eigenvalue >−1e-8 | explicit nonfinite/asymmetric/negative-eigenvalue/singular classifications | PASS |
| G-I1.9 | `git diff 9ed486c..HEAD` for `include/common_lib.h` and `src/vio.cpp` is empty; static ABI assertions | no visual source/operator change; ABI/layout regression | PASS |
| G-I1.10 | `catkin_make --pkg fast_livo` builds `fastlivo_mapping`, existing libraries, and isolated test target; no LIVMapper callsite or bag run | static runtime grep and executable link check; runtime integration/scope leak | PASS |
| G-I1.11 | public API takes `StatesGroup&`, exposes future scheduler timing/update seam, no persistent duplicate state/cov | source contract audit for copied filter state or second P; ownership violation | PASS |

All numerical evidence is deterministic and printed by
`prob_livo_i1_tests`; no `vector<bool>`, tautological production-vs-production
comparison, or bag execution is used.

## 15. I1 numerical and scope summary

The strongest observed errors are:

```text
retraction state/gravity:                 0 to machine precision
predict physical nominal/covariance:      0 in the oracle comparison
update rotation/position/velocity/bias/g: <= 2.93e-15
update physical covariance:               6.26e-13 (2 iterations)
nonzero-cross full P19 predict:           1.14e-13
posterior covariance symmetry:            9.06e-14
```

The negative fixtures are intentionally nonzero (for example, missing
right-Jacobian error `12.73`, direct exposure-row error `7.10`, and omitted
final-reset error `2.20e-3`), demonstrating that the tests discriminate the
required semantics. I1 makes no scheduler integration, VIO behavior change,
OctVox/P1–P4 import, P5 use, bag run, or dataset accuracy claim.

The I1 state after successful completion is:

```text
I0 = CLOSED / OWNER VERIFIED
I1 = CLOSED / OWNER VERIFIED
I2 = the next stage after this historical I1 boundary
I3–I8 = NOT STARTED

FAST-LIVO2 StatesGroup ABI = preserved
ProbESKF19 LIO semantics   = canonical Super ESKF
shared state               = one x19/P19 design
runtime integration        = NOT STARTED

Next stage = I2 Super IMU + undistortion under FAST-LIVO2 scheduler
```

Prompt 2 supersedes this historical boundary.

## 16. Prompt 2 / I2 — Super-native IMU and undistortion adapter

I2 closes the scheduler-owned H2 seam without replacing FAST's LiDAR map or
visual runtime. `include/prob_livo/prob_imu_adapter.h` and
`src/prob_livo/prob_imu_adapter.cpp` provide `ProbImuAdapter`, which accepts a
`LidarMeasureGroup`, uses the caller-owned `ProbESKF19`/`StatesGroup`, and
returns the explicitly named `prob_scan_undistort_imu` cloud. The adapter has
no second nominal pose or covariance.

### 16.1 Scheduler contract

The active `LIVMapper::sync_packages` source remains the scheduler authority:

| Mode | State epoch start | State epoch end | Point-time origin | IMU boundary |
|---|---|---|---|---|
| ONLY_LIO | `last_lio_update_time` (first initialized from scan header) | `measures.back().lio_time == lidar_frame_end_time` | `lidar_frame_beg_time` because raw curvature stays scan-header-relative | consume only `stamp <= lidar_frame_end_time` |
| LIVO LIO | `last_lio_update_time` | `measures.back().lio_time == image_capture_time` | `last_lio_update_time` because production cut rebases current points | collect only `stamp <= image_capture_time`; current points are pre-cut |

The LIVO current/next rebase expression is centralized in
`prob_livo::RebaseLivoPoint`, and production `sync_packages` calls that helper.
The adapter validates finite times, monotonic IMU input, current-point time
range, and the current/next boundary. It advances
`last_lio_update_time` exactly once, only after propagation and undistortion
success.

### 16.2 Super endpoint decision

The reference `SuperLIO::Propagation_Undistort()` calls `SetObsTime` with
`lidar.end_time` and passes its buffered IMUs to `ESKF::Predict`. The reference
`Predict` clips a sample whose timestamp is after `current_obs_time` to the
observation endpoint while retaining that sample as IMU history. FAST's
`sync_packages` consumes through `<=` endpoint and leaves newer samples in its
buffer, so H2 exposes an optional non-consuming look-ahead sample to the
adapter. If a final partial interval is required and the sample is absent, the
adapter rejects the epoch before mutating state; it never stops at the last
pre-endpoint sample.

### 16.3 Initialization, trace, and output

Initialization follows `SuperLIO::kf_init`: incremental mean gyro/acceleration,
minimum 50 samples, `gravity = -mean_accel * gravity_norm / ||mean_accel||`,
gyro bias equal to mean gyro, acceleration scale, gravity alignment, yaw and
robot-origin transforms, Super initial physical covariance, and the last IMU
timestamp in the accepted initialization measure. Exposure mean/covariance
remains the host visual state.

`ProbESKF19::Predict` now exposes accepted `PropagationSnapshot` values
`(time,R,p,v,a,w)` from its own state transition. The adapter records those
snapshots and uses no parallel trajectory. Undistortion applies the canonical
quaternion slerp, `p_h + v_h*tau + 0.5*a_t*tau^2`, configured LiDAR→IMU
extrinsic, and inverse endpoint pose. Its frame is scan-end IMU/body frame.

### 16.4 I2 gate ledger

| Gate | Invariant / authority / evidence | Negative fixture | Result |
|---|---|---|---|
| G-P2.0 | baseline `ecd8058`, existing I1 focused test, overlaid full build before edits | no source changes before baseline | PASS |
| G-I1.C1 | direct callback bool; `[F,F]`, `[F,F,F,T]`; independent Super oracle and final query | always-false, delayed, after-callback, indirect-state lifecycle mutations | PASS |
| G-I1.C2 | all I1 math gates remain green; actual Super SO(3) golden max matrix error `1.06e-7`, log error `6.67e-8` | linear SO(3) mutation and all prior I1 adversarial fixtures | PASS |
| G-I2.1 | production-used scheduler rebase helper; ONLY/LIVO origins, cut partition, boundary and source guard | header-origin, camera-as-both-origin, post-cut-current | PASS |
| G-I2.2 | exact endpoint and clipped partial endpoint; physical state/covariance parity, transactional missing-lookahead rejection | stop at last IMU; no look-ahead; post-endpoint IMU | PASS |
| G-I2.3 | level/tilted/scaled acceleration, bias, 50-sample transition, yaw/origin transform; init physical covariance parity | early init, hardcoded gravity, skipped transform | PASS |
| G-I2.4 | every accepted ProbESKF19 snapshot compared for time/R/p/v/a/w; max error `0` in deterministic oracle fixture | post-rotation acceleration ordering | PASS |
| G-I2.5 | no/pure translation/pure rotation/coupled motion, endpoints, midpoint, nonidentity extrinsic; max XYZ `2.05e-7` | LiDAR-frame, missing/inverted extrinsic, linear rotation, FAST formula | PASS |
| G-I2.6 | point count/order, intensity, curvature, normals and current/next identity preserved | timestamp sorting/identity loss | PASS |
| G-I2.7 | three sequential epochs, exact boundary continuity, one anchor advance per success | double integration, dropped gap, early anchor update | PASS |
| G-I2.8 | source points cross camera time through production rebase helper; current-only packet reaches image endpoint | post-cut point contamination | PASS |
| G-I2.9 | source guard confirms default `p_imu->Process2` path and no adapter output/ProcessLioEpoch call in `LIVMapper` | direct Prob IMU-frame → FAST `feats_undistort`/VoxelMap wiring | PASS |

Focused I2 tests are in `tests/prob_livo/test_i2_*` and use an independent
oracle from reference commit `9fc949f46291c0fa76e5b7cdb372c940eb4b3f6e`.
The I2 lifecycle handoff is now consumed by the camera-OFF backend. I3 uses
the same `ProbESKF19` state, the Super-native downsample/OctVox/HKNN/QR/P1/P2/
P3/P4 path, and Super legacy association. FAST remains the ROS and scheduler
shell; no camera topic is subscribed in the baseline and no VIO update is
executed.

The current stage state is:

```text
I0 = CLOSED / OWNER VERIFIED
I1 = CLOSED / OWNER VERIFIED
I2 = CLOSED/PASS — OWNER VERIFIED
I3 = CLOSED/PASS — OWNER VERIFIED
I4–I8 = NOT STARTED

H0 FAST scheduler             = preserved
H1 shared x19/P19             = ProbESKF19
H2 IMU init/prop/undistortion = Super-native ProbImuAdapter
I2 output frame               = scan-end IMU frame
Prob-LIO map backend          = ProbLioBackend, camera OFF, P0–P4 active
EEE01 baseline                = 0.052901597 m official ATE; raw trajectory close/non-identical

Next stage = I4 `pointWithVar`-compatible current-scan adapter
```

## 17. Prompt 3 / I3 — camera-OFF Prob-LIO P0–P4 baseline

I3 closes the I2 initialization-to-runtime handoff and wires the camera-OFF
Prob-LIO path into the FAST-LIVO2 ROS/scheduler shell. `ProbLioBackend` is the
single lifecycle owner for `IMU_INIT`, `MAP_INIT`, and `RUN`; it owns the
caller-shared `StatesGroup`, `ProbESKF19`, `ProbImuAdapter`, Prob OctVox map,
per-scan buffers, and trajectory output. `LIVMapper` only dispatches the
already-synchronized scheduler packet and publishes the backend result.

The backend preserves the required Super lifecycle: 50 IMU samples complete
KF initialization, four raw LiDAR scans are inserted during map initialization,
and later scans execute Super downsample, HKNN, QR plane fitting, P1/P2/P3
covariance, Super legacy association, P4 probabilistic weighting, the shared
ProbESKF19 update, and covariance-aware map insertion. P5 is not imported or
enabled. Camera loading, image subscription, VIO processing, and the FAST
LiDAR voxel map are disabled for this baseline.

### 17.1 I3 gate ledger

| Gate | Evidence | Result |
|---|---|---|
| G-I3.1 | `prob_livo_i3_tests`: lifecycle/map-init, downsample, OctVox, HKNN, QR/P3, P1/P2/P4, and backend seam; 79 checks | PASS |
| G-I3.2 | `prob_livo_backend` and `fastlivo_mapping` build from the host workspace | PASS |
| G-I3.3 | canonical runner requires a clean tree, uses one FAST node/backend, replays only IMU/LiDAR, records effective params and unique run metadata | PASS |
| G-I3.4 | EEE01 whole-bag camera-OFF run: `play_rc=0`, `node_rc=0`, `ground_truth_rc=0`, `evaluation_rc=0`, `run_rc=0`, 3595 trajectory rows | PASS |
| G-I3.5 | same NTU official evaluator and raw primary comparison against the legacy P4 artifact; no mutual SE(3) alignment | `I3_TRAJECTORY_CLOSE_NONIDENTICAL` |

### 17.2 Canonical EEE01 baseline record

```text
runner: tools/prob_livo/run_eee01_camera_off.sh
run_dir: results/prob_livo/runs/eee01_camera_off_p0_p4_retry
run_head: ce805bb8e02eb234d004a6e2d1c54ed35bfc1ba5
bag_sha256: 7ea43946cffdd49c88d993ad3f192a4e90a8f6826eddc2ef1a9d4f5343ca6c17
config_sha256: c8f94f130e599b928c3f02c3f3d3b2009ae01df76aec32f6ac96b6a987311ef3
camera: OFF
replayed_topics: /imu/imu, /os1_cloud_node1/points
backend: ProbLioBackend P0-P4
trajectory_rows: 3595
official_ate_rmse_m: 0.05290159739482509
official_matched: 3016
legacy_reference: /home/lc/prob_lio/src/Super-LIO/results/prob_lio/p11_smoke_eee_p4_lc/trajectory.tum
legacy_reference_rows: 3981
raw_compare_matched_rows: 3594
raw_translation_rmse_m: 0.1272755745726123
raw_rotation_rmse_rad: 0.01831955642234057
raw_alignment: NONE_RAW_WORLD_FRAMES
classification: I3_TRAJECTORY_CLOSE_NONIDENTICAL
```

The official evaluator remains the NTU VIRAL dataset-author benchmark with
`SE3_UMEYAMA_NO_SCALE` internal evaluation alignment, Leica interpolation at
estimate timestamps, and the fixed prism lever arm. The separate legacy
comparison is intentionally raw-world-frame and has exactly one classification
from `tools/prob_livo/compare_trajectories.py`. No parameter sweep or tuning was
performed. The ignored run directory is the executable evidence bundle; the
tracked evidence index records its identity and hashes.
