# Prob-LIVO Integration — Prompt 6
## Final I3 Numeric Attribution Closure + I4 pointWithVar-Compatible Current-Scan Adapter

> Two strictly ordered parts:
>
> **Part A:** close the final I3 numeric-attribution question with a tiny bounded initialization experiment.
>
> **Part B:** only after Part A closes, implement I4: a clean current-scan adapter compatible with FAST-LIVO2 `pointWithVar`.
>
> Do not start I5 plane-provider work.
> Do not enable camera/VIO.
> Do not reopen P5.

---

# 0. Current state

```text
I0 = CLOSED / OWNER VERIFIED
I1 = CLOSED / OWNER VERIFIED
I2 = CLOSED / OWNER VERIFIED

I3:
  backend functional                  = GREEN
  Super-input migration control       = NEAR PARITY
  legacy live oracle                  = EXACT
  migrated execution reproducible     = GREEN
  first divergence                    = initialization numeric path
  remaining attribution granularity   = tiny bounded check
  Owner Verified                      = pending

I4 = NOT STARTED
I5–I8 = NOT STARTED
```

Current repo:

```text
~/super_livo/src/FAST-LIVO2
branch: prob-livo
expected start HEAD:
f85c7d0a9b7f7b62d19656d7c4f48aa003401238
```

Reference:

```text
~/super_livo/ref/Super-LIO
HEAD:
9fc949f46291c0fa76e5b7cdb372c940eb4b3f6e
```

Historical canonical legacy SHA:

```text
621acbd8d9a67634d3782fe8ab56e8a49ec821a9
```

Owner-authorized legacy workspace:

```text
~/prob_lio/src/Super-LIO
```

Reminder: this workspace was renamed by Owner, so old build/devel absolute paths may be stale.

---

# 1. Production philosophy

Owner does not want defensive-programming bloat.

```text
tests/diagnostics may be strict
production path must remain lean
```

Do not add:
- generic validators everywhere;
- repeated expensive checks;
- per-point/per-neighbor logs;
- fallback algorithms;
- duplicate filter/map ownership.

I4 is an adapter, not a second estimator.

---

# 2. Startup gate

Verify:

```bash
cd ~/super_livo/src/FAST-LIVO2
git status --short
git branch -vv
git rev-parse HEAD
git rev-parse origin/prob-livo
git log --oneline -15
```

Run current build and focused I1/I2/I3/P4/P5 tests.

### G-P6.0

Require:
- exact start HEAD;
- clean tree;
- build PASS;
- focused tests PASS.

Register prompt:

```text
prompts/prob_livo/prompt6_i3_numeric_close_i4_pointwithvar_adapter.md
```

---

# PART A — FINAL I3 NUMERIC ATTRIBUTION

# 3. Purpose

Prompt 5 established:
- input/timestamps/rows/GT subset match;
- first difference appears before map/HKNN/TBB;
- legacy ESKF/init is float;
- migrated shared state/ESKF/init is double;
- float-init control moves toward legacy.

But it did not fully separate:

```text
A. float vs double scalar width
B. legacy vs migrated mean recurrence operation order
C. float vs double SO(3) initialization precision
```

Close only this question.

No full-bag rerun unless a semantic mismatch is unexpectedly found.

---

# 4. 58-IMU bounded fixture

Use the exact same first 58 initialization IMU samples used in Prompt 5.

Compute four source-faithful variants:

```text
M1 = float  + legacy recurrence
M2 = float  + migrated recurrence
M3 = double + legacy recurrence
M4 = double + migrated recurrence
```

Legacy recurrence:

```text
mean += (sample - mean) / n
```

Migrated recurrence:

```text
mean = (mean * count + sample) / (count + 1)
```

Do not algebraically rewrite them into one implementation.

Compare:
- mean_acc;
- mean_gyro;
- imu_scale.

---

# 5. SO(3) precision split

Using identical physical inputs, compare:

```text
R_float  = source-faithful legacy float gravity alignment/yaw handling
R_double = migrated double gravity alignment/yaw handling
```

Also compare:
- initial gravity;
- initial bg;
- initial P18.

Report rotation delta as:

```text
Log(R_float^T R_double)
```

or equivalent.

---

# 6. HARD GATE G-I3.N1 — numeric decomposition

Report:

```text
scalar-width contribution
recurrence-order contribution
SO3 precision contribution
```

Do not claim a sole cause unless measured.

---

# 7. HARD GATE G-I3.N2 — semantic invariance

Prove all variants use:
- identical samples;
- identical mathematical initialization intent;
- same gravity/bias definitions;
- same covariance semantics.

If only finite-precision/operation-order differences remain, classify:

```text
NUMERIC_IMPLEMENTATION_DIFFERENCE_CONFIRMED
```

If a real equation/ownership semantic mismatch appears:
- stop;
- do not start I4;
- fix that first.

---

# 8. I3 closure

If G-I3.N1/N2 pass:

```text
I3 = CLOSED / OWNER VERIFIED
```

Do not downgrade production back to float.

Document:

```text
legacy = float ESKF/init numeric implementation
FAST host = double shared-state/ESKF/init numeric implementation
remaining difference = finite precision / operation ordering
semantics = equivalent
```

Suggested commit:

```text
test(prob-livo): close initialization numeric attribution
```

Clean tree before Part B.

---

# PART B — I4 pointWithVar CURRENT-SCAN ADAPTER

# 9. I4 mission

I4 does not enable camera/VIO.

Target flow:

```text
FAST scheduler
→ ProbImuAdapter
→ ProbESKF19
→ current undistorted scan
→ P1 current-point covariance
→ I4 adapter
→ FAST-compatible pointWithVar current-scan buffer
```

I4 prepares the current scan for later visual integration.

It does not provide planes. I5 will do that.

---

# 10. Source audit before coding

Audit exact FAST-LIVO2 source definition/usages of:

```text
pointWithVar
```

Find:
- struct fields;
- point frame expected by consumers;
- covariance frame;
- normal semantics;
- whether fields are mutated;
- whether consumers expect pre- or post-LIO geometry;
- relation to `feat_map`;
- first real production consumer.

Inspect `vio.cpp` and only the necessary preparation paths.

Do not rewrite visual code.

Produce an interface contract table before implementation.

---

# 11. Critical distinction

Remember:

```text
pointWithVar contains point/covariance/normal-related data
plane covariance is separate
```

Do not fabricate FAST plane covariance in I4.

Do not implement I5.

---

# 12. Authoritative current-scan source

The adapter must consume the Prob production scan:

```text
ProbImuAdapter scan-end IMU/body-frame undistorted point
+
canonical P1 current sensor-point covariance
```

Use stable source identity metadata as needed.

Do not:
- call FAST `Process2` again;
- undistort twice;
- recompute FAST-native sensor covariance;
- query FAST VoxelMapManager as LIO authority.

---

# 13. Freeze output frame

Audit actual consumer expectations first.

Preferred if source-compatible:

```text
pointWithVar.point = current point in world frame using shared x19 pose
pointWithVar.cov   = R_WI Sigma_I R_WI^T
```

If a body-frame view is needed, expose it clearly and separately.

Do not overload one field with ambiguous frame semantics.

Document exact formulas.

---

# 14. Covariance rule

P1 is sensor-point uncertainty.

Allowed world rotation:

```text
Sigma_W_sensor = R_WI * Sigma_I * R_WI^T
```

Forbidden in I4:

```text
J_pose P_pose J_pose^T
```

Do not introduce current pose covariance/P5 semantics.

---

# 15. Normal semantics

Audit what `pointWithVar.normal` actually means in FAST source.

Do not invent a normal.

If semantically valid normal/plane data is unavailable until I5:
- keep capability explicitly unavailable;
- separate point/covariance validity from normal validity;
- do not mark fake zero/default values as valid.

---

# 16. Stable identity/order

Preserve enough identity to trace each adapted point to:
- scan;
- source point index/deterministic ID;
- downsampled identity if adaptation occurs after `VoxelGridClosest`.

Choose the stage based on actual FAST visual semantics.

Do not independently sort/reindex without source justification.

---

# 17. Ownership

Recommended small adapter:

```text
ProbPointWithVarAdapter
```

It owns only current-scan temporary adapter storage.

It must not own:
- filter;
- P19;
- OctVox;
- plane provider;
- feat_map;
- trajectory.

---

# 18. HARD GATE G-I4.1 — field contract

Document every output field:

```text
field | meaning | frame | unit | producer | lifetime | consumer | validity
```

No ambiguous frame/covariance/normal semantics.

---

# 19. HARD GATE G-I4.2 — point coordinate parity

Deterministic fixtures:
- identity pose;
- translation;
- rotation;
- coupled SE(3);
- nonidentity LiDAR→IMU already handled upstream;
- start/mid/end undistorted points.

Negative mutations must fail:
- treating IMU-frame point as LiDAR-frame;
- applying extrinsic twice;
- wrong rotation direction;
- using wrong state epoch.

---

# 20. HARD GATE G-I4.3 — covariance parity

If IMU frame:

```text
Sigma_out = Sigma_I
```

If world frame:

```text
Sigma_out = R_WI Sigma_I R_WI^T
```

Compare full 3×3.

Negative mutations:
- `R^T Sigma R`;
- double rotation;
- adding pose covariance;
- FAST-native covariance substituted for P1.

---

# 21. HARD GATE G-I4.4 — source identity/order

Require:
- output count = authoritative input stage count;
- source IDs preserved;
- order preserved unless FAST source semantics explicitly require otherwise;
- intensity/time metadata not corrupted.

Independent sorting/reindex mutation must fail.

---

# 22. HARD GATE G-I4.5 — no hidden P5 / duplicate geometry

Prove adapter does not:
- query FAST VoxelMapManager for LIO geometry;
- query a second LiDAR map;
- add current pose covariance;
- perform plane association;
- perform P5 gating;
- update feat_map;
- run VIO.

---

# 23. HARD GATE G-I4.6 — real consumer-facing compatibility

Exercise the real FAST consumer-facing seam without enabling VIO.

Preferred:
- use the exact host type/functions;
- feed adapted points through the first real preparation seam that accepts `pointWithVar`.

If full VIO is required to invoke that seam, extract/use a helper that production also uses.

Do not create a test-only fake API unrelated to production.

---

# 24. HARD GATE G-I4.7 — dormant runtime integration

Wire the adapter so camera-OFF Prob runtime can produce it.

Lightweight counters only:

```text
adapted_scans
adapted_points
```

No visual consumer activated.

No per-point logs.

---

# 25. HARD GATE G-I4.8 — LIO invariance

Adapter ON must not alter LIO estimation.

Prove ON vs OFF:
- same rows;
- same state/map result;
- same trajectory hash if deterministic, otherwise binary64-equivalent;
- same matched GT/ATE if a runtime run is used.

If merely constructing the adapter changes LIO, it is a bug.

A full eee_01 rerun is optional if focused production seam tests prove observational behavior; do not waste a bag run without need.

---

# 26. Recommended files

Suggested:

```text
include/prob_livo/prob_point_with_var_adapter.h
src/prob_livo/prob_point_with_var_adapter.cpp
tests/prob_livo/test_i4_point_with_var_adapter.cpp
```

Reuse authoritative FAST `pointWithVar` type. Do not duplicate it unless unavoidable.

---

# 27. Build/tests

After implementation:
- build host;
- rerun I1/I2/I3/P4/P5 closure tests;
- run I4 focused tests;
- run production-seam compatibility test.

No visual bag run.

---

# 28. Commit sequence

Suggested:

```text
test(prob-livo): close initialization numeric attribution
feat(prob-livo): add pointWithVar current-scan adapter
test(prob-livo): close pointWithVar adapter semantics
docs(prob-livo): close I4 adapter evidence
```

No force push.

---

# 29. State transition

After Part A:

```text
I3 = CLOSED / OWNER VERIFIED
I4 = ACTIVE
```

If I4 gates all pass:

```text
I4 = CLOSED/PASS — Owner audit pending
I5 = NOT STARTED
```

Agent must not mark I4 Owner Verified.

---

# 30. Final report

## Agent State Consensus
- start/final HEAD
- branch/origin
- clean state
- prompt registration

# PART A — I3 Numeric Closure

## 58-IMU Fixture
- exact provenance

## M1–M4
Report:
- mean_acc
- mean_gyro
- imu_scale
- delta to legacy

## SO(3) Split
- float result
- double result
- rotation delta
- gravity/bg/P18 delta

## Attribution
Separate:
- scalar-width contribution
- recurrence-order contribution
- SO3 contribution

## G-I3.N1 / G-I3.N2
PASS/FAIL.

## I3 Decision
If passed:

```text
I3 = CLOSED / OWNER VERIFIED
classification = NUMERIC_IMPLEMENTATION_DIFFERENCE_CONFIRMED
```

# PART B — I4

## FAST pointWithVar Source Audit
- struct
- consumers
- frame/covariance/normal semantics

## Adapter Contract
Full field table.

## Implementation
- files
- input source
- output frame
- covariance transform
- identity ownership

## Gates
Report independently:

```text
G-I4.1 field contract
G-I4.2 coordinate parity
G-I4.3 covariance parity
G-I4.4 identity/order
G-I4.5 no P5/duplicate map
G-I4.6 real consumer compatibility
G-I4.7 dormant runtime integration
G-I4.8 LIO invariance
```

## Build/Test
Commands and RCs.

## Scope Audit
Confirm:
- camera/VIO OFF;
- I5 not started;
- no plane-provider logic;
- no P5;
- no second geometry map;
- no defensive-programming bloat.

## Files/Commits
- changed files
- commits
- final HEAD
- clean/push state

## Final State

If complete:

```text
I0 = CLOSED / OWNER VERIFIED
I1 = CLOSED / OWNER VERIFIED
I2 = CLOSED / OWNER VERIFIED
I3 = CLOSED / OWNER VERIFIED
I4 = CLOSED/PASS — Owner audit pending
I5–I8 = NOT STARTED

pointWithVar-compatible current-scan adapter = operational
camera/VIO runtime                            = OFF

Next stage = I5 ProbPlaneProvider
```

If I4 remains ambiguous:

```text
I4 = OPEN
I5 = BLOCKED
```

---

# 31. CLOSE criteria

Prompt 6 closes only if:

```text
I3 numeric decomposition completed
no semantic migration mismatch remains
I3 Owner Verified

I4 field/frame/covariance contract explicit
source identity preserved
real production seam exercised
no P5
no duplicate map authority
no visual runtime
adapter ON does not alter LIO
focused tests/build PASS

worktree clean
fast-forward push complete
```

The purpose of I4 is interface compatibility, not new estimation logic.
