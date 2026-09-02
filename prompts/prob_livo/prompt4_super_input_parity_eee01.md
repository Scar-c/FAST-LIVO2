# Prob-LIVO Integration — Prompt 4
## I3 Corrective: Restore Super/Prob-LIO NTU Input Semantics + eee_01 Parity ATE

> This round does **not** start I4.
>
> The goal is to repair the experimental control for I3:
>
> 1. restore the **legacy Super/Prob-LIO NTU input semantics** inside the FAST-LIVO2 host;
> 2. explain the missing LiDAR epochs/trajectory rows;
> 3. run one clean full-bag `eee_01` camera-OFF parity experiment;
> 4. compare it against the previous canonical Prob-LIO P4 trajectory/ATE;
> 5. preserve the existing FAST-native `0.052901597 m` run as a separate host/frontend result.
>
> Do not tune parameters and do not proceed to visual integration.

---

# 0. Owner decision / current interpretation

Current accepted interpretation:

```text
Legacy canonical Prob-LIO:
    Super host
    Super NTU input/preprocess/timing semantics
    Prob-LIO P0–P4
    eee_01 ATE ≈ 0.088831554 m

Current FAST-host result:
    FAST-LIVO2 host
    FAST NTU input/preprocess/timing semantics
    Prob-LIO P0–P4
    eee_01 ATE = 0.05290159739482509 m
```

The current `0.05290 m` result is valid as a **FAST-native-input result**, but it is not a valid migration-parity control because input semantics differ.

The missing control is:

```text
FAST-LIVO2 host
+ Super/legacy NTU input semantics
+ same Prob-LIO P0–P4 backend
+ camera OFF
```

This round must produce that result.

Do not claim the `0.05290 m` result is a Prob-P4 algorithm improvement.

---

# 1. Repository / frontier consensus

Active repo:

```text
~/super_livo/src/FAST-LIVO2
```

Expected branch:

```text
prob-livo
```

Expected starting frontier:

```text
df83f635134a1b1915568dcfa491eaa7d37dc22a
```

Prob-LIO reference:

```text
~/super_livo/ref/Super-LIO
```

Expected reference:

```text
branch: prob-lio
HEAD:   9fc949f46291c0fa76e5b7cdb372c940eb4b3f6e
```

Legacy experiment/evaluator workspace:

```text
~/prob_lio/src/Super-LIO
```

Target bag:

```text
~/super_livo/bag/NTU/eee_01/eee_01.bag
```

At startup verify:

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
- host worktree clean;
- start HEAD matches Owner frontier;
- reference clean/read-only;
- no force push;
- no history rewrite.

Register exact prompt:

```text
prompts/prob_livo/prompt4_super_input_parity_eee01.md
```

---

# 2. Baseline build/test before edits

Use:

```bash
source /home/lc/design_ws/devel/setup.bash
cd /home/lc/super_livo
cmake -S src -B build
cmake --build build \
  --target prob_livo_backend prob_livo_i1_tests \
  prob_livo_i2_tests prob_livo_i3_tests fastlivo_mapping -j2
```

Run I1/I2/I3 focused tests.

### HARD GATE G-P4.0

Require:
- clean start;
- build PASS;
- current focused tests PASS.

Do not debug later commits or unrelated branches.

---

# 3. Preserve the existing FAST-native run

Do not overwrite or reinterpret:

```text
results/prob_livo/runs/eee01_camera_off_p0_p4_correction/
```

Current FAST-native evidence remains:

```text
ATE = 0.05290159739482509 m
trajectory rows = 3595
matched GT = 3016
config = FAST-LIVO2 NTU semantics
```

Rename/relabel documentation if necessary so this result is clearly classified as:

```text
FAST_HOST_NATIVE_INPUT
```

not:

```text
SUPER_INPUT_PARITY
```

No new FAST-native full-bag rerun is required in Prompt 4 unless a documentation-only comparison requires it.

---

# 4. First source audit: define “Super NTU input semantics” from production source

Do not equate “Super semantics” with merely loading `Super-LIO/config/NTU.yaml`.

Audit the exact active legacy production path at reference commit:

```text
9fc949f46291c0fa76e5b7cdb372c940eb4b3f6e
```

At minimum inspect:
- NTU YAML;
- LiDAR callback;
- Ouster preprocessing;
- point field/time interpretation;
- `filter_rate`;
- blind/range filter;
- maxrange behavior;
- point ordering;
- scan start/end timestamp semantics;
- where/how relative point time is stored;
- any scan sorting;
- any first/last point removal;
- any ring/tag-specific filter;
- any NaN/range logic.

Also audit the corresponding FAST host production path.

Produce a table:

```text
semantic                legacy Super             FAST-native
-------------------------------------------------------------
lidar timestamp origin
lidar_time_offset
point relative time
point sorting
blind
max range
stride/filter_rate
NaN handling
range test
scan start
scan end
point count semantics
```

Only differences proved by source are allowed into the corrective.

---

# 5. Known differences that must be verified

The current audit already strongly suggests:

```text
legacy Super:
    lidar_time_offset = 0 / no equivalent offset
    blind = 2.0 m
    maxrange = 150.0 m
    filter_rate = 3

FAST-native:
    lidar_time_offset = -0.1 s
    blind = 1.0 m
    point_filter_num = 3
    no equivalent legacy 150 m cut in current path
```

Verify these from exact source/config before coding.

Do not assume these are the only differences.

---

# 6. Implement a named input-semantics mode

Add one explicit runtime choice, conceptually:

```text
prob_livo/input_semantics:
    super_ntu_legacy
    fast_native
```

or equivalent.

Requirements:

### `fast_native`
Must preserve the already-measured FAST-LIVO2 behavior.

### `super_ntu_legacy`
Must reproduce the actual legacy Super preprocessing/timing semantics required for migration parity.

Do not create a giant parallel preprocessor.

Prefer the smallest clean adaptation:
- parameterize timestamp offset;
- parameterize blind/maxrange;
- reproduce source-point acceptance semantics;
- reproduce point-time interpretation/order where different.

If exact Super Ouster semantics cannot be represented by the existing FAST preprocessor without semantic distortion, implement a small dedicated compatibility helper used only by `super_ntu_legacy`.

No fallback switching at runtime.

---

# 7. Production philosophy

Owner explicitly rejects defensive-programming bloat.

Production path:
- simple;
- deterministic;
- directly follows frozen semantics.

Tests:
- strict;
- adversarial;
- detailed.

Do not add repeated runtime validators, per-point debug logs, or expensive diagnostics to the hot path.

---

# 8. HARD GATE G-P4.1 — legacy preprocessing parity

Create deterministic Ouster-like PointCloud2/point fixtures that exercise the **real production compatibility path**.

Compare legacy Super reference preprocessing against FAST-host `super_ntu_legacy`.

Required parity:
- accepted source point identities;
- accepted point count;
- XYZ/intensity;
- relative point time;
- ordering;
- blind cutoff;
- maxrange cutoff;
- stride/filter-rate behavior.

Fixtures must include:
- `< blind`;
- exactly/near blind;
- ordinary range;
- `>150 m`;
- mixed source indices;
- nonmonotonic input point times if relevant;
- invalid/NaN point if canonical source handles it.

### Negative mutations

Tests must fail if:
1. blind is 1.0 instead of 2.0;
2. 150 m max range is omitted;
3. FAST-native point selection substitutes legacy selection;
4. point-time sorting changes legacy source order when legacy does not sort;
5. filter stride is off by one.

---

# 9. HARD GATE G-P4.2 — LiDAR timestamp parity

For the same synthetic ROS LiDAR message:

```text
legacy Super scan time
==
FAST-host super_ntu_legacy scan time
```

within binary64 timestamp tolerance.

Prove:
- no `-0.1 s` shift in legacy mode;
- scan start/end derive from the same source semantics;
- point absolute acquisition times match.

### Negative mutation

Set `lidar_time_offset=-0.1` under `super_ntu_legacy`.

The gate must fail.

This gate is separate from the backend and evaluator.

---

# 10. Explain `3987 → 3602`

This is a mandatory I3 corrective.

The bag has:

```text
LiDAR messages = 3987
```

Legacy canonical trajectory:

```text
rows = 3981
```

Current FAST-native run:

```text
successful_epochs = 3602
trajectory rows = 3595
```

Before the Super-semantics full run, establish a lightweight accounting chain.

Required counters, preferably at event/epoch granularity only:

```text
lidar_callbacks_received
scheduler_epochs_emitted
backend_epochs_attempted
backend_epochs_success
backend_epochs_rejected

imu_init_epochs
map_init_epochs
run_epochs
trajectory_rows
```

If backend rejects epochs, classify only with a small enum/counter, e.g.:

```text
NO_LOOKAHEAD
STALE_EPOCH
EMPTY_CLOUD
INIT_NOT_READY
OTHER
```

Do not emit per-epoch verbose logs unless a failure requires local diagnosis.

The counters must reflect actual production paths, not reconstructed guesses.

---

# 11. HARD GATE G-P4.3 — epoch accounting closure

For both:
- existing FAST-native evidence if counters are already authoritative;
- new Super-input parity run;

prove a conservation-style accounting:

```text
LiDAR callback
→ scheduler emit / scheduler not emit
→ backend attempt
→ backend success/reject
→ lifecycle class
→ trajectory output
```

Every missing LiDAR message between 3987 and final output count must be explainable by an explicit stage/category.

No unexplained remainder is allowed.

This does **not** require every LiDAR message to produce a trajectory pose if canonical lifecycle intentionally omits some.

It requires the omissions to be understood.

---

# 12. Re-audit map-init temporal semantics

Legacy Super explicitly updates observation time during map init.

Audit exact source:

```text
stateWaitMapInit
map_init(...)
SetLastObsTime(...)
first normal Propagation_Undistort
```

Compare to current Prob backend.

If the current one-time bridge is mathematically/source-equivalent, prove it with a deterministic multi-epoch parity test.

If it is not equivalent, make the smallest semantic correction before the canonical run.

Do not redesign lifecycle.

### HARD GATE G-P4.4

From the same IMU/LiDAR epoch sequence:
- legacy Super;
- FAST-host Super-input mode;

require same:
- map-init epoch count;
- last observation boundary entering first RUN epoch;
- first RUN propagated state/covariance;
- first RUN undistorted cloud.

Negative mutation:
- omit map-init observation-time advancement;
- gate must fail.

---

# 13. Replace `std::vector<bool>`

Current backend uses `std::vector<bool>` for an effect mask.

Replace it with a normal POD representation such as:

```text
std::vector<uint8_t>
```

or equivalent.

Do not change semantics.

Focused test:
- same mask decisions;
- same accepted count;
- same H/r accumulation.

This is hygiene, not an ATE attribution.

---

# 14. Freeze the Super-input eee_01 effective config

Create a dedicated canonical config/overlay for this experiment.

Do not mutate `config/NTU_VIRAL.yaml` in a way that destroys FAST-native reproducibility.

Recommended separate file/overlay, e.g.:

```text
config/prob_livo/NTU_eee01_super_legacy.yaml
```

or a runner-generated explicit overlay tracked in evidence.

It must encode the **effective production semantics**, including at minimum:

```text
backend                 = Prob-LIO P0–P4
camera/VIO              = OFF
P5                      = OFF

input_semantics         = super_ntu_legacy
lidar_time_offset       = legacy value
blind                   = legacy value
maxrange                = legacy value
filter_rate             = legacy value

voxel size
HKNN
QR thresholds
P1 sensor noise
P2 map pose covariance mode
P3 enable
P4 mode
Super legacy association
IMU noise
gravity
extrinsic
IESKF iterations
quit epsilon
```

No tuning.

---

# 15. Exact historical reference

Use legacy evidence as authority.

Verify and record:

```text
legacy run:
~/prob_lio/src/Super-LIO/results/prob_lio/p11_smoke_eee_p4_lc/

old algorithm SHA:
621acbd8d9a67634d3782fe8ab56e8a49ec821a9

old trajectory SHA256:
259d3fbc16e5b918a75d5517c4f5feac0b29e40b7c6d5464f881185704595199

old trajectory rows:
3981

old matched GT:
3329

old ATE:
0.08883155405698266 m
```

If the actual legacy evidence disagrees with any of these values, use the evidence and explain the correction.

Also record old effective-config identity/provenance.

---

# 16. Canonical runner for Prompt 4

Create/use a dedicated run ID, e.g.:

```text
eee01_camera_off_p0_p4_super_input_parity
```

Requirements:
- clean committed source;
- one mapper;
- one ROS master;
- one rosbag playback;
- camera topic not replayed/subscribed for the algorithm;
- IMU + LiDAR only for estimator;
- same evaluator semantics as old Prob-LIO;
- effective config snapshot;
- HEAD/dirty state;
- bag hash;
- counters;
- trajectory SHA.

No sweep.

No preliminary parameter trial on full eee_01.

---

# 17. HARD GATE G-P4.5 — full Super-input eee_01 run

Run once after all source/tests are committed and clean.

Require:
- full bag playback completion;
- node completion;
- evaluator completion;
- finite trajectory;
- complete accounting counters;
- no duplicate processes;
- camera/VIO inactive;
- FAST VoxelMapManager inactive;
- Prob P0–P4 active;
- P5 inactive.

Record:
- wall time;
- rows;
- time range;
- lifecycle counts;
- backend rejects;
- trajectory hash.

---

# 18. Compare against legacy trajectory on exact/near-exact timestamps

The previous comparator is too permissive for migration parity.

Build a stricter comparison.

First try exact timestamp identity within a tight tolerance appropriate for binary64 ROS seconds, e.g.:

```text
|dt| <= 1e-6 s
```

or a source-justified threshold.

Report:
- exact/near-exact paired rows;
- unmatched old rows;
- unmatched new rows;
- timestamp RMSE/max;
- translation RMSE/median/max;
- rotation RMSE/median/max.

Do not interpolate one trajectory onto the other for the primary parity metric.

Interpolation may be reported only as a secondary diagnostic.

---

# 19. HARD GATE G-P4.6 — migration trajectory parity classification

Classify the Super-input result.

Suggested semantic classes:

### `SUPER_INPUT_TRAJECTORY_EQUIVALENT`
Use only if:
- epoch accounting is aligned/understood;
- timestamps align nearly exactly;
- trajectory differences are numerical/small implementation noise;
- ATE and matched-GT set are essentially the same.

### `SUPER_INPUT_TRAJECTORY_NEAR_PARITY`
Use if:
- same input semantics and complete accounting;
- small but material nonzero host/scheduler difference remains;
- ATE remains close to old canonical;
- residual seam is explicitly identified.

### `SUPER_INPUT_SEMANTIC_MISMATCH`
Use if:
- rows/timestamps differ materially without canonical explanation;
- trajectory error is material;
- ATE differs materially;
- or a production semantic seam remains unresolved.

Do not use the old loose `0.5 m / 0.5 rad` threshold.

---

# 20. Official ATE comparison

Run the same evaluator semantics used by old canonical Prob-LIO.

Report:

```text
legacy Super-host Prob-P4:
    ATE
    trajectory rows
    matched GT

FAST-host Prob-P4 + Super-input:
    ATE
    trajectory rows
    matched GT

delta ATE absolute
delta ATE percent
```

This is the required result requested by Owner.

---

# 21. Preserve FAST-native comparison separately

At the end produce a three-row table:

```text
A. Legacy Super-host Prob-P4
B. FAST-host Prob-P4 + Super input semantics
C. FAST-host Prob-P4 + FAST-native input semantics
```

At minimum report:
- input semantics;
- lidar offset;
- blind;
- max range;
- trajectory rows;
- matched GT;
- ATE.

Do not infer causal attribution yet.

Prompt 4's purpose is to create the missing parity control.

---

# 22. Optional attribution only if parity is already closed

Do **not** automatically launch more full-bag ablations.

If B reproduces A closely, record as a future hypothesis:

```text
FAST-native improvement candidate:
    lidar_time_offset=-0.1
    preprocess blind/range differences
```

Do not run the single-variable offset/blind ablations in this Prompt unless Owner separately authorizes them.

If B does **not** reproduce A, stop and diagnose the remaining migration seam.

---

# 23. HARD GATE G-P4.7 — no tuning / no contamination

For the new Super-input parity run:

```text
camera/VIO OFF
P5 absent
Prob P4 ON
Super legacy association
Super-input semantics
no sweep
no ATE-driven parameter changes
```

No reuse of FAST-native `-0.1` offset inside the parity mode.

---

# 24. Existing I3 gates — status correction

Do not simply preserve the old prose that I3 is closed.

Update SPEC/EVIDENCE to distinguish:

```text
I3 backend functional             = GREEN
FAST-native eee_01                = VALID RESULT
Super-input migration parity      = ACTIVE
Owner Verified                    = NO until Prompt 4 audit
```

If Prompt 4 closes all parity gates, final Agent status may be:

```text
I3 = CLOSED/PASS — Owner audit pending
```

Do not write `OWNER VERIFIED` for I3.

---

# 25. Commit policy

Suggested commits:

### Commit A — parity infrastructure/correctives

```text
fix(prob-livo): restore Super NTU input parity semantics
```

Includes:
- input semantics mode;
- scheduler/backend accounting;
- map-init temporal correction if required;
- `vector<bool>` cleanup;
- focused tests.

### Commit B — tests/evaluator if naturally separate

```text
test(prob-livo): close Super-input migration parity gates
```

### Commit C — evidence after canonical run

```text
docs(prob-livo): record Super-input eee01 parity baseline
```

Canonical full bag must run from clean committed source.

No force push.

---

# 26. Required tests / gates summary

Report independently:

```text
G-P4.0 baseline build/tests

G-P4.1 legacy preprocessing parity
G-P4.2 LiDAR timestamp parity
G-P4.3 epoch accounting closure
G-P4.4 map-init temporal parity
G-P4.5 full Super-input eee_01 run
G-P4.6 migration trajectory parity
G-P4.7 no tuning/visual/P5 contamination
```

Each must state:
- exact invariant;
- authoritative source;
- production path exercised;
- numerical/identity evidence;
- negative mutation;
- PASS/FAIL.

---

# 27. Final report format

## Agent State Consensus
- start HEAD
- final HEAD
- branch/origin
- reference SHA
- clean state
- prompt registration

## Why Prompt 4 Exists
State clearly:

```text
0.052901597 m is FAST-native-input evidence,
not a valid migration-parity comparison to legacy Super-host Prob-P4.
```

## Legacy vs FAST Source Audit
Provide the exact table for:
- timestamp
- offset
- blind
- maxrange
- stride
- point time
- ordering
- other proven differences

## Implementation
- input-semantics mode
- files/functions
- no duplicate frontend
- map-init temporal change if any
- `vector<bool>` cleanup

## G-P4.1
Preprocess parity.

## G-P4.2
Timestamp parity.

## G-P4.3
Epoch accounting.

Show full accounting for:
- bag LiDAR callbacks;
- scheduler emits;
- backend attempts;
- successes/rejects;
- init/map/run;
- trajectory rows.

Explain every omitted message/row.

## G-P4.4
Map-init temporal parity.

## Super-Input Effective Config
- config file/path
- SHA
- effective values
- provenance

## Canonical eee_01 Run
- run dir
- clean HEAD
- bag hash
- completion
- rows
- matched GT
- runtime
- counters
- trajectory hash

## Legacy Reference
- old SHA
- old config
- old trajectory hash
- rows
- matched GT
- ATE

## Strict Trajectory Comparison
Primary comparison with no interpolation:
- paired rows
- unmatched rows
- dt RMSE/max
- translation RMSE/median/max
- rotation RMSE/median/max

## ATE Comparison

```text
Legacy Super-host Prob-P4:
FAST-host + Super-input:
absolute delta:
percent delta:
matched GT old/new:
rows old/new:
```

## Three-Way Result Table

```text
A legacy Super-host
B FAST-host + Super input
C FAST-host + FAST-native input
```

Include key input semantics and ATE.

## Classification

Exactly one for B:

```text
SUPER_INPUT_TRAJECTORY_EQUIVALENT
SUPER_INPUT_TRAJECTORY_NEAR_PARITY
SUPER_INPUT_SEMANTIC_MISMATCH
```

## G-P4.5–G-P4.7
PASS/FAIL.

## Scope Audit
Confirm:
- no visual/I4;
- no P5;
- no parameter sweep;
- FAST-native run preserved separately;
- no large logs/bags committed.

## Files / Commits
- changed files
- commit SHAs
- final HEAD
- worktree clean
- fast-forward push

## Final State

If parity closes:

```text
I0 = CLOSED / OWNER VERIFIED
I1 = CLOSED / OWNER VERIFIED
I2 = CLOSED / OWNER VERIFIED
I3 = CLOSED/PASS — Owner audit pending
I4–I8 = NOT STARTED

Legacy Super-host Prob-P4 baseline         = recorded
FAST-host + Super-input parity baseline    = recorded
FAST-host + FAST-native-input result       = preserved separately
```

If parity fails:

```text
I3 = OPEN / SUPER_INPUT_SEMANTIC_MISMATCH
I4 = BLOCKED
```

Do not start I4.

---

# 28. Final CLOSE criteria

Prompt 4 is complete only if:

```text
Super/legacy NTU input semantics are source-defined
host compatibility mode reproduces them
preprocess parity GREEN
timestamp parity GREEN
epoch accounting has no unexplained remainder
map-init temporal seam GREEN

clean committed source
one full eee_01 Super-input run completed
same evaluator used
strict trajectory comparison produced
Super-input ATE produced
three-way A/B/C table produced

no tuning
no visual
no P5
worktree clean
fast-forward push complete
```

The target is not to force the new ATE to equal `0.088831554`.
The target is to create a scientifically valid migration control and explain any remaining difference.
