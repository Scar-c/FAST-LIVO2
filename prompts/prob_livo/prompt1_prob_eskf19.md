# Prob-LIVO Integration — Prompt 1
## I1 ProbESKF19: Host-Layout 19D Shared State with Canonical Super ESKF Semantics

> This is the first functional implementation round after I0.
>
> **Scope:** build and prove a 19D `ProbESKF19` state/filter core inside the FAST-LIVO2 host repository.
>
> **Do not** integrate scheduler, undistortion, OctVox, P1–P4, VIO runtime, or datasets yet.

---

# 0. Owner architecture decision — frozen

The project architecture is already accepted by Owner.

Current authoritative state:

```text
I0 = CLOSED / OWNER VERIFIED
I1 = ACTIVE
I2–I8 = NOT STARTED

HOST authority   = FAST-LIVO2
LIO authority    = canonical Prob-LIO P0–P4
Visual authority = FAST-LIVO2 public source
shared state     = one x19/P19
LiDAR map        = Prob OctVox only
visual map       = FAST-LIVO2 feat_map
P5               = excluded
```

This round must not reopen I0 architectural choices.

The critical rule for I1 is:

```text
Preserve FAST-LIVO2 state ABI/layout
≠
Preserve FAST-LIVO2 LIO state-update semantics
```

`StatesGroup` remains the host-side shared state representation expected by FAST-LIVO2 visual code, but the LIO-side predict/update algebra must reproduce canonical Super ESKF semantics.

---

# 1. Repository / state consensus

Active repository:

```text
~/super_livo/src/FAST-LIVO2
```

Expected branch:

```text
prob-livo
```

Expected starting frontier:

```text
9ed486cc9e78f075ec74f3c9c48eb2a0efcc0c1b
```

Prob-LIO implementation oracle:

```text
~/super_livo/ref/Super-LIO
```

Expected reference branch:

```text
prob-lio
```

Expected reference frontier approximately:

```text
9fc949f46291c0fa76e5b7cdb372c940eb4b3f6e
```

At startup verify independently:

```bash
cd ~/super_livo/src/FAST-LIVO2
git status --short
git branch -vv
git rev-parse HEAD
git rev-parse origin/prob-livo
git log --oneline -15

cd ~/super_livo/ref/Super-LIO
git status --short
git branch -vv
git rev-parse HEAD
git log --oneline -15
```

Requirements:
- active host worktree clean;
- host HEAD matches expected Owner frontier or discrepancy reported before work;
- reference worktree clean;
- reference remains read-only.

No force push.
No history rewrite.
No modifications inside `~/super_livo/ref/Super-LIO`.

Register this exact prompt at:

```text
prompts/prob_livo/prompt1_prob_eskf19.md
```

Update:

```text
prompts/prob_livo/README.md
spec/prob_livo/SPEC.md
spec/prob_livo/EVIDENCE_INDEX.md
spec/prob_livo/HISTORY.md
```

---

# 2. I0 documentation corrigenda — docs only, before implementation

Before I1 code, correct two small I0 evidence issues.

## 2.1 Evidence commit identities

`spec/prob_livo/EVIDENCE_INDEX.md` must explicitly record:

```text
I0 bootstrap commit:
7dac83e726a32bc2a8551f445322959a523cbba3

I0 final/push-record HEAD:
9ed486cc9e78f075ec74f3c9c48eb2a0efcc0c1b
```

Do not leave placeholders such as “this focused bootstrap commit”.

## 2.2 Dataset evidence wording

Where dataset inventory currently infers camera data from calibration files, change the wording to evidence-safe semantics:

```text
camera calibration present;
camera topic presence in each bag pending explicit rosbag topic audit
```

unless actual topic inventory already exists as direct evidence.

This is a documentation correction only.

---

# 3. Baseline build gate before changing production code

Build current host before any I1 implementation.

Environment contract discovered in I0:

```bash
source /home/lc/design_ws/devel/setup.bash
cd /home/lc/super_livo
catkin_make --pkg fast_livo
```

### HARD GATE G-I1.0 — Clean host baseline

Invariant:

```text
starting I0 frontier builds with zero source modifications
```

Required evidence:
- start HEAD;
- clean git status;
- build command;
- RC;
- executable/target produced.

Failure classes:

```text
I1_BASELINE_ENV_FAILURE
I1_BASELINE_SOURCE_FAILURE
I1_FRONTIER_MISMATCH
```

If the starting frontier does not build under the known environment, STOP before I1 implementation.

---

# 4. Source authorities for I1

Before writing code, audit exact production source.

## 4.1 Canonical Super ESKF authority

Reference:

```text
~/super_livo/ref/Super-LIO/
src/super_lio/include/lio/ESKF.h
src/super_lio/src/lio/ESKF.cpp
```

The canonical Super physical state is:

```text
18D error/state covariance layout:
0–2    R
3–5    p
6–8    v
9–11   bg
12–14  ba
15–17  g
```

Key active source semantics to preserve include:

### Right rotation update

```text
R <- R * Exp(delta_theta)
```

### Additive Euclidean components

```text
p  <- p  + delta_p
v  <- v  + delta_v
bg <- bg + delta_bg
ba <- ba + delta_ba
```

### Super gravity update

```text
g <- g + delta_g
g <- g_gravity_norm * normalize(g)
```

This gravity re-normalization is canonical Super behavior and must be preserved in LIO update.

### Predict covariance

Preserve the exact active Super:
- `RightJacobianSO3`;
- `F_X`;
- `F_W`;
- `Q`;
- state propagation order;
- timestamp/`current_obs_time` boundary semantics where relevant to the pure filter core.

Do not “clean up” suspicious comments or noise naming in I1.
Production semantic parity is the goal, not theoretical redesign.

### Super iterated observation update

Canonical active algorithm includes:
- frozen predicted state snapshot;
- frozen `P_pred`;
- iterative `dx_prior`;
- rotational prior Jacobian/reset Jacobian;
- pose-only 6×6 information block embedded into full state;
- information-form solve;
- Super state increment;
- convergence check requiring `iter > 0`;
- final covariance reset/symmetrization.

Do not replace this with FAST-LIVO2 `StateEstimation()` mathematics.

---

## 4.2 FAST-LIVO2 state ABI authority

Host source:

```text
include/common_lib.h
```

Host shared layout is frozen:

```text
0–2    R
3–5    p
6      inverse exposure
7–9    v
10–12  bg
13–15  ba
16–18  g
```

`DIM_STATE=19`.

`StatesGroup` also currently defines FAST operators:
- `operator+`;
- `operator+=`;
- `operator-`.

Those FAST operators are part of existing visual semantics and must **not** be modified in I1.

Important semantic mismatch:

FAST `StatesGroup::operator+=` applies:

```text
gravity += delta_g
```

without Super gravity-norm projection.

Therefore:

```text
ProbESKF19 LIO update MUST NOT use StatesGroup::operator+= as its authoritative retraction.
```

---

# 5. I1 target architecture

Implement a new production component conceptually:

```text
ProbESKF19
```

Recommended project namespace/path:

```text
include/prob_livo/prob_eskf19.h
src/prob_livo/prob_eskf19.cpp
```

Exact naming may vary if repository conventions strongly suggest another path, but keep all new I1 code under a clean `prob_livo` namespace/module.

## 5.1 One authoritative shared state

`ProbESKF19` must operate on the host shared:

```text
StatesGroup
```

or an explicit reference/pointer to it.

Do not create a second persistent authoritative:
- rotation;
- position;
- velocity;
- bias;
- gravity;
- exposure;
- covariance.

Temporary predicted-state snapshots / iteration-local copies are allowed.

The future runtime ownership must remain:

```text
one StatesGroup x19/P19
```

---

# 6. Explicit layout map

Create one centralized layout definition.

Do not scatter numeric indices throughout the new implementation.

Recommended constants:

```text
ROT   = 0
POS   = 3
EXPO  = 6
VEL   = 7
BG    = 10
BA    = 13
GRAV  = 16
DIM   = 19
```

Canonical physical-18 index set in host layout:

```text
I18_IN_19 =
[0,1,2,
 3,4,5,
 7,8,9,
 10,11,12,
 13,14,15,
 16,17,18]
```

Implement tested helpers for:
- embed 18D vector into 19D with exposure slot;
- extract physical 18D vector;
- embed 18×18 covariance into 19×19;
- extract physical covariance block;
- embed 18×18 dynamics Jacobian into host layout;
- embed 18×12 Super noise Jacobian into host layout.

No hand-coded repeated remapping inside filter equations.

---

# 7. Exposure semantics in I1

I1 must support the FAST-LIVO2 inverse-exposure state dimension without changing Super physical LIO semantics.

## 7.1 Mean

During IMU prediction:

```text
inv_expo_time mean remains constant
```

unless a visual update later changes it.

Do not invent deterministic exposure dynamics.

## 7.2 Covariance

FAST-LIVO2 active source currently models exposure process covariance through:

```text
cov_inv_expo * dt^2
```

when exposure estimation is enabled.

For I1 implement a configurable exposure random-walk covariance term consistent with that host semantic.

The exact config source/API can be minimal in I1, but:
- default/initial value provenance must be documented;
- no visual update is enabled yet.

## 7.3 Cross covariance

The exposure dynamics are independent in the transition model:

```text
F_expo,expo = 1
F_expo,physical = 0
F_physical,expo = 0
```

But if `P_physical,expo` is already nonzero, the physical transition must propagate those cross terms correctly:

```text
P_xe' = F_x * P_xe
```

Do not forcibly zero valid cross covariance during prediction.

For parity tests, initialize cross covariance to zero so the physical 18D block must remain exactly decoupled from exposure.

---

# 8. Super-native LIO retraction on host state

Implement a dedicated LIO-side increment function, e.g.:

```text
ApplySuperLioIncrement19(...)
```

or equivalent.

Required semantics:

```text
R   <- R * Exp(dR)
p   <- p + dp
expo<- expo + d_expo        # normally zero for LiDAR-only update
v   <- v + dv
bg  <- bg + dbg
ba  <- ba + dba
g   <- g + dg
g   <- g_norm * normalize(g)
```

For I1 LiDAR observation updates:

```text
d_expo must arise only through full covariance/information coupling if mathematically present;
there is no direct LiDAR exposure measurement row.
```

Do not directly call host `StatesGroup::operator+=` for LIO update.

Do not modify `StatesGroup::operator+=`.

---

# 9. Super-native state difference / prior error

Implement the LIO-side state difference required by Super iterated update.

The physical 18D semantics must match canonical Super:

```text
rotation:
Log(R_pred^{-1} * R_current)

Euclidean:
current - predicted
```

Exposure difference occupies host index 6.

For pure LiDAR I1 parity with exposure decoupled, exposure prior difference should remain zero.

Do not use host `StatesGroup::operator-` blindly unless a test proves it matches every required I1 semantic. The host operator itself is not the LIO authority.

---

# 10. ProbESKF19 Predict

Port canonical Super predict mathematics into host layout.

Do not port FAST-LIVO2 IMU propagation in I1.

This is a pure filter-core implementation; scheduler/undistortion integration belongs to I2.

Required behavior:
- same Super midpoint IMU semantics used by reference core;
- same `RightJacobianSO3` semantics;
- same physical `F_X18`;
- same physical `F_W18`;
- same physical `Q12`;
- embed into `F_X19` / `F_W19`;
- exposure identity transition;
- exposure process covariance;
- full `P19` propagation;
- physical nominal state propagation identical to Super;
- exposure mean unchanged.

If timestamp-management concerns prevent a clean pure-core test, factor the mathematical propagation step into a deterministic function receiving explicit:
- state;
- `dt`;
- averaged gyro;
- averaged accel;
- noise.

Then wrap it with the future timestamp logic later.
Do not weaken semantic tests because of clock plumbing.

---

# 11. ProbESKF19 iterated LiDAR update

Port the canonical Super `UpdateObserve()` algebra into 19D host layout.

Required observation contract for I1:

```text
pose information only:
HT_Vinv_H : 6x6
HT_Vinv_r : 6x1
```

Embed at host pose indices:

```text
R 0–2
p 3–5
```

Exposure has no direct LiDAR measurement column.

The full solve remains 19D so existing/future cross covariance is respected.

Required canonical semantics:
1. freeze predicted state;
2. freeze predicted `P19`;
3. begin `need_converge=false`;
4. per iteration, set `need_converge=true` only under the canonical Super iteration condition;
5. call observation callback;
6. construct host-layout prior error;
7. apply Super rotational prior Jacobian;
8. compute retracted predicted covariance;
9. embed 6×6 information into 19×19;
10. perform information-form solve;
11. compute iterative increment with canonical Super formula;
12. apply **Super LIO retraction**, not FAST `operator+=`;
13. canonical convergence test;
14. after final iteration, use final posterior covariance;
15. apply Super reset Jacobian to rotation block;
16. symmetrize covariance;
17. clear increment state.

Do not introduce P5 or Prob-LIO map logic here.

---

# 12. Visual semantics must remain untouched in I1

Do not change:
- `src/vio.cpp`;
- `VIOManager`;
- `StatesGroup::operator+/-/+=`;
- visual Jacobian;
- exposure photometric residual;
- visual state update;
- visual rollback;
- visual covariance update.

I1 may add tests that inspect/assert the host visual ABI, but no behavior change.

Future I6 will evaluate whether FAST visual retraction creates a material gravity-norm discrepancy.

Do not preemptively “fix” FAST visual gravity semantics now.

---

# 13. Runtime integration freeze

I1 must **not** wire `ProbESKF19` into:

```text
LIVMapper::processImu
LIVMapper::handleLIO
LIVMapper::handleVIO
sync_packages
```

yet.

The existing FAST-LIVO2 executable must retain existing runtime behavior after I1.

I1 output is:
- a compiled production-ready filter core;
- tests/oracle;
- documentation;
- no live path switch.

This isolation is deliberate.

---

# 14. Authoritative oracle strategy

Tests must compare against canonical Super behavior, not a re-derived “expected implementation” written from memory.

Preferred options, in order:

## Option A — test-only imported Super oracle

Copy the minimal required Super ESKF source/equations into:

```text
tests/prob_livo/oracle/
```

with provenance:

```text
source repo:
~/super_livo/ref/Super-LIO

source commit:
<exact verified SHA>

source paths:
src/super_lio/include/lio/ESKF.h
src/super_lio/src/lio/ESKF.cpp
```

Adapt only dependencies necessary to compile the oracle.

The imported oracle must remain test-only and must not become production authority.

## Option B — golden numerical fixtures generated by exact reference implementation

If direct oracle compilation is disproportionately invasive:
- build/run a small deterministic reference harness against the read-only reference;
- generate compact golden fixtures;
- commit those fixtures and generator provenance;
- compare `ProbESKF19` against them.

Do not use hand-calculated expected matrices as the only parity authority.

Record whichever method is used in:

```text
spec/prob_livo/EVIDENCE_INDEX.md
spec/prob_livo/HISTORY.md
```

---

# 15. HARD GATE G-I1.1 — Layout bijection / covariance embedding

## Invariant

For arbitrary finite physical 18D vector `x18`:

```text
Extract18(Embed18(x18)) == x18
```

Exposure slot is exactly controlled and does not shift physical components.

For arbitrary symmetric 18×18 covariance `P18`:

```text
ExtractP18(EmbedP18(P18)) == P18
```

within machine-level numerical tolerance.

## Required adversarial mutations

Tests must fail if:
1. velocity is mapped to host 6–8 instead of 7–9;
2. bg/ba/gravity are shifted incorrectly;
3. exposure row/column overwrites a physical covariance entry;
4. covariance extraction silently drops an off-diagonal physical correlation.

Forbidden proxy:
- only testing identity/diagonal matrices.

Use dense SPD fixtures with nonzero off-diagonal terms.

---

# 16. HARD GATE G-I1.2 — Super LIO retraction parity

## Invariant

Given the same initial physical state and same nonzero physical increment:

```text
canonical Super Update()
```

and:

```text
ProbESKF19 Super-LIO retraction
```

must produce matching:
- rotation;
- position;
- velocity;
- gyro bias;
- accel bias;
- gravity.

Special gravity invariant:

```text
||g_after|| == g_gravity_norm
```

within tight tolerance.

## Required fixtures

At least:
- small rotation increment;
- nonzero translational/velocity/bias increments;
- nonzero gravity increment not parallel to gravity;
- combined dense increment.

## Required negative mutation

Run the same increment using host:

```text
StatesGroup::operator+=
```

as the LIO update.

The negative fixture must detect semantic mismatch, specifically gravity norm/direction where applicable.

If this mutation passes, G-I1.2 FAILS.

Forbidden substitute:
- checking only pose R/p.

---

# 17. HARD GATE G-I1.3 — Predict nominal-state parity

## Invariant

For the same initial state, IMU sample pair / averaged IMU and `dt`, canonical Super predict and ProbESKF19 predict must match the physical nominal state:

```text
R
p
v
bg
ba
g
```

within explicit numerical tolerance.

Exposure mean must remain unchanged.

## Required fixtures

Include:
1. zero angular rate / gravity-only;
2. finite angular velocity;
3. nonzero accel with nonidentity rotation;
4. nonzero biases;
5. small `dt`;
6. realistic `dt`;
7. multiple sequential prediction steps.

## Forbidden substitutes

Do not compare only final pose.
Velocity and full physical state are mandatory.

---

# 18. HARD GATE G-I1.4 — Predict covariance parity

## Invariant

With:
- identical Super noise parameters;
- zero physical/exposure cross covariance initially;
- exposure random walk disabled or isolated for parity subtest;

the extracted physical covariance after ProbESKF19 predict must match canonical Super `P18`.

Compare the full dense 18×18 matrix.

## Required identities

Test:
- `F_X18` embedding;
- `F_W18` embedding;
- propagated `P18`;
- symmetry/finite status.

## Required negative mutations

Each must fail:
1. use FAST-LIVO2 `F_x` instead of Super `F_X`;
2. omit Super `RightJacobianSO3` bias-rotation term;
3. swap bias-noise placement;
4. use incorrect host index mapping;
5. square/unsquare Super noise parameters differently from reference semantics.

Do not “correct” Super implementation based on theoretical preference.

---

# 19. HARD GATE G-I1.5 — Exposure isolation and cross-covariance propagation

Two subtests are required.

## A. Decoupled parity mode

Initialize:

```text
P_xe = 0
P_ex = 0
```

No visual update.

After arbitrary prediction sequence:
- physical mean equals Super;
- physical covariance equals Super;
- exposure mean unchanged;
- physical/exposure cross covariance remains zero;
- only `P_ee` changes according to configured exposure process covariance.

## B. Existing nonzero cross covariance

Initialize a valid dense `P_xe != 0`.

Under independent exposure transition:

```text
P_xe' = F_x * P_xe
```

must be preserved mathematically.

Do not zero cross terms.

Required negative mutation:
- explicitly zero cross covariance after each predict;
- test must fail.

---

# 20. HARD GATE G-I1.6 — Iterated LiDAR update parity

## Invariant

Use the same deterministic observation callback sequence for:
- canonical Super ESKF;
- ProbESKF19.

Callback provides known per-iteration:

```text
HT_Vinv_H 6x6
HT_Vinv_r 6x1
```

Require parity of:
- number of observation callbacks;
- `need_converge` sequence;
- physical state after each iteration or at least final plus diagnostic trace;
- final physical covariance 18×18;
- final convergence decision.

Use full nontrivial covariance and residuals.

## Required cases

At least:
1. two-iteration convergence;
2. max-iteration path;
3. rotation + translation information;
4. dense pose information matrix;
5. nonzero prior error induced by iteration.

## Required negative mutations

Tests must fail if:
1. host `StatesGroup::operator+=` is used for LIO update;
2. prior rotational reset Jacobian is omitted;
3. final covariance reset Jacobian is omitted;
4. convergence is allowed at first callback contrary to Super condition;
5. FAST-LIVO2 LIO `StateEstimation` formula substitutes Super formula;
6. pose information is inserted at wrong host indices due exposure slot.

---

# 21. HARD GATE G-I1.7 — No direct LiDAR exposure measurement

## Invariant

The LiDAR observation information block contains direct information only in:

```text
R 0–2
p 3–5
```

No direct H/HTRH entry may be written to host exposure index 6.

With zero exposure cross covariance:
- LiDAR update leaves exposure mean and variance unchanged except unrelated process covariance already present.

With nonzero exposure cross covariance:
- exposure may change indirectly through full posterior coupling;
- this must be mathematically traceable, not from a direct measurement column.

Required negative mutation:
- insert pose block as contiguous host `[0:7]` or accidentally include exposure;
- gate must fail.

---

# 22. HARD GATE G-I1.8 — Covariance validity

For all deterministic tests:

```text
P19 finite
P19 symmetric within tolerance
physical P18 finite
```

Use a PSD/near-PSD diagnostic in test code if useful.

Do not add a heavy production eigensolver hot path.

This is I1 test validation only.

Failure classifications should distinguish:
- nonfinite;
- asymmetry;
- negative covariance pathology;
- singular information fixture.

---

# 23. HARD GATE G-I1.9 — FAST visual ABI untouched

## Invariant

I1 may add new code, but must not alter behavior of:
- `StatesGroup` layout;
- `StatesGroup::operator+`;
- `StatesGroup::operator+=`;
- `StatesGroup::operator-`;
- `src/vio.cpp`.

Required evidence:

```bash
git diff <I1_START>..HEAD -- include/common_lib.h src/vio.cpp
```

Any change must be documentation/comment-only and justified; behavioral changes are forbidden.

Required static checks:
- `DIM_STATE` remains 19;
- exposure remains index 6;
- visual H still assumes pose+exposure prefix layout.

---

# 24. HARD GATE G-I1.10 — Existing host runtime non-interference

Since I1 is not wired into LIVMapper:

```text
fastlivo_mapping
```

must still build with existing FAST runtime path.

Required:
- clean full host build;
- existing executable links;
- no callsite from `LIVMapper` to `ProbESKF19`.

A static grep/source check should prove the new filter is dormant except tests.

No bag run required.

---

# 25. HARD GATE G-I1.11 — Single shared-state future contract

The production API introduced by I1 must be suitable for future use with the existing `_state`.

Forbidden designs:
- internal persistent `StatesGroup state_` while LIVMapper also owns `_state`;
- internal persistent duplicate covariance disconnected from `StatesGroup::cov`;
- conversion/copy bridge every LIO update producing two competing authorities.

Acceptable:
- `ProbESKF19` operates on `StatesGroup&`;
- or owns a non-owning pointer/reference explicitly supplied by LIVMapper later;
- iteration-local predicted snapshots.

Provide a source-level ownership explanation in SPEC.

---

# 26. Test implementation quality requirements

Do not create tests that merely compare a function to itself.

Every parity test must have two independent semantic authorities:

```text
canonical Super oracle
vs
ProbESKF19
```

or:
```text
canonical Super-generated golden fixture
vs
ProbESKF19
```

Do not:
- derive expected matrices from the same helper under test;
- use tautological state round-trips as proof of filter parity;
- mark a mutation caught only because a field changed if the semantic invariant is not checked.

Every HARD GATE must include:
- authoritative production path;
- expected observable;
- tolerance;
- negative mutation or adversarial fixture;
- failure classification.

---

# 27. Recommended tests

Recommended files:

```text
tests/prob_livo/test_i1_layout.cpp
tests/prob_livo/test_i1_retraction.cpp
tests/prob_livo/test_i1_predict.cpp
tests/prob_livo/test_i1_update.cpp
tests/prob_livo/test_i1_exposure.cpp
```

or a similarly clean split.

Avoid one 2000-line monolithic test.

No `std::vector<bool>`.

Use deterministic seeds if randomized dense SPD fixtures are generated.

Print enough information on failure to identify:
- matrix block;
- max abs error;
- index;
- expected/actual value.

---

# 28. Numerical tolerances

Do not choose loose tolerances to force PASS.

Use double precision.

For exact algebraic embedding:
```text
~1e-12 or tighter where reasonable
```

For SO(3)/multi-step numeric parity:
```text
derive tolerance from double-precision numerical path;
normally 1e-10–1e-8 depending on operation count
```

If oracle and production use different SO(3) implementations and produce slightly different rounding:
- characterize the difference;
- use a justified tolerance;
- do not accept millimeter/degree-scale discrepancy as “numerical”.

Report max errors for:
- rotation log norm;
- p/v/bias/g;
- covariance.

---

# 29. Implementation sequence — compile after every bounded step

Use this order:

```text
A. I0 docs corrigenda
→ commit optional docs-only or keep staged

B. layout constants/helpers
→ focused compile/test

C. Super LIO retraction/state difference
→ focused compile/test

D. predict nominal + covariance
→ focused compile/test

E. exposure process/cross covariance
→ focused compile/test

F. iterated LiDAR update
→ focused compile/test

G. full I1 suite
→ full host build

H. commit
→ clean
→ final verification
```

Do not accumulate all I1 code before first build.

---

# 30. No bag / no scheduler / no VIO runtime

Prompt 1 explicitly forbids:
- NTU run;
- Oxford run;
- rosbag play;
- LIVO scheduler integration;
- current scan undistortion integration;
- OctVox import;
- P1–P4 import;
- `pointWithVar` runtime adapter;
- PlaneProvider;
- camera/VIO run.

These belong to later stages.

---

# 31. Copy-on-demand provenance

If you copy test/oracle code from:

```text
~/super_livo/ref/Super-LIO
```

or:

```text
~/prob_lio/src/Super-LIO
```

record in the SPEC import ledger:

```text
source repo/workspace
source commit
source path
destination
purpose
test-only vs production
adaptations
```

Do not symlink.

Do not import old build/devel artifacts.

---

# 32. SPEC updates

Update `spec/prob_livo/SPEC.md`.

Required changes:

## I0

```text
I0 = CLOSED / OWNER VERIFIED
```

## I1 architecture

Document:
- state layout;
- physical-18 index map;
- `StatesGroup` visual ABI vs Super LIO retraction distinction;
- one shared state ownership;
- exposure process semantics;
- parity oracle;
- I1 gate results.

## Roadmap

At completion:

```text
I1 = CLOSED/PASS — Owner audit pending
I2–I8 = NOT STARTED
```

Do not mark I1 Owner Verified yourself.

---

# 33. EVIDENCE_INDEX updates

Record:
- start HEAD;
- reference oracle SHA;
- I0 corrected commit identities;
- baseline build;
- test commands;
- all I1 gate evidence;
- max numerical errors;
- final build;
- I1 implementation commit;
- final HEAD.

No dataset evidence.

---

# 34. HISTORY update

Record why the project intentionally uses two different state-operation authorities:

```text
FAST StatesGroup operators:
    retained for FAST visual semantics / ABI

ProbESKF19 LIO retraction:
    canonical Super semantics
```

Explicitly note:
- FAST additive gravity differs from Super gravity normalization;
- this is why direct use of `StatesGroup::operator+=` in LIO is forbidden;
- I6 will later observe visual-induced gravity correction rather than changing VIO now.

---

# 35. Commit policy

Use focused commits.

Suggested:

### Commit A — optional I0 docs correction

```text
docs(prob-livo): close owner audit of integration bootstrap
```

### Commit B — I1 implementation

```text
feat(prob-livo): add Super-semantic 19d filter core
```

### Commit C — tests/docs if naturally separated

```text
test(prob-livo): prove ProbESKF19 parity with Super oracle
```

Do not create many tiny noisy commits unless necessary for safe development.

Before final push:

```bash
git status --short
git diff <I1_START>..HEAD --stat
git log --oneline <I1_START>..HEAD
```

Worktree must be clean.

Push fast-forward to:

```text
origin/prob-livo
```

No force push.

---

# 36. Final report format

## Agent State Consensus
- start HEAD
- origin HEAD
- branch
- worktree
- reference SHA
- prompt registration

## I0 Corrigenda
- evidence SHA correction
- dataset wording correction
- I0 status → OWNER VERIFIED

## Source Authority Audit
### Super
- state layout
- Predict
- Update
- UpdateObserve
- gravity semantics

### FAST
- StatesGroup layout
- operator semantics
- visual dependence on layout

## I1 Implementation
- files/classes
- one-state ownership model
- layout mapping
- retraction
- predict
- update
- exposure process

## Oracle
- method used
- provenance
- imported files if any

## Gate Results

Report each independently:

```text
G-I1.0  clean host baseline
G-I1.1  layout/covariance embedding
G-I1.2  Super LIO retraction parity
G-I1.3  predict nominal parity
G-I1.4  predict covariance parity
G-I1.5  exposure isolation/cross covariance
G-I1.6  iterated LiDAR update parity
G-I1.7  no direct LiDAR exposure measurement
G-I1.8  covariance validity
G-I1.9  FAST visual ABI untouched
G-I1.10 existing host runtime non-interference
G-I1.11 single shared-state future contract
```

For each:
- semantic invariant;
- authoritative production/oracle path;
- numerical evidence;
- negative mutation/adversarial fixture;
- PASS/FAIL.

## Numerical Error Summary
At minimum:
- rotation error;
- position;
- velocity;
- biases;
- gravity;
- physical covariance max abs error;
- exposure covariance;
- cross covariance.

## Build
- focused test commands
- full build command
- RC

## Scope Audit
Explicitly confirm:
- no scheduler integration;
- no VIO behavior change;
- no OctVox/P1–P4 import;
- no bag run;
- P5 absent.

## Files / Commits
- changed files
- commits
- final HEAD
- worktree clean
- push status

## Final State

End with:

```text
I0 = CLOSED / OWNER VERIFIED
I1 = CLOSED/PASS — Owner audit pending
I2–I8 = NOT STARTED

FAST-LIVO2 StatesGroup ABI = preserved
ProbESKF19 LIO semantics   = canonical Super ESKF
shared state               = one x19/P19 design
runtime integration        = NOT STARTED

Next stage = I2 Super IMU + undistortion under FAST-LIVO2 scheduler
```

Do not begin I2.

---

# 37. Final CLOSE criteria

Prompt 1 is complete only if:

```text
G-I1.0  GREEN
G-I1.1  GREEN
G-I1.2  GREEN
G-I1.3  GREEN
G-I1.4  GREEN
G-I1.5  GREEN
G-I1.6  GREEN
G-I1.7  GREEN
G-I1.8  GREEN
G-I1.9  GREEN
G-I1.10 GREEN
G-I1.11 GREEN

canonical Super oracle used
host full build PASS
visual ABI unchanged
runtime path unchanged
no bag run
SPEC/EVIDENCE/HISTORY updated
worktree clean
fast-forward push complete
```

If any gate cannot be proven, do not declare I1 closed.

---

# 38. Review contract

The Agent's final prose is not acceptance authority.

Owner/reviewer will independently inspect:
- `ProbESKF19`;
- state layout mapping;
- gravity update;
- Predict F/Q embedding;
- observation information embedding;
- iterative prior/reset semantics;
- test oracle independence;
- negative mutations;
- exposure isolation;
- FAST visual ABI diff;
- runtime non-interference;
- SPEC/evidence.

A green test suite is not enough if the tests encode the same wrong semantics as production.
