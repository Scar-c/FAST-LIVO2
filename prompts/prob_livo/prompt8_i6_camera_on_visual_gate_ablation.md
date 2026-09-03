# Prob-LIVO Integration — Prompt 8
## I6 Camera-ON FAST Visual Closure + Visual Plane-Gate Ablation
## Downsample Ablation CANCELLED — Super VoxelGridClosest Remains Frozen

> Goals:
> 1. complete I6 camera-ON FAST-LIVO2 visual sequential closure on top of verified Prob-LIO;
> 2. run real full-bag eee_01 camera-ON experiments;
> 3. move the visual-gate ablation from old I7 into I6;
> 4. cancel the downsample ablation permanently for this phase.
>
> Do not enable any FAST/LIVO2/PCL downsample comparison.
> Do not re-enable LiDAR P5.
> Do not tune by ATE.

---

# 0. Owner decisions / roadmap update

Current accepted state:

```text
I0 = CLOSED / OWNER VERIFIED
I1 = CLOSED / OWNER VERIFIED
I2 = CLOSED / OWNER VERIFIED
I3 = CLOSED / OWNER VERIFIED
I4 = CLOSED / OWNER VERIFIED
I5 = CLOSED / OWNER VERIFIED

I6 = ACTIVE
I7 visual-gate ablation = MOVED INTO I6
I7 downsample ablation  = CANCELLED
I8 = NOT STARTED
```

Canonical LiDAR downsample is frozen:

```text
Super VoxelGridClosest
```

The only visual-plane-gate ablation authorized:

```text
A. FAST-LIVO2 visual probabilistic 3σ plane-consistency gate
B. Super legacy deterministic point-to-plane gate
```

For both A and B:

```text
the 3×radius local-support gate stays ON and identical
```

Only the second plane-consistency gate is ablated.

---

# 1. Repository / frontier consensus

Active repo:

```text
~/super_livo/src/FAST-LIVO2
branch: prob-livo
expected start HEAD:
c4612b55b6e336da029597b7295f2094f13276f6
```

Prob-LIO reference:

```text
~/super_livo/ref/Super-LIO
HEAD:
9fc949f46291c0fa76e5b7cdb372c940eb4b3f6e
```

Legacy Super gate source may be audited from:

```text
~/prob_lio/src/Super-LIO
```

Reminder: legacy workspace was renamed; stale build/devel absolute paths may exist.

Verify HEAD/branch/origin/status before work and register this prompt at:

```text
prompts/prob_livo/prompt8_i6_camera_on_visual_gate_ablation.md
```

---

# 2. Baseline gate

Before edits:
- worktree clean;
- full required build PASS;
- I1–I5 focused tests PASS;
- no unexpected frontier divergence.

### HARD GATE G-P8.0

Stop on baseline regression.

---

# 3. Production philosophy

Owner does not want defensive-programming bloat.

Use:

```text
strict tests
lean production
```

No:
- per-point/per-pixel log flood;
- generic validator layers;
- duplicate state/map;
- fallback geometry;
- hidden tuning;
- alternate estimator.

Lightweight counters are allowed.

---

# PART A — CAMERA-ON VISUAL ARCHITECTURE CLOSURE

# 4. Architecture

Canonical I6:

```text
FAST ROS / scheduler / camera timing
        ↓
shared x19 / P19
        ↓
Prob IMU + Prob-LIO P0–P4
        ↓
I4 pointWithVar current-scan adapter
        ↓
I5 ProbPlaneProvider
        ↓
FAST-LIVO2 visual pipeline
        ↓
same shared x19 / P19
```

There must remain:

```text
one state/covariance
one Prob OctVox LiDAR geometry map
one FAST visual map (feat_map / VisualPoint)
```

No pose-copy bridge.
No second filter.
No FAST LiDAR geometry authority in Prob mode.

---

# 5. Audit exact FAST visual sequence before wiring

Audit current source ordering for:
- camera epoch scheduling;
- LiDAR posterior;
- pointWithVar preparation;
- VisualPoint creation;
- raycast/plane lookup;
- reference patch update;
- pyramid/coarse-to-fine;
- photometric residual;
- inverse exposure;
- relinearization/rollback;
- covariance/state commit.

Record exact functions and mutation ownership.

Do not rewrite `vio.cpp` wholesale.
FAST public source is the visual semantic authority.

---

# 6. Shared state authority

Visual update must operate directly on the same `StatesGroup x19/P19`.

Prob LIO keeps Super-semantic retraction.
FAST visual keeps its source visual update semantics.

### HARD GATE G-I6.1 — one-state sequential ownership

Prove:

```text
Prob LiDAR posterior
→ FAST visual update
→ same x19/P19
→ next epoch consumes that posterior
```

Negative mutation: temporary second visual filter / pose-copy bridge must fail.

---

# 7. FAST camera scheduler remains authority

Preserve FAST:
- camera-cut packetization;
- current/next curvature rebasing;
- IMU ownership/lookahead;
- scan recombination;
- image cadence.

### HARD GATE G-I6.2

Use a real/extracted production-used camera-cut helper to prove:
- no point loss/duplication;
- no double IMU integration;
- current/next timing correct.

Do not rely only on hand-built final packets.

---

# 8. Wire I4 current-scan input

In Prob visual mode, source current `pointWithVar` from I4.

Frozen semantics:

```text
point_b       = LiDAR frame
body_var      = Sigma_L
point_i       = IMU frame
point_w       = world frame
var_nostate   = sensor-only world covariance
var           = FAST visual-compatible full covariance
normal        = accepted current Super QR normal
```

Rejected current points keep invalid/zero normal and follow FAST skip semantics.

Do not recompute current normals through I5.

### HARD GATE G-I6.3

At a real camera epoch:
- accepted points reach real visual candidate prep;
- IDs/order preserved;
- rejected points skipped;
- FAST VoxelMapManager not queried.

---

# 9. Wire I5 plane provider

Replace visual arbitrary plane lookup with:

```text
ProbPlaneProvider
→ same Prob OctVox
→ same Super HKNN
→ same Super QR
```

Use for:
- raycast;
- reference-patch local plane query;
- other visual arbitrary world-point plane needs.

Do not redo I4 current correspondence normals.

### HARD GATE G-I6.4

Prove:
- visual plane queries call I5;
- FAST VoxelMapManager plane lookup count = 0;
- no second LiDAR geometry map exists.

---

# 10. Geometry-valid vs uncertainty-valid

Audit visual consumers and distinguish, if required:

```text
geometry_valid
uncertainty_valid
```

A geometry-only raycast/homography path must not be rejected only because P3 covariance is unavailable if source semantics do not require uncertainty.

Probabilistic 3σ gate does require valid covariance.

No fake covariance fallback.

### HARD GATE G-I6.5

Fixtures must prove correct consumer-specific validity handling.

---

# PART B — VISUAL PLANE-GATE ABLATION

# 11. Preserve first gate: 3×radius

Both A/B retain the exact FAST source local-support condition involving `3 * radius`.

This is not the ablated variable.

Counters:

```text
plane_query_valid
radius_gate_pass
radius_gate_reject
```

---

# 12. Variant A — FAST/LIVO2 probabilistic 3σ gate

Preserve the original visual probability semantics using Prob-native plane uncertainty.

For:

```text
r = n^T p_W + d
plane parameter = [n_x,n_y,n_z,d]
J_nd = [p_W^T, 1]
```

compute:

```text
sigma_plane² = J_nd Sigma_nd J_nd^T
sigma_point² = n^T Sigma_VP n
sigma_total² = sigma_plane² + sigma_point²
```

accept according to the source-equivalent threshold:

```text
|r| <or<= 3 * sqrt(sigma_total²)
```

Audit exact inequality.

`Sigma_VP` must be the correct FAST VisualPoint covariance.

Do not fabricate old FAST 6×6 `[normal,center]` plane covariance.

### HARD GATE G-I6.GA1

Independent oracle cases:
- clearly inside;
- threshold-near;
- outside;
- plane variance dominant;
- VisualPoint variance dominant;
- invalid covariance.

Negative mutations:
- omit VisualPoint covariance;
- use wrong covariance field;
- fake 6×6;
- call LiDAR P5 code.

---

# 13. Variant B — Super legacy deterministic visual gate

Audit the exact canonical Super legacy LiDAR point-to-plane acceptance formula from source.

Do not rely on memory.

Map the exact required physical quantities into the visual/reference-patch context.

If the formula requires sensor-relative range:
- transform the world VisualPoint through current shared pose/extrinsic;
- use the correct current LiDAR/body frame quantity;
- never use world-origin norm.

If direct source-faithful application is genuinely ambiguous, STOP and report instead of inventing a “legacy-like” rule.

The 3×radius support gate remains unchanged.

### HARD GATE G-I6.GB1

Require:
- exact source formula;
- exact frame/range variable;
- direct legacy-oracle fixture parity;
- visual world-point → sensor-relative transform test.

Negative mutations:
- world norm;
- covariance retained in deterministic gate;
- P4 weight used as gate;
- radius gate disabled.

---

# 14. Single ablation switch

Implement one visual-only switch:

```text
prob_livo/visual_plane_gate:
  livo2_prob_3sigma
  super_legacy
```

It affects only the second visual point-plane consistency gate.

Must not change:
- LiDAR association/P4;
- I4 covariance;
- I5 fitting;
- radius gate;
- downsample;
- pyramid;
- exposure;
- photometric residual.

### HARD GATE G-I6.G2

Prove H1/H2 are identical before the second gate and only the second gate decision differs.

---

# PART C — FAST VISUAL SEMANTIC CLOSURE

# 15. Preserve FAST visual core

Keep source semantics for:
- VisualPoint creation;
- patch extraction;
- exposure-aware residual;
- inverse exposure state;
- pyramid;
- coarse→fine;
- relinearization;
- rollback;
- covariance commit;
- reference patch update.

Only adapt:
- I4 input seam;
- I5 plane seam;
- second visual plane gate.

---

# 16. Exposure

Audit shared x19 exposure index usage.

### HARD GATE G-I6.6

Prove:
- visual update can observe/update exposure as FAST source intends;
- LiDAR update does not directly observe exposure;
- P19 cross-covariance remains on same shared matrix.

---

# 17. Pyramid / relinearization / commit

### HARD GATE G-I6.7

Bounded fixture proves:
- source pyramid order;
- source relinearization;
- source rollback;
- source-equivalent final state/covariance commit.

---

# 18. Reference patch update

Use I5 normal/d/center/radius.

For A:
- use native Prob 4×4 `[n,d]` covariance.

For B:
- second gate deterministic; covariance may remain available for other legitimate visual bookkeeping.

### HARD GATE G-I6.8

Fixture:
- radius pass/reject;
- A pass/reject oracle;
- B pass/reject oracle;
- accepted patch update modifies only source-defined data;
- rejected update leaves reference unchanged.

---

# PART D — REAL CAMERA-ON eee_01 RUNS

# 19. Three fixed runtime variants

To isolate camera scheduling from visual estimation:

```text
H0 = camera scheduler ON + image flow ON + visual state update OFF
H1 = camera ON + visual ON + livo2_prob_3sigma
H2 = camera ON + visual ON + super_legacy
```

H0 is the camera-scheduler control.

All variants use:
- same Super-input LiDAR semantics;
- same Prob P0–P4;
- same Super VoxelGridClosest;
- same I4/I5;
- same visual config except explicit visual enable/gate mode.

No tuning between H0/H1/H2.

---

# 20. Runtime counters

Lightweight counters only:

```text
camera_epochs
images_received
visual_process_calls

current_scan_candidates
current_scan_normal_valid

plane_queries
plane_geometry_valid
plane_uncertainty_valid

radius_gate_pass
radius_gate_reject

second_gate_pass
second_gate_reject

visual_points_created
reference_patch_update_attempts
reference_patch_updates_accepted

photometric_update_attempts
photometric_updates_accepted

visual_state_commits
visual_rollbacks
```

---

# 21. HARD GATE G-I6.9 — visual actually active

H1/H2 require:
- visual calls > 0;
- valid normals > 0;
- plane queries > 0;
- VisualPoints created > 0;
- patch or photometric attempts > 0;
- visual state commits > 0 if source/data permits.

If not:

```text
VISUAL_INACTIVE_FAIL
```

Do not tune thresholds to force activity.

---

# 22. Dataset/config

Use:

```text
~/super_livo/bag/NTU/eee_01/eee_01.bag
```

Use same official NTU evaluator provenance.

Freeze FAST visual config.

No sweep of:
- patch size;
- pyramid;
- exposure;
- photometric thresholds;
- visual noise;
- gate threshold.

Record effective config hashes.

---

# 23. Full-bag run order

Clean committed source only:

```text
1. H0
2. H1
3. H2
```

One bounded experiment at a time.
No concurrent rosbag/mapper.

Use repository-owned offline runner only if camera/image scheduler and visual runtime semantics are production-equivalent; otherwise use bounded online runtime.

---

# 24. HARD GATE G-I6.10 — H0

Report:
- completion;
- rows;
- matched GT;
- ATE;
- trajectory hash;
- camera counters;
- `visual_state_commits = 0`.

Do not interpret H0 vs old ONLY_LIO as visual effect.

---

# 25. HARD GATE G-I6.11 — H1

Configuration:

```text
camera ON
visual ON
second gate = livo2_prob_3sigma
radius gate unchanged
downsample = Super VoxelGridClosest
```

Require full completion, active visual, finite trajectory, evaluator result and counters.

Classification:
- VISUAL_ACTIVE_VALID
- VISUAL_INACTIVE_FAIL
- ALGORITHM_DIVERGED
- EXECUTION_INVALID

---

# 26. HARD GATE G-I6.12 — H2

Same as H1 except:

```text
second visual gate = super_legacy
```

Require equivalent evidence.

---

# 27. Gate-ablation comparison

Compare H1 vs H2:

```text
ATE
rows
matched GT
visual active epochs
second gate pass/reject
VisualPoints created
reference updates accepted
photometric updates accepted
visual state commits
rollbacks
runtime
```

Also compare each to H0.

### HARD GATE G-I6.13

Require:
- same bag/hash;
- same source HEAD;
- same scheduler/input/downsample;
- same radius gate;
- same visual config;
- only second gate differs.

---

# 28. NO downsample ablation

Hard Owner decision:

```text
Super VoxelGridClosest only
```

for H0/H1/H2.

Forbidden:
- FAST/PCL centroid downsample;
- LIVO2 native downsample A/B;
- hidden per-mode downsample change.

### HARD GATE G-I6.14

Prove all variants use the same Super downsample.

This supersedes old I7 downsample plans.

---

# 29. LiDAR P5 remains OFF

For all variants:

```text
LiDAR association = Super legacy
LiDAR weighting   = P4
LiDAR P5          = OFF
```

The H1 3σ gate is visual/reference-patch logic only.

### HARD GATE G-I6.15

Prove visual gate switch has no effect on LiDAR association code path.

---

# 30. Shared-state safety

Because visual now modifies shared x19/P19, bounded diagnostics should audit:
- gravity norm before/after visual;
- exposure state;
- pose/velocity/bias/gravity blocks;
- covariance symmetry/finiteness.

Do not add heavy production validators.

### HARD GATE G-I6.16

Prove visual update preserves the frozen state index/ABI contract.

---

# 31. Recommended tests

```text
test_i6_camera_scheduler
test_i6_visual_input_seam
test_i6_plane_query_seam
test_i6_visual_prob_gate
test_i6_visual_legacy_gate
test_i6_visual_gate_ablation
test_i6_visual_state_update
test_i6_reference_patch
```

No `std::vector<bool>`.

---

# 32. Commit order

Suggested:

```text
feat(prob-livo): wire Prob geometry into FAST visual pipeline
feat(prob-livo): add visual plane-gate ablation modes
test(prob-livo): close camera-on visual semantics
docs(prob-livo): record eee01 visual gate ablation
```

No force push.
Canonical runs from clean committed source.

---

# 33. SPEC / roadmap

Update SPEC/EVIDENCE/HISTORY:

```text
I5 = CLOSED / OWNER VERIFIED

I6:
  camera-on FAST visual integration
  includes visual plane-gate ablation

I7:
  visual-gate ablation moved to I6
  downsample ablation cancelled
  Super VoxelGridClosest frozen
```

Do not mark I6 Owner Verified yourself.

---

# 34. Final report

## Agent State Consensus
- start/final HEAD
- branch/origin
- clean state
- prompt registration

## Source Audit
- FAST visual sequence
- pointWithVar seam
- plane query consumers
- reference patch
- exposure
- pyramid/relinearization
- commit/rollback

## Architecture Wiring
- shared x19/P19
- I4 input
- I5 provider
- FAST visual map
- no FAST LiDAR map

## Visual Gates

### Radius Gate
- exact source formula
- proof unchanged A/B

### A — LIVO2 Probabilistic 3σ
- residual
- J_nd
- plane variance
- VisualPoint variance
- final threshold

### B — Super Legacy
- exact source formula
- exact frame/range variable
- transform
- final threshold

## Gate Tests
Report:
- G-I6.GA1
- G-I6.GB1
- G-I6.G2
- G-I6.1..G-I6.9

## Dataset / Effective Config
- bag/hash
- topics
- camera/visual config
- downsample proof
- gate modes

## H0
- completion
- rows/matched/ATE
- hash
- counters

## H1
- classification
- rows/matched/ATE
- hash
- counters

## H2
- classification
- rows/matched/ATE
- hash
- counters

## Ablation Table
Columns:

```text
variant
ATE
rows
matched GT
camera epochs
visual calls
plane valid
radius pass
second-gate pass
VisualPoints created
reference updates accepted
photometric updates accepted
visual commits
rollbacks
runtime
```

## G-I6.10–G-I6.16
Independent PASS/FAIL.

## Scope Audit
Confirm:
- Super VoxelGridClosest only;
- no downsample ablation;
- no LiDAR P5;
- no second map/filter;
- no tuning;
- no I8 yet.

## Build/Test
- commands/RCs/check counts

## Files / Commits
- changed files
- commits
- final HEAD
- clean/push

## Final State

If all valid:

```text
I0–I5 = CLOSED / OWNER VERIFIED
I6 = CLOSED/PASS — Owner audit pending
I7 visual-gate ablation = completed inside I6
I7 downsample ablation  = CANCELLED
I8 = NOT STARTED

camera-on FAST visual integration = operational
visual plane-gate ablation        = complete
Super VoxelGridClosest            = frozen canonical downsample
```

If visual inactive or semantic seam unresolved:

```text
I6 = OPEN
I8 = BLOCKED
```

---

# 35. CLOSE criteria

Prompt 8 closes only if:

```text
I4/I5 are the only Prob geometry sources
FAST visual core remains source-semantic authority
shared x19/P19 used sequentially
camera scheduler seam proven

radius gate preserved
A probabilistic 3σ gate uses Prob 4×4 [n,d] + VisualPoint covariance
B Super legacy gate is source-faithful
A/B differ only in the second visual gate

Super VoxelGridClosest fixed everywhere
no downsample ablation
LiDAR P5 remains excluded

H0 valid
H1 valid and visual active
H2 valid and visual active
same evaluator / same source / same config except intended switches
no tuning

worktree clean
fast-forward push complete
```

Scientific question:

```text
Does uncertainty-aware FAST-LIVO2 visual plane consistency
improve visual map/update behavior versus a deterministic
Super legacy point-to-plane gate when every other component is fixed?
```
