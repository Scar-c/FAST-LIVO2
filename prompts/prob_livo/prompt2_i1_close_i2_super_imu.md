# Prob-LIVO Integration — Prompt 2
## Close I1 Corrective + I2 Super-Native IMU/Undistortion Under FAST-LIVO2 Scheduler

> This round has two strictly ordered parts.
>
> **Part A:** make the tiny I1 corrective found by Owner and close I1.
>
> **Part B:** implement and prove I2 — Super-native IMU initialization, propagation and undistortion operating on the FAST-LIVO2 scheduler packet/timing contract.
>
> Do **not** start I3 Prob-LIO map/LiDAR backend migration.

---

# 0. Owner state / architecture freeze

Current authoritative state entering Prompt 2:

```text
I0 = CLOSED / OWNER VERIFIED

I1:
  19D mathematical core    = GREEN
  physical 18D parity      = GREEN
  observation seam         = CORRECTIVE REQUIRED
  Owner Verified           = NO

I2 = BLOCKED until Part A closes
I3–I8 = NOT STARTED
```

Project authority remains:

```text
HOST authority   = FAST-LIVO2 public source
LIO authority    = canonical Prob-LIO P0–P4
Visual authority = FAST-LIVO2 public source

shared state     = one x19/P19
LiDAR map        = future Prob OctVox only
visual map       = FAST-LIVO2 feat_map
P5               = excluded
```

Do not reopen these decisions.

---

# 1. Repository / consensus

Active repo:

```text
~/super_livo/src/FAST-LIVO2
```

Expected branch:

```text
prob-livo
```

Expected start frontier:

```text
ecd8058f08bcae987f3d87934f237964409773bf
```

Prob-LIO oracle:

```text
~/super_livo/ref/Super-LIO
```

Expected reference:

```text
branch: prob-lio
HEAD:   9fc949f46291c0fa76e5b7cdb372c940eb4b3f6e
```

Verify before work:

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
git log --oneline -10
```

Requirements:
- host clean;
- host HEAD matches Owner frontier;
- reference clean/read-only;
- no history rewrite;
- no force push.

Register this exact prompt:

```text
prompts/prob_livo/prompt2_i1_close_i2_super_imu.md
```

Update prompt index.

---

# 2. Build baseline before edits

Known build environment:

```bash
source /home/lc/design_ws/devel/setup.bash
cd /home/lc/super_livo
catkin_make --pkg fast_livo
```

### HARD GATE G-P2.0

Before editing:

```text
ecd8058 clean full host build = PASS
existing I1 focused tests = PASS
```

If not:
- classify environment vs source;
- stop before modifying code.

---

# PART A — TERMINAL I1 CORRECTIVE

# 3. Exact I1 defect to fix

Current `ProbESKF19::ObservationCallback` provides:

```text
state
HT_Vinv_H
HT_Vinv_r
```

but does not pass:

```text
need_converge
```

to the measurement producer.

This is not canonical Super observation semantics.

Canonical Super production contract is conceptually:

```text
KFState {
    need_converge
    pose
}
→ Observe(...)
```

and `Observe()` uses `need_converge` to control:
- HKNN refresh;
- plane recomputation;
- association lifecycle.

Therefore the callback itself must carry the convergence-phase state.

Do not defer this to I3.

---

# 4. Required API corrective

Change the production callback seam to a direct, explicit contract, conceptually:

```cpp
using ObservationCallback =
    std::function<void(
        const StatesGroup& state,
        bool need_converge,
        Matrix6& HT_Vinv_H,
        Vector6& HT_Vinv_r)>;
```

Inside `UpdateObserve()`:

```cpp
observation(
    state_,
    need_converge_,
    HT_Vinv_H,
    HT_Vinv_r);
```

Equivalent strongly typed struct is acceptable only if it is simpler and directly exposes the same semantic invariant.

Forbidden:
- callback captures filter and calls `filter.need_converge()`;
- external iteration counter;
- hidden global/thread-local convergence state;
- delayed reporting after callback.

The measurement producer must receive the value as part of the call contract.

---

# 5. HARD GATE G-I1.C1 — callback lifecycle parity

## Invariant

For a 4-callback/max-iteration fixture the callback must observe exactly:

```text
callback 1 → need_converge=false
callback 2 → need_converge=false
callback 3 → need_converge=false
callback 4 → need_converge=true
```

i.e.:

```text
[F,F,F,T]
```

For a 2-callback convergence fixture:

```text
[F,F]
```

Cross-check this against canonical Super `UpdateObserve`.

## Required negative mutations

Each must fail:
1. always pass `false`;
2. update `need_converge` only after callback;
3. set true at callback 3 (`iter >= 2`);
4. callback queries filter state indirectly instead of receiving authoritative argument.

## Observable evidence

Report:
- callback sequence;
- canonical oracle sequence;
- count parity;
- final state/covariance parity unchanged.

---

# 6. HARD GATE G-I1.C2 — I1 math remains unchanged

After callback corrective, rerun all existing I1 gates.

Required:
- same numerical parity envelope as Prompt 1;
- no change to Predict;
- no change to retraction;
- no change to exposure process;
- no change to FAST visual ABI.

If the callback change causes a mathematical difference in existing deterministic fixtures, classify and stop.

---

# 7. SO(3) oracle hardening

This is a hardening item, not a redesign.

Current test-only oracle reimplements standard SO(3) functions. Add one compact golden parity fixture generated from the **actual Super reference implementation** or directly compiled against the reference implementation.

Cover at least:
- very small rotation vector;
- normal small IMU increment;
- moderate angle;
- `Log(Exp(phi))` consistency in the estimator operating range.

Purpose:

```text
prove production/oracle helpers are also compatible with actual Super BASIC::SO3 behavior
```

Do not rewrite production SO(3) if current numbers already match.

---

# 8. Provenance correction

Fix stale oracle source-line references.

Use exact reference commit/path as authority:

```text
Super-LIO:
9fc949f46291c0fa76e5b7cdb372c940eb4b3f6e
```

Record function names as primary provenance; line numbers may be included but must match the actual file at that commit.

---

# 9. I1 closure commit

After:
- G-I1.C1 GREEN;
- G-I1.C2 GREEN;
- SO(3) hardening GREEN;
- full host build GREEN;

commit the I1 corrective before I2 implementation.

Suggested:

```text
fix(prob-livo): close Super observation lifecycle seam
```

Verify clean worktree.

Update SPEC:

```text
I1 = CLOSED / OWNER VERIFIED
I2 = ACTIVE
```

Do not start I2 on a dirty tree.

---

# PART B — I2 SUPER IMU + UNDISTORTION UNDER FAST SCHEDULER

# 10. I2 mission

I2 must implement a production-ready adapter:

```text
FAST-LIVO2 scheduler packet/timing
        ↓
Super-native IMU initialization
        ↓
ProbESKF19 Predict
        ↓
Super-native propagation-state trace
        ↓
Super-native scan undistortion
        ↓
scan-end IMU-frame point cloud for future I3
```

I2 does **not** replace FAST-LIVO2 LiDAR map/update yet.

The end product is a tested `ProbImuAdapter` ready for I3.

---

# 11. Critical architecture boundary for I2

Do not create an accidental hybrid estimator:

```text
Super undistorted IMU-frame points
→ existing FAST VoxelMapManager LIO
```

because the frame/semantics are not the same.

Current FAST LiDAR backend expects its own undistorted LiDAR-frame representation and extrinsic handling.

Therefore:

```text
I2 canonical output = scan-end IMU-frame cloud for future Prob-LIO I3
```

and must not silently feed that cloud into current FAST `handleLIO()`.

If a runtime backend selector is introduced, default remains existing FAST runtime and the Prob-Super option must be staged/guarded until I3 consumes the correct output.

No full hybrid bag result may be presented as meaningful algorithm evidence.

---

# 12. I2 source authorities

## 12.1 FAST scheduler authority

Audit current active:

```text
LIVMapper::sync_packages
LidarMeasureGroup
MeasureGroup
LIVMapper::run
LIVMapper::processImu
```

Important scheduler facts to verify from production source:

### ONLY_LIO

```text
epoch end:
    meas.measures.back().lio_time
    = lidar_frame_end_time
```

and `pcl_proc_cur` is the whole current LiDAR packet with its time offsets.

### LIVO LIO epoch

The scheduler cuts LiDAR at image capture time.

For each point assigned to current LIO packet, production rewrites:

```text
curvature_ms +=
(frame_header_time - last_lio_update_time) * 1000
```

so current `pcl_proc_cur` point curvature is an offset from the current LIO epoch start:

```text
last_lio_update_time
```

while:

```text
measures.back().lio_time = camera capture time
```

This time contract is central to I2.

Do not change scheduler cut policy in I2.

---

# 13. Canonical Super IMU authority

Use reference:

```text
SuperLIO::kf_init
SuperLIO::Propagation_Undistort
ESKF::Predict
```

Canonical initialization includes:
- accumulated mean gyro/accel;
- minimum initialization sample count;
- gravity magnitude normalization;
- initial gyro bias from mean gyro;
- accel scale;
- initial rotation/gravity alignment;
- configured robot/yaw transform semantics if active;
- Super covariance initialization/options.

Canonical propagation includes:
- Super midpoint IMU;
- Super physical state/covariance Predict;
- Super observation-boundary clipping semantics;
- propagated state snapshots.

Canonical undistortion uses:
- scan point time;
- two propagated IMU states;
- SO(3) slerp;
- translational interpolation with velocity/acceleration;
- LiDAR→IMU extrinsic;
- transform to scan-end IMU frame.

---

# 14. Implement `ProbImuAdapter`

Recommended module:

```text
include/prob_livo/prob_imu_adapter.h
src/prob_livo/prob_imu_adapter.cpp
```

Possible API:

```cpp
class ProbImuAdapter {
public:
    struct Result {
        bool initialized;
        bool propagated;
        double epoch_start;
        double epoch_end;
        PointCloudXYZI::Ptr scan_end_imu;
    };

    Result ProcessLioEpoch(
        LidarMeasureGroup& measures,
        ProbESKF19& filter,
        ...);
};
```

Exact naming may vary.

Requirements:
- no duplicate authoritative navigation state;
- same `StatesGroup&` / `ProbESKF19`;
- persistent bookkeeping may contain init accumulators, prior IMU sample and propagation trace;
- no second persistent pose/covariance authority.

---

# 15. I2 output frame contract

Canonical I2 undistorted output for future I3 is:

```text
scan-end IMU/body frame
```

For each raw LiDAR point:

```text
LiDAR frame at acquisition
→ LiDAR-to-IMU extrinsic
→ propagated motion
→ epoch-end inverse pose
→ scan-end IMU frame
```

Give this buffer an explicit frame-safe name such as:

```text
prob_scan_undistort_imu
```

Do not disguise it as current FAST LiDAR-frame `feats_undistort`.

---

# 16. FAST packet → Super scan-time adapter

For an LIO epoch, after source audit establish one centralized contract.

Expected LIVO semantics:

```text
epoch_start = LidarMeasures.last_lio_update_time at adapter entry
epoch_end   = LidarMeasures.measures.back().lio_time

point_time =
epoch_start + point.curvature / 1000
```

because `sync_packages` already rebases `pcl_proc_cur.curvature` to `last_lio_update_time`.

For ONLY_LIO, verify the corresponding start origin from production source before coding; do not blindly reuse LIVO assumptions.

Required validity checks:
- finite start/end;
- end >= start;
- finite point offsets;
- point absolute times lie in expected epoch tolerance;
- no `pcl_proc_next` point leaks into current epoch.

---

# 17. HARD GATE G-I2.1 — production scheduler time contract

## Invariant

Exercise the real scheduling/cut function or a helper extracted from and used by production `sync_packages`.

### ONLY_LIO
Prove:
- `lio_time` equals scan end;
- point offsets resolve to correct absolute times.

### LIVO cut
Given two source LiDAR frames straddling camera time, prove:
- pre-cut points → `pcl_proc_cur`;
- post-cut points → `pcl_proc_next`;
- current curvature origin = prior `last_lio_update_time`;
- next curvature origin = current `lio_time`;
- current IMU collection boundary is `<= lio_time`.

## Forbidden substitute

Do not hand-construct an already-correct final packet and call that scheduler testing.

## Negative mutations

Tests fail if:
1. original frame header is used instead of rebased epoch origin;
2. camera time is incorrectly used as both current and next origin;
3. post-cut point enters current packet;
4. current measurement consumes IMU after epoch end.

---

# 18. Endpoint propagation audit — mandatory before implementation choice

There is a potentially important seam difference between FAST scheduler packet ownership and canonical Super observation-boundary handling.

Before implementing endpoint behavior, inspect the **actual canonical Super measurement wrapper** and answer with production evidence:

```text
Does standalone Super supply a look-ahead IMU sample beyond lidar.end_time,
so ESKF::Predict clamps that interval to current_obs_time?

or

How exactly is the state brought to lidar.end_time?
```

Then compare to FAST scheduler, which collects current IMU samples up to the LIO epoch boundary and leaves newer samples buffered.

Do not guess.

### Decision rule

If the FAST packet does not expose data required for exact Super endpoint semantics:
- implement the smallest explicit scheduler-interface adapter;
- document it as interface adaptation, not estimator redesign;
- prove it numerically against the canonical Super oracle.

Acceptable source-grounded mechanisms may include:
- non-consuming look-ahead IMU access;
- exact final partial interval formed from the same neighboring IMU data;
- another mechanism proved by source audit.

Forbidden:
- simply stop at the last IMU timestamp when `epoch_end` is later;
- silently use FAST propagation math for the final segment.

---

# 19. HARD GATE G-I2.2 — exact epoch-end state

After an initialized LIO epoch:

```text
state/covariance must correspond to epoch_end
```

and the physical state/covariance must match canonical Super semantics propagated to the same endpoint.

Cases:
1. IMU exactly at epoch end;
2. epoch end between IMU timestamps where canonical source supports bracketing;
3. final scheduler IMU before epoch end;
4. tiny final partial interval;
5. repeated consecutive epochs.

Observe:
- final time authority;
- R/p/v/bg/ba/g;
- physical covariance;
- exposure;
- last-IMU/last-observation bookkeeping.

Negative mutation:
- stop at last IMU sample rather than requested endpoint.

---

# 20. Super-native IMU initialization

Implement canonical Super initialization rather than using FAST `IMU_init` as Prob authority.

Support:
- IMU accumulation;
- canonical minimum sample count;
- mean gyro/accel;
- gyro-bias initialization;
- accel scale;
- gravity vector;
- initial rotation/gravity alignment;
- Super initial covariance/options;
- initial timestamp;
- any active canonical robot/yaw transform semantics.

Exposure remains host/default visual state and is not initialized from LiDAR/IMU.

Do not overload FAST `p_imu->imu_need_init` as the new authority. Expose a Prob initialization status seam.

---

# 21. HARD GATE G-I2.3 — initialization parity

Feed the same deterministic IMU sequence to:
- actual/canonical Super initialization oracle;
- `ProbImuAdapter`.

Require parity in:
- exact transition point from uninitialized to initialized;
- mean gyro;
- mean accel;
- bg;
- accel scale;
- gravity;
- initial rotation;
- physical covariance;
- timestamp.

Fixtures:
1. level stationary;
2. tilted stationary;
3. nonzero gyro bias;
4. raw accel norm requiring scaling;
5. nontrivial robot/yaw transform when supported by reference.

Negative mutations:
- FAST initialization math;
- hardcode 9.81 where canonical config differs;
- skip transform;
- initialize too early.

---

# 22. Propagation trace authority

Super undistortion needs per-IMU snapshots:

```text
time
R
p
v
a
w
```

These snapshots must come from the actual `ProbESKF19` propagation path.

Do not integrate a second trajectory in the adapter.

If needed, extend `ProbESKF19::Predict` with a small explicit output/trace API.

Forbidden:
- parallel FAST `IMUpose` trajectory;
- duplicate propagation equations for undistortion.

---

# 23. HARD GATE G-I2.4 — propagated trace parity

For the same initial state and IMU sequence compare each accepted snapshot against canonical Super:

```text
time
R
p
v
a
w
```

Cases:
- static;
- constant angular rate;
- angular + translational acceleration;
- nonzero biases;
- partial final interval.

Report max errors.

Negative mutation:
- use post-rotation orientation for acceleration if canonical Super uses pre-update rotation ordering.

---

# 24. Super-native undistortion

Port/adapt canonical `Propagation_Undistort()`.

For each point:
1. absolute query time = scheduler epoch origin + point offset;
2. find bracketing propagation states;
3. quaternion slerp for rotation;
4. canonical translation interpolation:
   ```text
   p_i = p_h + v_h*tau + 0.5*acc_t*tau^2
   ```
5. apply LiDAR→IMU extrinsic in the canonical direction;
6. transform into epoch-end IMU frame.

Preserve point identity and intensity.

Explicitly handle:
- query == first state time;
- query == final time;
- small numeric epsilon outside range;
- duplicate timestamps / zero interval;
- empty or insufficient trace.

No unsafe `next_iter` dereference.

---

# 25. HARD GATE G-I2.5 — undistortion geometric parity

Compare every point against canonical Super undistortion.

Fixtures:
1. no motion;
2. pure translation;
3. pure rotation;
4. coupled motion;
5. nonidentity LiDAR→IMU R/t;
6. point at epoch start;
7. point at epoch end;
8. point between IMU states;
9. multiple LiDAR frames recombined into one LIVO epoch.

Evidence:
- max XYZ error;
- point count;
- point ordering;
- output frame demonstration.

Negative mutations:
1. output LiDAR frame instead of IMU frame;
2. omit extrinsic translation;
3. invert extrinsic rotation;
4. linear rotation interpolation;
5. use original source-frame header for LIVO-recombined point;
6. substitute FAST backward-undistortion formula.

---

# 26. HARD GATE G-I2.6 — point/time identity

For every current packet:

```text
input point i → output point i
```

unless canonical Super explicitly reorders.

Verify:
- intensity preserved;
- index identity preserved;
- offset/time provenance preserved;
- current and next packets remain disjoint.

Use unique point IDs/intensities.

Negative mutation:
- timestamp sort that silently loses original identity.

---

# 27. HARD GATE G-I2.7 — consecutive epoch continuity

Run at least three sequential LIO epochs.

Require:

```text
epoch N end == epoch N+1 start
```

within time representation precision.

Prove:
- no double integration;
- no dropped integration gap;
- IMU boundary bookkeeping correct;
- `last_lio_update_time` advances exactly once after successful epoch completion;
- LIVO `pcl_proc_next` becomes future current packet as scheduler defines.

Negative mutations:
1. advance `last_lio_update_time` before success;
2. scheduler and adapter both advance it independently;
3. integrate last interval twice;
4. skip first interval next epoch.

---

# 28. HARD GATE G-I2.8 — camera-cut scheduler seam

Construct a seam-faithful LIVO scenario:
- LiDAR frame spans image time;
- scheduler cuts at image capture;
- IMU samples support that epoch;
- current cloud contains all and only pre-cut points;
- adapter reaches the exact image/LIO epoch time;
- current points are undistorted into that epoch-end IMU frame.

This must exercise real H0→H1→H2 seam:

```text
FAST scheduler
→ shared ProbESKF19 state
→ ProbImuAdapter
```

This is the decisive I2 integration test.

---

# 29. Runtime integration policy

I2 may add a clean IMU backend interface/selector around `LIVMapper::processImu`, but:

```text
default runtime must remain existing FAST path
```

until I3 adds the matching Prob-LIO LiDAR backend.

If `prob_super` is selectable:
- focused tests may invoke it;
- production must guard against passing its IMU-frame cloud into existing FAST `VoxelMapManager` as if it were FAST LiDAR-frame `feats_undistort`.

Acceptable:
- staged backend selector defaulting to FAST;
- a guard saying Prob IMU requires future Prob LIO backend;
- dedicated scheduler seam test.

Forbidden:
- enable hybrid runtime by default;
- present hybrid ATE as algorithm evidence.

---

# 30. HARD GATE G-I2.9 — no accidental hybrid authority

Prove statically/source-level that the I2 canonical Prob output:

```text
scan-end IMU-frame cloud
```

is not consumed by existing FAST LIO backend in default production execution.

Required negative mutation:
- wire Prob IMU-frame cloud directly to existing FAST `feats_undistort` / `VoxelMapManager::StateEstimation`;
- safety/static test must fail.

Document both frame contracts.

---

# 31. Scheduler / visual freeze

I2 must not alter semantic behavior of:
- camera cut timestamp;
- image drop policy;
- point current/next partition;
- LIO→VIO scheduling;
- VIOManager;
- StatesGroup FAST operators;
- visual update.

Scheduler refactor into helpers is allowed only if production uses those helpers and equivalence tests pass.

---

# 32. No dataset run

Do not run full NTU or Oxford bags in I2.

I2 evidence:
- deterministic oracle tests;
- production scheduler seam tests;
- full build.

The first real camera-OFF bag baseline is I3.

---

# 33. CWD / artifact hygiene

Any test/helper must:
- use project-relative path resolution;
- not depend on accidental CWD;
- not place required evidence only in `/tmp`;
- not write generated output into `src/` or `include/`.

COPY-ON-DEMAND still applies to old Prob-LIO tools/oracles.

---

# 34. Recommended files

Prefer:

```text
include/prob_livo/prob_imu_adapter.h
src/prob_livo/prob_imu_adapter.cpp

tests/prob_livo/test_i1_lifecycle_corrective.cpp
tests/prob_livo/test_i2_scheduler_time.cpp
tests/prob_livo/test_i2_imu_init.cpp
tests/prob_livo/test_i2_propagation.cpp
tests/prob_livo/test_i2_undistort.cpp
tests/prob_livo/test_i2_epoch_continuity.cpp
```

Avoid monolithic tests.

No `std::vector<bool>`.

---

# 35. Numerical tolerance policy

Use double precision.

Target ranges:
- state parity usually `1e-10` or tighter;
- covariance parity `1e-9` or tighter unless justified;
- undistorted XYZ `1e-9`–`1e-7` depending on identical/different interpolation implementation.

Report actual maxima.

Do not hide:
- millisecond timing offsets;
- centimeter geometric mismatch;
- gravity scaling mismatch.

---

# 36. Mandatory implementation order

```text
A. baseline build/test

B. I1 callback corrective
→ focused mutation test
→ all I1 tests
→ full build
→ commit
→ clean

C. source audit:
   FAST scheduler endpoint ownership
   canonical Super wrapper endpoint ownership

D. Super initialization adapter
→ test

E. scheduler time conversion
→ test

F. authoritative Prob propagation trace
→ test

G. Super undistortion
→ test

H. consecutive epoch + camera-cut seam
→ test

I. optional staged LIVMapper selector
→ default runtime non-interference test

J. full focused suite
→ full build
→ docs/evidence
→ commit
→ clean push
```

Do not implement an endpoint workaround before the source audit.

---

# 37. SPEC finalization

Update:

```text
spec/prob_livo/SPEC.md
```

If all pass:

```text
I0 = CLOSED / OWNER VERIFIED
I1 = CLOSED / OWNER VERIFIED
I2 = CLOSED/PASS — Owner audit pending
I3–I8 = NOT STARTED
```

Record:
- callback lifecycle contract;
- FAST scheduler time authority;
- Super initialization authority;
- endpoint mechanism;
- point curvature origin in each mode;
- I2 output frame;
- `last_lio_update_time` ownership;
- H2 implementation.

---

# 38. EVIDENCE_INDEX / HISTORY

Update `EVIDENCE_INDEX.md` with:
- start HEAD;
- I1 corrective SHA;
- reference SHA;
- scheduler audit;
- Super wrapper endpoint audit;
- exact endpoint decision;
- focused tests;
- max errors;
- full build;
- I2 SHA;
- final HEAD.

Update HISTORY with:
- why I1 needed `need_converge` in callback;
- why FAST scheduler is retained;
- why Super IMU initialization/propagation/undistortion replaces FAST authority;
- why I2 output is IMU-frame;
- why hybrid FAST-map + Prob-IMU is forbidden;
- why camera-cut remains FAST authority.

---

# 39. Gate summary

Report independently:

```text
G-P2.0   baseline build
G-I1.C1  callback convergence lifecycle
G-I1.C2  I1 math non-regression

G-I2.1   scheduler time contract
G-I2.2   exact epoch-end state
G-I2.3   Super initialization parity
G-I2.4   propagation trace parity
G-I2.5   undistortion geometric parity
G-I2.6   point/time identity
G-I2.7   consecutive epoch continuity
G-I2.8   camera-cut scheduler seam
G-I2.9   no accidental hybrid authority
```

Each gate must give:
- invariant;
- authoritative path;
- evidence;
- threshold;
- negative mutation;
- PASS/FAIL.

Do not substitute “tests pass”.

---

# 40. Commit policy

Suggested:

### Commit A
```text
fix(prob-livo): close Super observation lifecycle seam
```

### Commit B
```text
feat(prob-livo): add Super-native IMU propagation and undistortion adapter
```

### Commit C, only if naturally separated
```text
test(prob-livo): close scheduler-to-Super IMU seam
```

Final:
- clean worktree;
- fast-forward push to `origin/prob-livo`;
- no force push.

---

# 41. Final report format

## Agent State Consensus
- start HEAD
- origin
- reference SHA
- clean state
- prompt registration

## Baseline
- build/test status

# PART A — I1 Closure

## Defect confirmation
- old callback
- canonical Super contract
- production importance

## Corrective
- API
- callback sequence

## G-I1.C1
- `[F,F]`
- `[F,F,F,T]`
- mutation

## G-I1.C2
- numerical non-regression

## SO(3) hardening
- actual Super provenance
- error

## I1 Commit
- SHA
- clean status
- `I1 = CLOSED / OWNER VERIFIED`

# PART B — I2

## Scheduler Source Audit
- ONLY_LIO
- LIVO cut
- curvature origins
- IMU boundary
- last_lio_update_time

## Canonical Super Wrapper Audit
- init
- endpoint handling
- undistortion

## Endpoint Decision
Exactly how state reaches epoch_end.

## ProbImuAdapter
- API/files
- state ownership
- bookkeeping
- output frame

## G-I2.1–G-I2.9
Full independent evidence.

## Numerical Summary
- initialization errors
- final state errors
- covariance error
- trace error
- max undistorted XYZ error
- epoch timing error

## Runtime Scope Audit
Confirm:
- default FAST runtime unchanged;
- Prob IMU-frame cloud not fed to FAST LIO;
- no OctVox/P1–P4;
- no VIO change;
- no bag;
- no I3.

## Build
- focused tests
- full build
- RC

## Files / Commits
- files
- SHAs
- final HEAD
- clean
- push

## Final State

```text
I0 = CLOSED / OWNER VERIFIED
I1 = CLOSED / OWNER VERIFIED
I2 = CLOSED/PASS — Owner audit pending
I3–I8 = NOT STARTED

H0 FAST scheduler             = preserved
H1 shared x19/P19             = ProbESKF19
H2 IMU init/prop/undistortion = Super-native ProbImuAdapter
I2 output frame               = scan-end IMU frame
Prob-LIO map backend          = NOT STARTED

Next stage = I3 Prob-LIO P0–P4 backend + camera-OFF baseline
```

Do not begin I3.

---

# 42. Final CLOSE criteria

Prompt 2 closes only when:

```text
I1 callback lifecycle GREEN
all I1 parity still GREEN
I1 corrective committed clean

FAST scheduler timing audited
Super endpoint wrapper audited
Super init parity GREEN
Super propagation trace parity GREEN
Super undistortion parity GREEN
camera-cut seam GREEN
multi-epoch continuity GREEN
hybrid-authority guard GREEN
full build GREEN

No bag
No OctVox/P1–P4
No VIO behavior change
SPEC/EVIDENCE/HISTORY consistent
clean worktree
fast-forward push complete
```

If exact endpoint semantics cannot be proven, stop with I2 BLOCKED rather than inventing a timing rule.

---

# 43. Review contract

The final report is not acceptance authority.

Owner/reviewer will independently inspect:
- `need_converge` callback seam;
- actual FAST `sync_packages` timing/curvature ownership;
- actual canonical Super wrapper endpoint semantics;
- IMU initialization;
- ProbESKF19 trace authority;
- point interpolation and extrinsic direction;
- frame contract;
- multi-epoch bookkeeping;
- hybrid guard;
- negative tests;
- docs/evidence.

A correct IMU integrator is not enough if scheduler epoch boundaries are wrong.
