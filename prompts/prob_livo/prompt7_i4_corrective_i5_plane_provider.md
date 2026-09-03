# Prob-LIVO Integration — Prompt 7
## I4 Corrective Closure + I5 ProbPlaneProvider

> This round has two strictly ordered parts.
>
> **Part A:** correct and close I4. The current `pointWithVar` adapter is structurally useful, but its FAST visual semantics are incomplete:
>
> 1. `body_var` frame contract is wrong;
> 2. `var` incorrectly equals `var_nostate`;
> 3. `normal` is left zero although the current Prob-LIO association already owns a valid accepted QR-plane normal;
> 4. the previous “real consumer compatibility” test was only a type/compile check and is not sufficient.
>
> **Part B:** only after I4 is cleanly closed, implement I5 `ProbPlaneProvider`: a read-only plane-query interface backed by the authoritative Prob OctVox + Super HKNN + Super QR geometry, for future FAST-LIVO2 visual/raycast/reference-patch semantics.
>
> Do **not** enable camera/VIO runtime in this round.
> Do **not** implement I6 visual update.
> Do **not** reintroduce P5.
> Keep production code lean.

---

# 0. Current Owner state

Accepted state before this prompt:

```text
I0 = CLOSED / OWNER VERIFIED
I1 = CLOSED / OWNER VERIFIED
I2 = CLOSED / OWNER VERIFIED
I3 = CLOSED / OWNER VERIFIED

I4:
  point coordinate adapter          = GREEN
  point_crossmat                    = GREEN
  var_nostate sensor-only world cov = GREEN
  dormant integration               = GREEN
  LIO invariance                    = GREEN

  body_var FAST-compatible semantics = RED
  var full visual covariance         = RED
  normal current-scan semantics       = RED
  real consumer seam                  = FAIL / false-positive

  status = OPEN / CORRECTIVE REQUIRED

I5 = BLOCKED
I6–I8 = NOT STARTED
```

Current repo:

```text
~/super_livo/src/FAST-LIVO2
branch: prob-livo
expected start HEAD:
8464b33d9c936dfa2ed559e2b2f26962ac614439
```

Prob-LIO reference:

```text
~/super_livo/ref/Super-LIO
HEAD:
9fc949f46291c0fa76e5b7cdb372c940eb4b3f6e
```

Historical legacy source remains available for diagnostics if needed:

```text
~/prob_lio/src/Super-LIO
```

Remember that legacy workspace was renamed; do not reuse stale absolute-path build artifacts blindly.

---

# 1. Production philosophy

Owner explicitly rejects defensive-programming bloat.

Use the rule:

```text
tests may be aggressive
production must remain simple
```

Do not add:
- per-point logging;
- repeated expensive PSD/eigensolver checks;
- generic safety wrappers;
- silent fallback geometry;
- duplicate map/state ownership;
- fake “valid” normals/planes;
- second LIO association path.

If a field is unavailable, keep validity explicit rather than inventing data.

---

# 2. Startup consensus and baseline gate

Before edits:

```bash
cd ~/super_livo/src/FAST-LIVO2
git status --short
git branch -vv
git rev-parse HEAD
git rev-parse origin/prob-livo
git log --oneline -15
```

Build and run current focused tests.

### HARD GATE G-P7.0

Require:
- exact expected start HEAD;
- clean tree;
- current I1/I2/I3/P4/I4 tests PASS before modification;
- full required host build PASS.

Register exact prompt:

```text
prompts/prob_livo/prompt7_i4_corrective_i5_plane_provider.md
```

---

# PART A — I4 CORRECTIVE CLOSURE

# 3. Re-audit authoritative FAST `pointWithVar` semantics

Before changing code, re-open the exact current FAST source and record the real producer/consumer semantics for:

```text
point_i
point_b
point_w
body_var
var_nostate
var
normal
point_crossmat
```

At minimum inspect:
- `include/common_lib.h`;
- FAST current-point preparation;
- FAST covariance construction;
- FAST LIO posterior world covariance update;
- `VIOManager::processFrame`;
- `generateVisualMapPoints()`.

The source contract, not the previous Prompt6 assumption, is authoritative.

---

# 4. Correct field contract

The corrected adapter must preserve the FAST visual-facing contract while sourcing all geometry from Prob-LIO.

Expected semantic structure to verify from source:

```text
point_i
  current undistorted point in IMU/body frame

point_b
  corresponding LiDAR-body-frame point

point_w
  world point using shared current state

body_var
  LiDAR-frame sensor covariance (FAST field contract)

var_nostate
  world-frame sensor-only covariance

var
  FAST visual-compatible full world-point covariance
  = sensor term + current-state propagation required by FAST visual semantics

point_crossmat
  skew(point_i)

normal
  accepted current LiDAR map-plane normal in world frame
  from the already-computed Super QR correspondence;
  invalid/zero only when no accepted correspondence exists
```

If current FAST source proves any field means something else, follow source and document the correction.

---

# 5. `body_var` corrective

The Prob P1 chain already has the information necessary to recover:

```text
Sigma_L
Sigma_I
```

Current adapter incorrectly writes IMU-frame `Sigma_I` into `body_var`.

Correct the contract so:

```text
point_b  ↔ Sigma_L
point_i  ↔ Sigma_I
```

Do not infer `Sigma_L` from an unrelated FAST covariance path.

Preferred:
- preserve/carry the canonical P1 LiDAR-frame covariance alongside the IMU-frame covariance;
- or invert the known rigid rotation exactly if source identity is guaranteed.

Do not re-run a separate FAST covariance model.

### HARD GATE G-I4.C1

Fixtures with nonidentity `R_LI` must prove:

```text
body_var == Sigma_L
Sigma_I == R_LI Sigma_L R_LI^T
```

and must fail if:
- `body_var = Sigma_I`;
- extrinsic is applied twice;
- wrong rotation direction is used.

---

# 6. `var_nostate` vs `var`

This distinction must match FAST visual semantics.

Keep:

```text
var_nostate = R_WI Sigma_I R_WI^T
```

if confirmed by source.

For `var`, reproduce the actual FAST visual world-point covariance propagation.

Audit the exact production equation from current FAST source.

Expected structure conceptually:

```text
Sigma_world_full =
    sensor_world_term
  + rotational state term
  + position state covariance
  + any required rotation/position cross terms if the source includes them
```

Do **not** approximate from memory.

Use the exact shared x19/P19 physical-state block mapping.

Important:

```text
Using pose covariance here for FAST visual point uncertainty
is NOT the excluded Prob-LIO P5 LiDAR association.
```

P5 remains excluded from:
- LiDAR association;
- LiDAR P4 weighting;
- current LIO gate.

This is visual input covariance only.

### HARD GATE G-I4.C2

Compare adapter `var` against an independent source-faithful FAST covariance oracle using the same:
- point;
- state;
- covariance;
- extrinsic.

Fixtures:
- zero pose covariance;
- translation covariance only;
- rotation covariance only;
- nonzero rotation-position cross covariance if relevant;
- coupled state covariance.

Require:
- `var_nostate` remains sensor-only;
- `var` includes exactly the source-required state term;
- no P5 logic enters the LIO backend.

Negative mutations:
- `var = var_nostate`;
- add wrong physical P indices;
- use exposure index;
- apply pose covariance to P4 weight.

---

# 7. Current-scan normal corrective

Do not wait for I5 to provide the current scan's accepted normal.

The Prob LIO observation already owns, per current point:
- accepted/effective mask;
- Super QR plane coefficients.

Use the accepted plane normal already produced by the current LIO correspondence lifecycle.

Contract:

```text
if current point has accepted Super legacy/QR correspondence:
    pointWithVar.normal = accepted world-frame plane normal
else:
    normal = invalid/zero
```

The normal must correspond to the same point identity and same accepted association used by the LIO update.

Do not:
- re-query the map;
- run a second QR;
- use FAST plane fitting;
- fabricate a normal for rejected points.

### HARD GATE G-I4.C3

For a deterministic observation fixture:
- accepted point → exact QR normal copied;
- rejected point → invalid/zero;
- source index/order unchanged.

Negative mutations:
- normal copied from neighboring point;
- stale previous-iteration normal;
- recomputed FAST normal;
- all-zero normal for accepted points.

---

# 8. Real consumer seam — replace false-positive G-I4.6

The previous test only proved that a `VIOManager::processFrame` function pointer compiled.

That is insufficient.

Create a real semantic seam test around the actual FAST consumer logic used by `generateVisualMapPoints()` or the smallest production helper extracted from it.

Do not enable camera runtime.

The test must exercise real conditions such as:

```text
accepted point:
  normal != 0
  point_w valid
  var valid
  reaches the visual-map candidate path

rejected point:
  normal == 0
  is skipped exactly as FAST source semantics require
```

If direct invocation of the full function requires camera state, extract the smallest **production-used** predicate/preparation helper and test that helper.

Do not build a fake test-only consumer.

### HARD GATE G-I4.C4

Require:
- at least one accepted adapted point is not skipped due to normal;
- rejected adapted point is skipped;
- `var`/`point_w` are read with correct semantics;
- real production code path/helper is exercised.

---

# 9. I4 invariance

I4 remains observational to LIO.

Correcting `body_var`, `var`, and `normal` must not affect:
- ProbESKF19;
- OctVox;
- association;
- P4;
- trajectory.

### HARD GATE G-I4.C5

Adapter OFF vs ON:
- same LIO state/map;
- same trajectory hash where deterministic;
- same rows/ATE if runtime run is used.

No need for a full bag if focused production seam proves it, but a lightweight offline verification is acceptable.

---

# 10. I4 closure

Only after G-I4.C1–C5 pass:

```text
I4 = CLOSED / OWNER VERIFIED
I5 = ACTIVE
```

Suggested commit:

```text
fix(prob-livo): close pointWithVar visual semantics
```

Clean tree before I5.

---

# PART B — I5 ProbPlaneProvider

# 11. I5 mission

Implement a read-only plane-query provider backed by the authoritative Prob LiDAR geometry:

```text
query point / ray-derived world point
        ↓
Prob OctVox
        ↓
Super HKNN
        ↓
same Super QR plane estimator
        ↓
plane result for FAST visual semantics
```

This provider exists so future visual code can request local plane geometry without using FAST's old LiDAR geometry map.

It does not update the map.
It does not own a second map.
It does not run an estimator.
It does not enable camera/VIO yet.

---

# 12. Distinguish I4 current normal from I5 provider

Freeze this ownership boundary:

```text
I4:
  current scan point normal
  comes from the already accepted LIO correspondence

I5:
  arbitrary world-space plane query
  recomputes/querys local geometry from Prob OctVox
```

Examples of future I5 consumers:
- visual raycast;
- plane-supported reference patch geometry;
- reference patch update;
- future visual plane consistency checks.

Do not use I5 to redundantly recompute current LIO normals.

---

# 13. Audit FAST visual plane needs before API design

Before implementing I5, audit current FAST-LIVO2 visual source for every place that needs local plane geometry.

At minimum inspect:
- visual point creation;
- `generateVisualMapPoints`;
- raycast/search path;
- reference patch construction/update;
- any homography/plane warp path;
- any current 3σ plane-consistency gate;
- any use of plane center/radius/normal.

Create a table:

```text
consumer
query input
required output
frame
whether covariance is used
whether center is used
whether radius is used
whether normal orientation matters
```

Do not design an oversized generic plane struct from imagination.

---

# 14. Plane result contract

Recommended provider result, subject to source audit:

```text
valid
normal_W
d
center_W
support_count
support_ids / support keys (diagnostic-capable)
radius or spread metric if FAST consumer requires it
optional plane uncertainty representation only if actually required now
```

Do not fabricate FAST's legacy 6×6 plane covariance.

Canonical Prob plane uncertainty is:

```text
4×4 covariance of [n, d]
```

If I5 consumers do not need uncertainty yet, keep it available internally/provider-side but do not force a fake compatibility conversion.

---

# 15. Plane center semantics

Use the same HKNN support that produced the QR plane.

If FAST visual needs a plane center, use the frozen formula:

```text
mu = mean(support points)
center = mu - (n^T mu + d) n
```

This is the orthogonal projection of the support centroid onto the fitted plane.

Do not use:
- arbitrary nearest point;
- query point itself;
- voxel center;
- fake patch center.

### HARD GATE G-I5.1

Deterministic support fixture:
- QR plane known;
- support centroid known;
- center satisfies plane equation;
- `center - mu` parallel to normal;
- center matches formula.

Negative mutations:
- center = mu without projection;
- center = query point;
- wrong d sign.

---

# 16. Normal orientation policy

Audit whether FAST visual consumers require a deterministic sign convention.

If source relies only on plane geometry where `(n,d)` and `(-n,-d)` are equivalent, preserve QR source orientation.

If a consumer requires view-facing orientation, implement that orientation at the consumer seam, not by silently mutating the canonical provider plane.

Document sign policy explicitly.

---

# 17. Radius / local support spread

If FAST visual source uses a plane radius or support extent, audit the exact semantic role.

If required, derive it only from the same HKNN support.

Possible source-grounded choices may include:
- max projected support distance from center;
- source FAST plane radius logic.

Use the actual FAST visual meaning, not an arbitrary constant.

### HARD GATE G-I5.2

If radius is required:
- source-faithful fixture;
- same support;
- exact radius/spread semantics.

If not required at I5, mark `radius` intentionally absent and explain why.

---

# 18. Query input semantics

The provider query must be explicit.

Preferred primary API:

```text
QueryAtWorldPoint(p_W)
```

because future raycasting can first produce a world point and query directly.

Do not hide transforms inside an ambiguous API.

If future FAST visual source naturally queries:
- ray origin + direction + depth;
- a `pointWithVar`;
- reference patch point;

provide only a thin overload that resolves to `p_W`.

One world-frame geometric query authority.

---

# 19. Use authoritative Prob OctVox/HKNN/QR

The provider must reuse the same production components as Prob LIO:

```text
OctVox map
Super HKNN
Super QR
same plane-validity semantics
```

No:
- FAST VoxelMapManager;
- PCL KD-tree;
- new spatial map;
- S3 spatial-balanced map;
- alternate PCA plane;
- duplicated neighbor table.

The same map object owned by `ProbLioBackend` is queried read-only.

---

# 20. Query independence from current filter update

I5 is a geometry query.

It must not:
- alter `need_converge`;
- mutate correspondence buffers;
- alter current LIO effect mask;
- insert map points;
- update plane cache in a way that changes LIO behavior;
- mutate filter state/covariance.

If caching is introduced for performance, it must be semantically transparent and not necessary for correctness.

Prefer no cache in I5 unless profiling proves need.

---

# 21. Plane uncertainty

Canonical Prob-LIO P3 provides:

```text
Sigma_[n,d] ∈ R^(4×4)
```

Audit whether future FAST visual consumers at the I5 boundary actually need it now.

If useful to expose, return it explicitly as:

```text
plane_cov_nd
```

with documented `[n_x,n_y,n_z,d]` ordering.

Do not convert to FAST's historical 6×6 `[normal, center]` covariance.

If a later visual interface genuinely requires another representation, that conversion belongs in a later adapter with a derived, mathematically justified transform.

### HARD GATE G-I5.3

If exposed:
- compare provider P3 covariance against canonical Prob QR covariance on deterministic support;
- correct `[n,d]` ordering;
- scalar residual variance at query point matches canonical formula.

---

# 22. Query validity

Provider validity must follow source-grounded geometry requirements:
- sufficient neighbors;
- QR rank/validity;
- canonical plane acceptance prerequisites where appropriate.

Do not apply current-scan Super legacy residual gate to arbitrary plane queries unless the FAST visual consumer is specifically asking for a query-point plane-consistency test.

Separate:

```text
plane exists / fitted successfully
```

from:

```text
query point is consistent with that plane
```

The provider should not silently conflate them.

---

# 23. HARD GATE G-I5.4 — HKNN identity parity

For deterministic map/query fixtures:
- exact same neighbor IDs/keys;
- same order;
- same support count;
- same point coordinates;
- same associated covariance where used.

Compare provider query support against direct canonical Prob backend HKNN.

Mutation:
- FAST map/KD-tree;
- different neighbor count/order;
- gate fails.

---

# 24. HARD GATE G-I5.5 — QR plane parity

Given the same support:
- rank;
- q;
- n,d;
- validity;

must match direct canonical QR.

Cases:
- clean plane;
- tilted plane;
- near-degenerate;
- rank-deficient reject.

---

# 25. HARD GATE G-I5.6 — provider end-to-end query

Use a real populated Prob OctVox fixture:

```text
map insert
→ provider QueryAtWorldPoint
→ HKNN
→ QR
→ result
```

Require:
- valid result where direct backend query is valid;
- invalid where insufficient/degenerate;
- center/radius/covariance fields consistent with contract.

This gate must exercise production provider + production map, not only individual helper functions.

---

# 26. HARD GATE G-I5.7 — read-only / no LIO side effects

Run the same LIO fixture or small deterministic epoch with:

```text
provider queries OFF
provider queries ON
```

Require identical:
- filter state;
- covariance;
- map content;
- trajectory;
- current association results.

Provider queries must be observational only.

---

# 27. Consumer-facing seam for future visual use

Do not enable VIO, but prepare a narrow interface that future visual code can call.

Preferred:

```text
ProbPlaneProvider::QueryAtWorldPoint(...)
```

owned/exposed by the Prob backend/controller.

Avoid exposing internal OctVox pointers throughout `vio.cpp`.

Future I6 should receive a plane-provider interface, not directly know map internals.

### HARD GATE G-I5.8

Compile and exercise the actual integration seam:
- backend owns map;
- provider references the same map;
- caller sees only provider API;
- no second map object instantiated.

---

# 28. Performance sanity

Super/HKNN is designed for fast queries. Do not prematurely cache every plane.

Do a lightweight bounded microbenchmark only:
- representative map size fixture or real-map snapshot if already easy;
- e.g. 1k–10k provider queries;
- report mean/median/p95 if convenient.

This is not a hard optimization gate.

The goal is to catch accidental O(N) scans or expensive allocations per query.

Do not add elaborate benchmarking infrastructure.

---

# 29. Suggested files

Adapt naming to repo, but preferred:

```text
include/prob_livo/prob_plane_provider.h
src/prob_livo/prob_plane_provider.cpp
tests/prob_livo/test_i5_plane_provider.cpp
```

Reuse:
- Prob OctVox type;
- HKNN;
- QR;
- P3 helpers.

Do not fork these implementations.

---

# 30. No runtime camera/VIO yet

I5 may be wired into the backend/controller as a dormant service.

Do not:
- subscribe camera;
- call `processFrame`;
- create/update VisualPoints;
- update reference patches;
- run photometric residuals.

Those belong to I6+.

---

# 31. Build / regression

After I5:
- full required build;
- rerun I1/I2/I3/P4/I4 tests;
- run I5 focused tests;
- verify camera-OFF LIO remains unchanged.

A full `eee_01` rerun is optional unless needed for side-effect proof. Prefer focused deterministic invariance tests if sufficient.

---

# 32. Commit sequence

Suggested:

### Commit A
```text
fix(prob-livo): close pointWithVar visual semantics
```

### Commit B
```text
feat(prob-livo): add OctVox-backed plane provider
```

### Commit C
```text
test(prob-livo): close plane provider parity gates
```

### Commit D
```text
docs(prob-livo): close I5 evidence
```

No force push.
Fast-forward only.

---

# 33. State transition

After I4 corrective passes:

```text
I4 = CLOSED / OWNER VERIFIED
I5 = ACTIVE
```

After all I5 gates:

```text
I5 = CLOSED/PASS — Owner audit pending
I6–I8 = NOT STARTED
```

Do not mark I5 Owner Verified.

---

# 34. Final report format

## Agent State Consensus
- start/final HEAD
- branch/origin
- clean state
- prompt registration

# PART A — I4 Corrective

## FAST pointWithVar Source Contract
Table:

```text
field | exact FAST meaning | frame | source producer | consumer
```

## Corrections
- body_var
- var_nostate
- var
- normal
- point_crossmat if unchanged

## G-I4.C1
body_var frame parity.

## G-I4.C2
full visual covariance parity.

## G-I4.C3
accepted QR normal parity.

## G-I4.C4
real consumer semantic seam.

## G-I4.C5
LIO invariance.

## I4 Decision

If passed:

```text
I4 = CLOSED / OWNER VERIFIED
```

# PART B — I5

## FAST Visual Plane Need Audit
For each consumer:
- input;
- normal;
- d;
- center;
- radius;
- covariance;
- frame.

## Provider API
- exact signature;
- ownership;
- map reference;
- result fields;
- validity semantics.

## Plane Center
- exact formula;
- support semantics.

## Radius
- source semantics or intentionally absent.

## Plane Covariance
- whether exposed;
- `[n,d]` ordering;
- no fake 6×6 conversion.

## Gates

```text
G-I5.1 plane center
G-I5.2 radius/spread if required
G-I5.3 P3 covariance if exposed
G-I5.4 HKNN identity parity
G-I5.5 QR parity
G-I5.6 production end-to-end query
G-I5.7 read-only / no LIO side effects
G-I5.8 integration ownership seam
```

## Performance Sanity
- bounded query timing;
- no accidental O(N) path.

## Build/Test
- commands;
- return codes;
- check counts.

## Scope Audit
Confirm:
- camera/VIO OFF;
- no VisualPoint creation/update;
- no reference patch update;
- no photometric update;
- no P5;
- no second LiDAR map;
- no FAST VoxelMapManager geometry authority;
- no defensive-programming bloat.

## Files / Commits
- changed files;
- commits;
- final HEAD;
- clean/push status.

## Final State

If all closes:

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

If I5 has unresolved semantic ambiguity:

```text
I5 = OPEN
I6 = BLOCKED
```

---

# 35. CLOSE criteria

Prompt 7 closes only if:

```text
I4:
  body_var frame fixed
  var_nostate / var distinguished correctly
  pose covariance used only for FAST visual point covariance
  current accepted QR normal propagated
  real consumer seam exercised
  LIO unchanged
  I4 Owner Verified

I5:
  visual plane requirements source-audited
  one read-only provider API
  same authoritative Prob OctVox
  same Super HKNN
  same Super QR
  center semantics explicit
  radius semantics source-grounded or absent
  P3 exposed only as native 4×4 [n,d] if needed
  no fake 6×6 plane covariance
  no map mutation
  no second map
  no camera/VIO runtime
  focused tests/build PASS

worktree clean
fast-forward push complete
```

The goal is to make the future visual layer consume Prob-LIO geometry without recreating or weakening the geometry semantics.
