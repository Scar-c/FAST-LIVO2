# Prompt 19 — Canonical Native-Compatible Prob-LIVO Stride Closure + Resource Comparison

## 0. Owner decision

Prompt18 revealed an important correction:

Native FAST-LIVO2 camera-driven LIVO scheduler uses **blind carry promotion**. Therefore:

- Prompt17 behavior (`blind carry + native-compatible late-point deskew`) is the canonical LIVO2-compatible Prob-LIVO timing semantics.
- Prompt18 P1 strict carry reclassification is an experimental scheduler policy, not a correctness restoration.
- Prompt18 P2 is a stride ablation on strict-reclassification policy.

The missing canonical experiment is:

`P0S = P0 + stride`

where:

- P0 = Prompt17 canonical Native-compatible Prob-LIVO
- blind carry preserved
- native late-point deskew preserved
- no strict carry reclassification
- no stride

and:

- P0S = same exact estimator/scheduler semantics as P0
- only deterministic camera stride added

Prompt19 must close this missing arm and record a fair resource comparison.

---

# 1. Primary questions

Prompt19 must answer:

1. Under canonical Native-compatible Prob-LIVO semantics, how does Oxford camera stride change ATE?
2. How does the same stride change:
   - estimator wall compute
   - estimator CPU compute
   - total offline wall
   - CPU utilization
   - RSS/PSS/USS
3. Compared with Native LIVO2:
   - does stride affect Prob and Native similarly?
   - which benefits more in accuracy?
   - which benefits more in compute/resource cost?
4. How do previously measured strict-reclassification P1/P2 compare with canonical P0/P0S?

P1/P2 are secondary ablation evidence only.

---

# 2. State consensus

Primary repo:

`/home/lc/super_livo/src/FAST-LIVO2`

Branch:

`prob-livo`

Expected Prompt18 final frontier:

`3974c77356315b735eb8c09cddd5422fcb2f710c`

Verify:
- branch
- local HEAD
- origin/prob-livo
- clean worktree
- ancestry from Prompt18
- exact commits after Prompt18

Native benchmark authority remains the accepted benchmark-safe Native branch used by Prompt18.

Do not modify Native estimator math.

---

# 3. Prompt registration

Create:

`prompts/prob_livo/prompt19_canonical_stride_resource_closure.md`

Update:

`prompts/README.md`

Create:

`spec/prob_livo/PROMPT19_EVIDENCE.md`

Create:

`spec/prob_livo/PROMPT19_RUN_LEDGER.csv`

Record prompt SHA256.

---

# 4. Canonical variant definitions

## P0 — Canonical Prob-LIVO / full-rate

Must match Prompt17 final LIVO timing semantics:

- Native-style blind carry promotion
- native-compatible late-point deskew
- no strict carry reclassification
- full camera rate
- corrected Oxford config
- 4-core benchmark resources

## P0S — Canonical Prob-LIVO / stride

Identical to P0 except:
- deterministic frozen camera stride enabled

## N0 — Native LIVO2 / full-rate

Accepted Native benchmark baseline.

## N1 — Native LIVO2 / stride

Same Native estimator/config/build as N0.
Only camera selection differs.

## P1 / P2 — secondary Prompt18 ablation

P1:
- strict carry reclassification
- full-rate

P2:
- strict carry reclassification
- stride

Do not modify P1/P2 behavior in Prompt19.

---

# 5. Oxford scope

Sequences:

- Church_05
- College_03
- Palace_01
- Quarter_01

Frozen Prompt18 stride policy:

- Church_05: stride 1
- College_03: stride 2
- Palace_01: stride 2
- Quarter_01: stride 2

Do not change stride based on Prompt19 results.

NTU is not rerun.
Prompt18 already established NTU camera/LiDAR rates are approximately matched and stride remains 1.

---

# 6. Canonical resource contract

All new Prompt19 comparison runs use the frozen formal environment:

- exactly 4 physical cores
- one logical CPU per physical core
- same affinity for Prob and Native
- Native MP=4
- Prob TBB=4
- Prob OMP=4
- Release build
- heavy diagnostics OFF
- debug logging OFF
- RViz OFF
- PCD save OFF
- sanitizer OFF

Compilation:
- use `-j4`
- do not use `-j16` or `-j32`

Do not use 32-core diagnostic runs for canonical resource comparison.

---

# 7. Oxford config authority

Use corrected Oxford author effective config from Prompt16/17.

At minimum:

- `filter_size_surf = 0.5`
- `lio/min_eigen_value = 0.01`

Preserve:
- topics
- extrinsics
- time offsets
- IMU init semantics
- point filtering
- scan timing
- visual config
- exposure/pyramid
- image adapter

No tuning.

---

# 8. P0 restoration gate

Prompt18 production contains strict carry-reclassification machinery.

Before P0S execution, ensure canonical P0 can still be run exactly.

Preferred:
- preserve P1/P2 implementation;
- expose an explicit frozen scheduler-policy switch:
  - `native_blind_carry`
  - `strict_reclass`

Default/canonical production setting must be:

`native_blind_carry`

If such a switch already exists, reuse it.

Do not delete Prompt18 P1/P2 code/evidence.

Hard gate:

P0 under `native_blind_carry` must reproduce Prompt17 canonical semantics.

Verify on one Oxford high-rate sequence before full runs.

---

# 9. Canonical P0 semantic gate

Use `Palace_01` or `Quarter_01`.

Under P0:

- blind carry active
- strict carry reclassification inactive
- native late-point deskew active
- backend rejected = 0
- complete trajectory
- visual active

Compare against Prompt17 canonical behavior.

Expected:
- same top-level event semantics
- same scheduler counts
- same blind-carry behavior
- large late-point populations are allowed because that is part of native blind-carry timing behavior

Do NOT classify canonical blind-carry late points as future-data leakage.

If P0 cannot reproduce Prompt17 semantics:
STOP FOR OWNER.

---

# 10. Stride input identity gate

P0S and N1 must consume the same retained camera timestamp sequence per sequence.

Required exact:
- selected camera count
- timestamp order
- timestamp values
- SHA256

Hard gate:

`camera_timestamp_sha(P0S) == camera_timestamp_sha(N1)`

For Church stride=1:
P0/P0S and N0/N1 camera timestamp sequences must each be identical.

Any mismatch:
`STRIDE_INPUT_IDENTITY_FAIL`

STOP.

---

# 11. Formal repetitions

Canonical new runs:

## P0
Run/reuse 3 canonical 4-core repetitions per Oxford sequence.

Reuse Prompt17 artifacts only if:
- exact corrected Oxford config
- exact canonical P0 semantics
- exact 4-core resource mode
- complete metrics for ATE/resource comparison

If resource artifacts are missing or measurement envelope differs:
rerun P0.

## P0S
3 canonical runs per Oxford sequence.

## N0
Reuse Prompt15/18 canonical N0 ×3 only if resource artifacts are complete and measurement-compatible.
Otherwise rerun.

## N1
Reuse Prompt18 N1 ×3 only if resource artifacts are complete and measurement-compatible.
Otherwise rerun.

Primary maximum:
4 sequences × 4 arms × 3 = 48 runs.

Existing valid runs may reduce execution count.

P1/P2:
reuse Prompt18 only.
No rerun unless a required comparison field is missing and can be completed without semantic change.

---

# 12. Accuracy metrics

Per run:
- ATE RMSE
- valid GT matches
- trajectory rows
- start/end timestamps
- evaluator RC
- completion status

Per sequence × arm:
- mean
- std
- median
- min
- max

Primary stride effects:

## Prob canonical

`ΔATE_P0S = ATE(P0S) - ATE(P0)`

## Native

`ΔATE_N1 = ATE(N1) - ATE(N0)`

Also compute:

### Full-rate gap
`gap_full = ATE(P0) - ATE(N0)`

### Stride gap
`gap_stride = ATE(P0S) - ATE(N1)`

---

# 13. Negative-control interpretation

Church_05 has stride=1.

Therefore:
- P0 and P0S have identical camera input semantics
- N0 and N1 have identical camera input semantics

Any measured ATE difference on Church is NOT a causal stride effect.

Treat it as:
- repeat-run numerical variability
- run-order noise
- accepted visual nondeterminism

Use Church as no-treatment noise reference.

For College/Palace/Quarter:
interpret stride deltas relative to this no-treatment variation scale.

---

# 14. Runtime measurement

Use the same top-level estimator measurement seam frozen in Prompt15.

Record:

## Estimator wall compute
- total estimator wall seconds
- wall / authoritative estimator epoch

## Estimator process CPU compute
- total estimator CPU seconds
- CPU / estimator epoch
- CPU / bag-duration second

## End-to-end offline wall
- total wall runtime
- bag duration
- realtime factor / speedup

Bag I/O/decode remains separate where measurable.

Do not compare unlike internal component timers as primary runtime metric.

---

# 15. CPU metrics

Record:

- whole-process CPU time total
- average raw process CPU utilization
- peak raw process CPU utilization
- normalized 4-core utilization

Primary cost metric:

`estimator CPU-time / estimator epoch`

Also report:
- camera frames retained
- VIO calls
- visual commits

to explain CPU change.

---

# 16. Memory metrics

Use the same Prompt15 memory methodology.

Sample process memory at the same fixed interval.

Record:
- Peak RSS
- Final RSS
- Peak PSS
- Final PSS
- Peak USS
- Final USS
- memory growth

Timestamp-progress checkpoints:
- 10%
- 25%
- 50%
- 75%
- 90%
- end
- peak

Label:

`OFFLINE_SYSTEM_PROCESS_MEMORY`

because offline process may include:
- bag reader
- image decode
- adapter
- estimator

Do not call these pure estimator/map memory.

Do not mix Prompt15 estimator-only online memory with Prompt19 offline system memory percentages.

---

# 17. Canonical stride resource effect

Per high-rate Oxford sequence:

## Prob

- wall ratio: `P0S / P0`
- CPU ratio: `P0S / P0`
- Peak USS ratio: `P0S / P0`
- Final USS ratio: `P0S / P0`

## Native

- wall ratio: `N1 / N0`
- CPU ratio: `N1 / N0`
- Peak USS ratio: `N1 / N0`
- Final USS ratio: `N1 / N0`

Interpret:
- <1 = stride reduces resource usage
- >1 = stride increases it

Church stride=1 ratios are no-treatment repeatability controls.

---

# 18. Visual workload explanation

For every arm record:

- input camera messages
- retained camera frames
- camera epochs
- VIO calls
- visual state commits
- reference commits/equivalent
- visual map participation

Compute:

- retained camera ratio
- VIO call ratio
- visual commit ratio

Relate resource changes to actual visual workload.

---

# 19. Blind-carry × stride factorial interpretation

Using existing Prompt18 P1/P2 and Prompt19 P0/P0S:

| Policy | Blind carry | Strict reclass | Stride |
|---|---:|---:|---:|
| P0 | yes | no | no |
| P0S | yes | no | yes |
| P1 | no | yes | no |
| P2 | no | yes | yes |

For each high-rate sequence report:

### Blind-carry effect at full rate
`P0 vs P1`

### Blind-carry effect under stride
`P0S vs P2`

### Stride effect with blind carry
`P0 vs P0S`

### Stride effect with strict reclassification
`P1 vs P2`

Do not over-interpret beyond tested configurations.

Palace is expected to be especially informative because Prompt18 showed a large P1→P2 change.

---

# 20. Native comparison

Compare:

- P0 vs N0 at full rate
- P0S vs N1 under stride

These are the primary canonical architecture comparisons.

Do not use P1/P2 against Native as the primary claim.

---

# 21. Online/offline verification

At minimum on one high-rate Oxford sequence:

## P0S Prob
normal online vs offline

Compare:
- selected camera timestamps
- LiDAR epochs
- camera epochs
- backend counters
- visual calls/commits
- trajectory under accepted Prob-LIVO parity

## N1 Native
verify accepted event-semantic online/offline contract if current artifact does not already provide it.

Any event-source mismatch:
FAIL.

---

# 22. Run order

Predeclare canonical run order before new runs.

Use balanced/rotating order across:
- P0
- P0S
- N0
- N1
- repetitions

Do not always run no-stride before stride.

Retain all valid runs.
No best-run selection.

---

# 23. Failure classifications

At minimum:

- `VALID`
- `STRIDE_INPUT_IDENTITY_FAIL`
- `P0_CANONICAL_RESTORE_FAIL`
- `VISUAL_INACTIVE_FAIL`
- `EVENT_SOURCE_MISMATCH`
- `INCOMPLETE_TRAJECTORY`
- `EVALUATOR_FAIL`
- `EXECUTION_FAIL`
- `CONTAMINATED`

No tuning to rescue failures.

---

# 24. Hard prohibitions

No:

- strict carry reclassification as canonical default
- adaptive stride
- per-sequence stride tuning beyond frozen Prompt18 policy
- visual threshold changes
- LiDAR/IMU config changes
- map config changes
- scheduler redesign
- P5
- I8
- H1/H2
- NTU stride experiment
- Prompt15 full rerun
- unequal core budgets
- 32-core canonical timing/resource runs

---

# 25. Required final tables

## A. Canonical accuracy

| Sequence | P0 | P0S | ΔProb stride | N0 | N1 | ΔNative stride |
|---|---:|---:|---:|---:|---:|---:|

## B. Canonical Prob/Native gap

| Sequence | P0-N0 full-rate | P0S-N1 stride |
|---|---:|---:|

## C. Canonical runtime

| Sequence | P0 wall/epoch | P0S wall/epoch | Ratio | N0 | N1 | Ratio |
|---|---:|---:|---:|---:|---:|---:|

## D. Canonical CPU

| Sequence | P0 CPU/epoch | P0S CPU/epoch | Ratio | N0 | N1 | Ratio |
|---|---:|---:|---:|---:|---:|---:|

## E. Canonical memory

| Sequence | Arm | Peak PSS | Final PSS | Peak USS | Final USS |
|---|---|---:|---:|---:|---:|

## F. Visual workload

| Sequence | Arm | Retained cameras | VIO calls | Visual commits |
|---|---|---:|---:|---:|

## G. Carry × stride factorial ATE

| Sequence | P0 | P0S | P1 | P2 |
|---|---:|---:|---:|---:|

Each ATE cell:
mean ± std, median, min/max.

---

# 26. Required conclusions

Final report must explicitly answer:

1. Is Prompt17 P0 reproducibly the canonical Native-compatible Prob-LIVO scheduler behavior?
2. What is the ATE effect of stride on canonical Prob-LIVO: `P0 -> P0S`?
3. What is the ATE effect of stride on Native: `N0 -> N1`?
4. What is canonical Prob-vs-Native gap:
   - full-rate
   - stride
5. How much compute is saved by stride:
   - wall
   - CPU
6. How much memory is changed by stride:
   - Peak/Final PSS
   - Peak/Final USS
7. How much of resource change is explained by fewer visual frames/VIO calls?
8. Does strict carry reclassification interact strongly with stride:
   compare P0/P0S/P1/P2.

---

# 27. Evidence

Create:

`spec/prob_livo/PROMPT19_EVIDENCE.md`

Required sections:

A. State consensus
B. Canonical P0 restoration
C. Oxford camera policy authority
D. Camera timestamp identity
E. Canonical run ledger
F. Accuracy
G. Runtime
H. CPU
I. Memory
J. Visual workload
K. P0/P0S canonical stride effect
L. N0/N1 native stride effect
M. P0/P0S/P1/P2 factorial interpretation
N. Online/offline verification
O. Build/resource authority
P. Failures/deviations
Q. Final owner answers

---

# 28. Commit discipline

Suggested:

1. `feat(prob-livo): expose native blind-carry scheduler policy`
   only if a policy switch is required

2. `test(prob-livo): verify canonical blind-carry restoration`

3. `test(benchmark): close canonical Oxford stride comparison`

4. `docs(prob-livo): record Prompt19 evidence`

Do not change canonical estimator math.

Push `origin/prob-livo`.

If Native stride tooling requires a branch change, commit only benchmark/input-selection tooling to the Native benchmark branch.

---

# 29. Final statuses

Only:

### `PROMPT19 CLOSED — CANONICAL STRIDE + RESOURCE COMPARISON COMPLETE`

Requires:
- P0 canonical semantics reproduced
- P0S completed
- N0/N1 valid
- selected camera timestamps matched
- 3-run canonical ATE summaries
- runtime/CPU/memory collected under same 4-core envelope
- carry×stride factorial interpretation completed
- no event-source contamination

### `PROMPT19 ACCURACY CLOSED / RESOURCE OWNER DECISION REQUIRED`

Use only if semantic/ATE experiment closes but fair resource measurement cannot be recovered or reproduced.

### `PROMPT19 OWNER DECISION REQUIRED`

For canonical scheduler-authority or input-identity ambiguity.

### `PROMPT19 FAILED`

For contaminated execution.

At final:

**STOP. Do not automatically rerun Prompt15, do not tune Oxford visual parameters, and do not extend stride to NTU.**
