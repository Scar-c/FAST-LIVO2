# Prob-LIVO Integration — Prompt 5
## I3 Near-Parity Divergence Attribution / Find the First Real Difference vs Legacy Prob-LIO

> This round does **not** start I4.
>
> Goal: explain the remaining difference between the legacy Super-host Prob-P4 and the FAST-host + Super-input Prob-P4, find the first production-semantic divergence, and decide whether it is a real semantic mismatch or an acceptable numerical/discrete implementation difference.
>
> No parameter tuning. No visual/I4 work.

---

# 0. Current state

```text
I0 = CLOSED / OWNER VERIFIED
I1 = CLOSED / OWNER VERIFIED
I2 = CLOSED / OWNER VERIFIED

I3:
  backend functional              = GREEN
  Super-input control             = NEAR PARITY
  rows / GT matches / timestamps  = exact
  remaining trajectory difference = UNATTRIBUTED
  Owner Verified                  = NO

I4–I8 = NOT STARTED
```

Reference results:

```text
Legacy Super-host Prob-P4
rows        = 3981
matched GT  = 3329
ATE         = 0.08883155405698266 m

FAST-host + Super-input Prob-P4
rows        = 3981
matched GT  = 3329
ATE         = 0.090995748 m

raw translation RMSE ≈ 0.033195 m
raw rotation RMSE    ≈ 0.001943 rad
timestamps           = exact
```

The goal is not bitwise equality. Find the first causal difference.

---

# 1. Current integration repo

```text
~/super_livo/src/FAST-LIVO2
branch: prob-livo
expected start HEAD:
fb159ca2848a3fe7ea87db314a905dd98b01de12
```

Current read-only reference:

```text
~/super_livo/ref/Super-LIO
expected HEAD:
9fc949f46291c0fa76e5b7cdb372c940eb4b3f6e
```

Verify branch/HEAD/status/origin before work. No force push/history rewrite.

Register this prompt at:

```text
prompts/prob_livo/prompt5_i3_divergence_attribution.md
```

---

# 2. Owner-authorized original Prob-LIO workspace

Owner explicitly allows use of:

```text
~/prob_lio/src/
├── CMakeLists.txt -> /opt/ros/noetic/share/catkin/cmake/toplevel.cmake
└── Super-LIO
    ├── build
    ├── devel
    ├── docs
    ├── eval
    ├── prompts
    ├── README.md
    ├── results
    ├── spec
    ├── src
    ├── tests
    └── tools
```

Important:

```text
This original Prob-LIO workspace was renamed by Owner.
```

Therefore old `build/`, `devel/`, generated CMake metadata, scripts, and binaries may contain stale absolute paths from the previous workspace name.

Do not diagnose stale-path failures as algorithm failures.

Before using old binaries/build artifacts:
1. inspect provenance/path assumptions;
2. verify paths resolve now;
3. otherwise rebuild under the current path or use a detached diagnostic worktree.

Do not alter Owner's historical source/history merely to repair stale build paths.

---

# 3. Exact historical authority

Historical canonical eee_01 run:

```text
algorithm SHA:
621acbd8d9a67634d3782fe8ab56e8a49ec821a9

trajectory SHA256:
259d3fbc16e5b918a75d5517c4f5feac0b29e40b7c6d5464f881185704595199

rows:
3981

matched GT:
3329

ATE:
0.08883155405698266 m
```

This SHA, not just current `9fc949f`, is the historical executable authority.

Verify the commit exists locally and verify the associated config/evaluator/run provenance.

---

# 4. Protect historical workspace

Preferred:

```text
create a detached diagnostic git worktree at 621acbd...
```

e.g.:

```text
~/prob_lio_diag/legacy_621acbd/
```

Diagnostic instrumentation there is allowed.

Forbidden:
- commit/push diagnostic changes to legacy history;
- reset Owner's original workspace;
- modify historical result evidence.

At final report, show the original workspace remained untouched.

---

# 5. Rebuild legacy after rename

Do not trust legacy `build/` / `devel/` automatically.

Build the detached legacy worktree from source under its actual current path.

Record:
- compiler;
- Eigen/PCL;
- optimization flags;
- OpenMP/TBB/thread settings;
- ROS environment.

Do not normalize threading/config by guessing.

---

# 6. HARD GATE G-P5.0 — legacy live oracle

Before divergence tracing, reproduce the historical legacy result from SHA `621acbd...`.

Require evidence:

```text
rows
matched GT
timestamps
ATE
trajectory SHA
```

Classify:

```text
EXACT
NEAR_REPRODUCED
NOT_REPRODUCED
```

If NOT_REPRODUCED, stop numerical attribution and first explain legacy build/runtime/provenance differences.

A non-reproducible legacy execution is not a valid live oracle.

---

# 7. Current migrated oracle

Use current:

```text
FAST host
+ super_ntu_legacy input semantics
+ Prob-LIO P0–P4
+ camera OFF
```

Reference:

```text
rows        = 3981
matched GT  = 3329
ATE         = 0.090995748 m
```

Use current repository-owned offline runner when useful.

Do not modify config, P0–P4 parameters, input semantics, evaluator, or canonical thread setting during baseline reproduction.

---

# 8. Find the FIRST divergence

Do not start from final ATE.

Use sparse checkpoints / binary search over RUN epochs.

Example:

```text
RUN 0,1,2,4,8,16,...
```

or binary search until finding:

```text
last fully matching epoch
first divergent epoch
```

Then trace only the first divergent epoch in depth.

No full-bag per-point log flood.

---

# 9. Signature ladder

Compare the same epoch in this order.

## S0 scheduler / epoch
- scan start/end;
- raw LiDAR identity/count;
- IMU sample timestamps;
- lookahead timestamp;
- lifecycle state.

## S1 preprocessing
- point count;
- source point identities/order;
- XYZ/intensity;
- relative time;
- compact hash.

## S2 predict / undistortion
- predicted R,p,v,bg,ba,g;
- physical P18;
- undistorted point count/order/XYZ/hash.

## S3 VoxelGridClosest
- selected source-point identities;
- output count/order/hash.

## S4 P1
- sampled point covariances;
- aggregate signature.

## S5 HKNN
- exact neighbor identities/keys/order;
- distances.

## S6 QR
- support IDs;
- rank;
- q;
- n,d;
- residual.

## S7 P3/P4
- plane covariance;
- sensor variance;
- plane variance;
- R_i;
- weight;
- legacy accept/reject.

## S8 measurement system
- effective correspondence count;
- HT_Vinv_H;
- HT_Vinv_r;
- need_converge sequence.

## S9 ESKF posterior
- per-iteration state;
- final physical state;
- final P18.

## S10 map update
- inserted count;
- affected voxel/subvoxel keys;
- representative position/covariance;
- accepted N;
- compact map signature.

The first differing stage is the attribution target.

---

# 10. Lightweight diagnostics only

Owner does not want defensive-programming bloat.

Use:
- test-only trace hooks;
- offline diagnostic mode;
- selected-epoch tracing;
- compact binary/CSV/JSON evidence.

Do not add permanent per-point/per-neighbor log spam, heavy PSD checks, generic runtime validators, or fallback algorithms.

Diagnostics OFF by default.

---

# 11. Hash for bisection, identities for proof

Hashes may locate the mismatch.

After mismatch:
- emit real semantic identities;
- source point index;
- voxel/subvoxel key;
- HKNN neighbor key;
- QR support IDs;
- gate decision.

Do not conclude “floating point” from hashes alone.

---

# 12. Threading / TBB is a suspect, not an assumption

If the first difference occurs in a parallel stage, run bounded controlled diagnostics:

```text
A migrated canonical threading
B migrated single-thread/deterministic diagnostic for that stage
C legacy historical threading
```

Compare:
- identities;
- ordering;
- numeric deltas;
- branch flips.

Do not change canonical production threading unless a real semantic defect is proven.

---

# 13. Classification vocabulary

At the first divergence choose one:

```text
NUMERIC_ONLY
DISCRETE_BOUNDARY_FROM_NUMERIC
SEMANTIC_INPUT_DIFFERENCE
SEMANTIC_ALGORITHM_DIFFERENCE
MAP_ORDERING_DIFFERENCE
PARALLEL_ORDER_DIFFERENCE
UNKNOWN
```

`DISCRETE_BOUNDARY_FROM_NUMERIC` includes tiny numeric changes flipping:
- nearest/voxel choice;
- gate accept/reject;
- rank threshold;
- neighbor order;
- convergence threshold.

Do not call a branch flip harmless without proving its semantics.

---

# 14. Specific suspects to audit only as evidence directs

```text
SO(3) implementation
float vs double storage
Eigen QR details
OctVox insertion/update order
VoxelGridClosest tie behavior
HKNN equal-distance ordering
TBB/OMP ordering
measurement accumulation order
map update order
map-init insertion order
IEKF convergence comparison
normalization/epsilon
compiler/vectorization flags
```

Do not shotgun-change these.

---

# 15. HARD GATE G-P5.1 — first divergence localized

Final evidence must state:

```text
first divergent RUN epoch index:
timestamp:
last matching stage:
first divergent stage:
legacy value/identity:
migrated value/identity:
```

Final trajectory RMSE or “probably TBB” is insufficient.

---

# 16. HARD GATE G-P5.2 — causal minimal reproducer

Build a bounded fixture reproducing the exact first divergence.

Possible fixture:
- one voxel selection;
- one HKNN query;
- one QR support set;
- one map insertion;
- one IEKF update.

Show:
- legacy behavior;
- migrated behavior;
- control/negative mutation.

The fixture must establish causality, not correlation.

---

# 17. If semantic bug is found

If the difference is a real unintended migration semantic difference:
1. fix only the smallest seam;
2. add regression test;
3. rerun focused tests;
4. rerun Super-input eee_01;
5. compare rows/timestamps/trajectory/ATE.

No tuning.

Do not modify FAST-native semantics unless the same production bug truly applies.

---

# 18. If difference is numeric/discrete but semantics match

If proven:

```text
NUMERIC_ONLY
PARALLEL_ORDER_DIFFERENCE
DISCRETE_BOUNDARY_FROM_NUMERIC
```

and formulas/inputs/ownership are source-equivalent:

Do not chase bitwise equality.

Document:
- exact first divergence;
- causal numeric reason;
- whether a discrete branch flips;
- downstream effect;
- why it is acceptable.

This is enough to close I3 if reproducibility is stable.

---

# 19. Historical Prompt-3 FAST-native 0.052901597 is OUT OF SCOPE

Do not spend this round reproducing:

```text
0.052901597
3595 rows
3016 matches
```

unless the exact same causal issue directly affects the Super-input divergence.

Prompt 5 is only:

```text
legacy Super-host Prob-P4
vs
current FAST-host + Super-input Prob-P4
```

---

# 20. HARD GATE G-P5.3 — legacy reproducibility

Report:

```text
historical ATE / live ATE
historical rows / live rows
historical matched / live matched
historical trajectory SHA / live SHA
```

No live-oracle attribution without this gate.

---

# 21. HARD GATE G-P5.4 — migrated reproducibility

Repeat current Super-input offline run if needed.

Require stable:

```text
3981 rows
3329 matched GT
stable trajectory/ATE
```

This proves the target being diagnosed is deterministic enough.

---

# 22. Final decision

Choose exactly one:

```text
I3_DIVERGENCE_SEMANTIC_FIXED
I3_DIVERGENCE_NUMERIC_ACCEPTED
I3_DIVERGENCE_UNRESOLVED
```

If unresolved:

```text
I3 remains open
I4 remains blocked
```

---

# 23. Current repo commit policy

Do not commit diagnostic modifications to the legacy worktree.

Current repo suggested commits only if needed:

```text
test(prob-livo): localize legacy migration divergence
fix(prob-livo): close legacy migration semantic divergence
docs(prob-livo): record I3 divergence attribution
```

No force push.

---

# 24. SPEC / evidence

Update:

```text
spec/prob_livo/SPEC.md
spec/prob_livo/EVIDENCE_INDEX.md
spec/prob_livo/HISTORY.md
```

Record:
- legacy diagnostic worktree/path/SHA;
- renamed-workspace warning;
- live legacy reproduction;
- migrated reproduction;
- first divergent epoch/stage;
- causal classification;
- minimal reproducer;
- fix if any;
- final ATE if rerun.

Do not mark I3 Owner Verified yourself.

---

# 25. Final report

## Agent State Consensus
- start/final HEAD
- branch/origin
- clean status
- prompt registration

## Legacy Workspace Handling
- original workspace path
- rename implications
- detached diagnostic worktree
- exact SHA
- rebuild environment
- proof original workspace unchanged

## Legacy Live Oracle
- rows/matched/ATE/SHA
- EXACT/NEAR/NOT_REPRODUCED

## Current Migrated Reproduction
- rows/matched/ATE/SHA

## Search Method
- checkpoints/bisection
- stages compared

## First Divergence
```text
RUN epoch:
timestamp:
last equal stage:
first different stage:
legacy observable:
migrated observable:
```

## Causal Analysis
- classification
- exact source/functions
- mechanism

## Minimal Reproducer
- input
- legacy output
- migrated output
- control mutation

## Fix
If semantic: production patch + regression.
If numeric accepted: explain why no production change is needed.

## Final eee_01
If rerun:
- rows/matched/ATE
- strict trajectory comparison

## Gate Results
```text
G-P5.0
G-P5.1
G-P5.2
G-P5.3
G-P5.4
```

## Scope Audit
Confirm:
- no I4;
- no visual;
- no P5 association;
- no tuning;
- no defensive runtime bloat;
- legacy canonical workspace/history preserved.

## Files / Commits
- current repo changes
- diagnostic legacy path
- commits
- clean/push state

## Final Classification
Exactly one:
```text
I3_DIVERGENCE_SEMANTIC_FIXED
I3_DIVERGENCE_NUMERIC_ACCEPTED
I3_DIVERGENCE_UNRESOLVED
```

Do not start I4.

---

# 26. CLOSE criteria

Prompt 5 closes only if:
- legacy historical execution is reproduced well enough to serve as oracle;
- current migrated Super-input execution is reproduced;
- first divergent epoch is found;
- first divergent production stage is found;
- semantic identities are compared;
- causal minimal reproducer exists;
- semantic bug is fixed OR numeric/discrete cause is proven acceptable;
- no tuning;
- no visual/I4;
- no P5;
- no production defensive-programming expansion;
- Owner's legacy workspace is preserved;
- current worktree is clean.

The goal is causal attribution, not byte parity.
