# Prob-LIVO Integration Specification

Status: Prompt 0 / I0 bootstrap; current status is `CLOSED/PASS pending Owner audit`.

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
| H2 | IMU propagation / undistortion | `ImuProcess::Process2`, `UndistortPcl` | Super-native IMU adapter under H0 | current scan packet | FAST-LIVO2 shell + Super `Propagation_Undistort` semantics | I2 | NOT STARTED |
| H3 | LiDAR downsample | PCL `VoxelGrid` in `handleLIO` | Super `VoxelGridClosest` | OctVox/backend | Super `VoxelGridFilter.h:15-80` | I3 | NOT STARTED |
| H4 | compact map insertion/storage | FAST `VoxelMapManager` octree | Prob `OctVox` | HKNN, plane provider | Super `OctVoxMap.hpp:104-210,417-467` | I3 | NOT STARTED |
| H5 | HKNN | FAST local voxel lookup/recursion | Super HKNN | QR and association | Super `OctVoxMap.hpp:470-553`, `HKNN_list60_gem.h` | I3 | NOT STARTED |
| H6 | QR plane estimator | FAST PCA/eigen plane | Super QR plane solve | P3, LIO, provider | Super `super_lio.cpp:16-44`, `prob_qr_plane.h:40-190` | I3 | NOT STARTED |
| H7 | P1 sensor covariance | FAST `calcBodyCov` | Prob P1 frame-correct covariance | P2/P4/current scan | Super `point_covariance.h:37-64,129-152` | I3 | NOT STARTED |
| H8 | P2 map covariance | FAST pose-aware point covariance | Prob map covariance | OctVox/HKNN | Super `point_covariance.h:236-290` | I3 | NOT STARTED |
| H9 | P3 QR covariance | FAST 6x6 PCA covariance | Prob 4x4 QR covariance | P4/provider | Super `prob_qr_plane.h:98-190` | I3/I5 | NOT STARTED |
| H10 | LiDAR association | FAST `build_single_residual` gate | Super legacy gate | LIO residual construction | Super `compute_error`, `super_lio.cpp:48-54` | I3 | NOT STARTED |
| H11 | P4 weighting | FAST active variance weight | Prob P4 `w=1/R_i` | IESKF information update | Super `point_covariance.h:292-366` | I3 | NOT STARTED |
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
| I0 | Host / contract freeze | CLOSED/PASS pending Owner audit |
| I1 | `ProbESKF19` | NOT STARTED |
| I2 | Super IMU + undistort under LIVO2 scheduler | NOT STARTED |
| I3 | Prob-LIO P0–P4 backend, camera OFF | NOT STARTED |
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
LiDAR, IMU, Leica prism, and UWB metadata per sequence; Oxford has camera/IMU
and LiDAR/IMU calibration plus TUM ground truth. The legacy official NTU
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

## 13. Prompt 0 completion boundary

Prompt 0 does not implement I1 and does not begin a bag run. The project stops
after documentation, hygiene, baseline build, and audit evidence.

```text
I0 = CLOSED/PASS — Owner audit pending
I1–I8 = NOT STARTED

HOST authority   = FAST-LIVO2
LIO authority    = canonical Prob-LIO P0–P4
Visual authority = FAST-LIVO2 public source

Next stage = I1 ProbESKF19
```
