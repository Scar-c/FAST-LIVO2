# Prompt 5 Evidence — I3 First-Divergence Attribution

## Agent State Consensus

- Prompt: `prompts/prob_livo/prompt5_i3_divergence_attribution.md`, SHA256
  `63f7e02b17ac8d87d61977458dbf000fae71532d58fdc773ffd05fc22a953ac7`.
- Start HEAD: `fb159ca2848a3fe7ea87db314a905dd98b01de12`.
- Branch: `prob-livo`; no history rewrite or force push was used.
- The final evidence commit and pushed HEAD are recorded in the handoff below
  and in the final response. I3 is not marked Owner Verified.

## Legacy Workspace Handling

The owner workspace `/home/lc/prob_lio/src/Super-LIO` was treated as
read-only. It remained on branch `prob-livo`, HEAD
`9fc949f46291c0fa76e5b7cdb372c940eb4b3f6e`, with clean status. Its old
`build/` and `devel/` contain absolute paths referring to the pre-rename
workspace, so they were not used as algorithm evidence.

A no-hardlinks clone and detached diagnostic worktree were created at
`/tmp/prob_lio_diag/legacy_621acbd`, detached at
`621acbd8d9a67634d3782fe8ab56e8a49ec821a9`. It was rebuilt from source under
that path into `/tmp/prob_lio_diag/legacy_build` and
`/tmp/prob_lio_diag/legacy_devel`.

Both builds used `/usr/bin/c++` / GCC 9.4.0, Eigen 3.3.7, PCL 1.10, and TBB.
The legacy diagnostic target was Release with `-O3 -pthread -fexceptions
-flto=auto -fPIC -std=gnu++17`. The current catkin target was rebuilt cleanly
with its repository flags (`-O3 -DNDEBUG -march=native -mtune=native
-funroll-loops -fPIC -fopenmp -std=c++17`). The offline hot loops are TBB
controlled at 32 threads; no OMP cap was introduced.

## Legacy Live Oracle

The direct detached-worktree offline run used SHA `621acbd...`, the same bag,
NTU configuration, evaluator, and P4 overrides as the historical run. It
completed with return code zero in about 46.2 s:

```text
run: /tmp/prob_lio_diag/results/legacy_621acbd_p5_direct
rows: 3981
matched GT: 3329
ATE: 0.08883155405698266 m
trajectory SHA256: 259d3fbc16e5b918a75d5517c4f5feac0b29e40b7c6d5464f881185704595199
LiDAR/IMU: 3987 / 153347
```

The historical trajectory, live trajectory, and official evaluator result are
identical. G-P5.0 and G-P5.3 are therefore `EXACT`, not merely near-reproduced.

## Current Migrated Reproduction

The clean current run used the FAST host, `super_ntu_legacy` input semantics,
Prob-LIO P0–P4, camera OFF, and the repository-owned in-process offline
runner:

```text
run: results/prob_livo/runs/eee01_camera_off_p0_p5_migrated_clean
rows: 3981
matched GT: 3329
ATE: 0.09099574805341126 m
trajectory SHA256: d06e472b04f7d304d1462b30b2077766f62bf7047cd4247f909c96b3ca277f03
LiDAR/IMU: 3987 / 153347
callbacks/emitted/attempted/success/reject: 3987/3986/3986/3986/0
IMU_INIT/MAP_INIT/RUN: 1/4/3981
pending/discarded LiDAR: 1/0
TBB maximum parallelism: 32
runtime: 40 s wall-clock; sensor/wall speed: 10.313x
```

## Search Method

The final trajectory was first compared at sparse leading checkpoints and the
earliest mismatch was then selected for a one-epoch diagnostic trace. The
first mismatch is the first output row, so no later-epoch tail search was
needed: RUN index `0` (one-based trajectory row `1`) is already different.

The selected epoch was compared through the scheduler/input, initialization,
propagation/undistortion, downsample, measurement, and update boundaries; map
insert counts were audited from the final runtime counters. Temporary
diagnostics were enabled only for this epoch in the detached worktree and a
separate current diagnostic run; all trace code and the float-initialization
control were removed before the clean rebuild.

## First Divergence

```text
RUN epoch:          0 (first RUN row; trajectory row 1 one-based)
epoch start/end:    1609059013.6553447 / 1609059013.7657526
trajectory/filter:  1609059013.7636957
last equal stage:   S0 scheduler plus audited S1 input/preprocess contract
first different:   S2 predict/undistortion state; the cause is already
                    observable in the preceding IMU INIT stage
```

The first epoch has the same record-order input and observable point stream:
3987 LiDAR callbacks/3986 emitted epochs overall, and at the selected epoch
3479 undistorted points and 2433 downsampled points. The first downsampled
point is identical in both traces:
`(-133.78764343261719, -23.524204254150391, 36.610374450683594)`.

The initialization window contains 58 IMU samples and ends at
`1609059013.2591093`. The first post-update trajectory row is:

```text
legacy:   -0.0021442065481096506 -0.0028820030856877565 -0.00048168757348321378
migrated: -0.0022756750331101028 -0.0029468597047416756 -0.00046833760455423543
```

The full timestamps remain byte-identical across all 3981 paired rows. The
strict raw-world comparison is 3981/3981 pairs, timestamp delta RMSE/max
`0/0 s`, translation RMSE/median/max
`0.03319535524213128/0.03112573966899626/0.07739101605452314 m`, and rotation
RMSE/median/max
`0.00194296843409943/0.002017233514502395/0.009015083200112439 rad`.

## Causal Analysis

Classification at the first divergence is `NUMERIC_ONLY`.

The legacy authority defines `BASIC::scalar = float` in
`/tmp/prob_lio_diag/legacy_621acbd/src/basic/include/basic/alias.h:143`; its
ESKF nominal state and 18x18 covariance are consequently float. The migrated
host defines `StatesGroup` fields and `ProbESKF19` matrices as double in
`include/common_lib.h:130-220` and `include/prob_livo/prob_eskf19.h:37-47`.
The initialization implementation is
`src/prob_livo/prob_imu_adapter.cpp:89-153`, followed by double-precision
propagation in `src/prob_livo/prob_eskf19.cpp:114-195`.

The selected trace proves the difference before any map update: legacy and
migrated IMU means are already different at the displayed precision, as are
the gravity-alignment rotation and propagated state. For example:

```text
mean_acc legacy   = 0.84424549341201782  0.02315736748278141 -9.5785360336303711
mean_acc migrated = 0.84424540194971809  0.02315736831374595 -9.5785373819285429

S8 HtH[0,0] legacy   = 1173523.625
S8 HtH[0,0] migrated = 1173239.1195794202
```

Both update traces run two IEKF iterations and retain the same selected
epoch's point counts. The first difference is therefore not an input mode,
timestamp, lifecycle, map ownership, or TBB scheduling defect. The later
small differences in aggregate map-update counts are downstream consequences
of the changed double-precision state/geometry, not the first causal seam.
The current native/offline control is independently identical, so offline
serialization is not the cause.

## Minimal Reproducer

Selected-epoch fixture from the full `eee_01` replay: the first 58 IMU samples,
followed by the first RUN observation window ending at `1609059013.7657526`, with the audited
`super_ntu_legacy` first-frame semantics and the first 3981-row pipeline.

The bounded control mutation was diagnostic only:

```text
A canonical migrated double-precision initialization/ESKF:
  first row = -0.0022756750331101028 -0.0029468597047416756 -0.00046833760455423543
B migrated initialization means/alignment evaluated through float:
  first row = -0.0021589561959674012 -0.0029585045111730933 -0.00046494500801798106
legacy float state/ESKF:
  first row = -0.0021442065481096506 -0.0028820030856877565 -0.00048168757348321378
```

Mutation B moves the output toward the legacy oracle but does not reproduce it,
which is the expected negative control: initialization precision is causal,
but changing only initialization cannot turn the double-state implementation
into the legacy float-state implementation. The first-epoch counts, first
downsample point, two-iteration decision, and finite/valid QR/weight gates
remain stable in the bounded trace. No diagnostic mutation was retained.

## Fix

No production patch is warranted. The formulas, input contract, lifecycle,
map authority, gate decisions, and trajectory timestamps are preserved; the
observed seam is the deliberate host-side double state/covariance versus the
historical float ESKF representation. Forcing the current production path to
float solely for byte equality would reduce numerical precision and would
change the current host contract without evidence of a semantic defect.

The canonical offline runner remains TBB-based with maximum parallelism 32;
no OMP limit, parameter tuning, fallback, or runtime defensive log/validator
was added. Temporary trace instrumentation was removed and both source trees
were rebuilt cleanly.

## Final eee_01

The clean rerun is the migrated result above: 3981 rows, 3329 GT matches,
ATE `0.09099574805341126 m`, trajectory SHA
`d06e472b04f7d304d1462b30b2077766f62bf7047cd4247f909c96b3ca277f03`.

Against the legacy live oracle, the strict raw-world comparison is:

```text
paired rows: 3981
timestamp dt RMSE/max: 0 / 0 s
translation RMSE/median/max: 0.03319535524213128 / 0.03112573966899626 / 0.07739101605452314 m
rotation RMSE/median/max: 0.00194296843409943 / 0.002017233514502395 / 0.009015083200112439 rad
classification: I3_TRAJECTORY_CLOSE_NONIDENTICAL
```

## Gate Results

```text
G-P5.0: PASS — detached 621acbd live legacy oracle is EXACT.
G-P5.1: PASS — first RUN index 0 localized to INIT-caused S2 state divergence.
G-P5.2: PASS — bounded float-initialization control establishes causality and
              its negative control does not reproduce the full legacy state.
G-P5.3: PASS — historical and live legacy rows/matches/ATE/trajectory SHA are exact.
G-P5.4: PASS — clean migrated run is 3981/3329 with stable SHA and ATE.
```

## Scope Audit

- I4 was not started; no visual work was performed.
- No P5 association, camera/VIO runtime, parameter tuning, or sweep was used.
- No permanent per-point/per-neighbor diagnostic flood or defensive runtime
  expansion was added.
- The current offline runner is the repository-owned FAST-LIVO2 runner and
  does not invoke `super_ntu_legacy` or any legacy runtime; that name denotes
  only the audited input semantics.
- The original legacy workspace/history remained untouched; diagnostics were
  confined to the detached `/tmp` worktree and then removed.
- Do not start I4.

## Files / Commits

Tracked Prompt5 artifacts:

```text
prompts/prob_livo/prompt5_i3_divergence_attribution.md
spec/prob_livo/PROMPT5_EVIDENCE.md
spec/prob_livo/SPEC.md
spec/prob_livo/EVIDENCE_INDEX.md
spec/prob_livo/HISTORY.md
```

The ignored executable evidence bundle is
`results/prob_livo/runs/eee01_camera_off_p0_p5_migrated_clean/`; the detached
legacy bundle is under `/tmp/prob_lio_diag/results/`. The final evidence commit,
clean status, and pushed `origin/prob-livo` SHA are supplied in the final
handoff after commit.

## Final Classification

`I3_DIVERGENCE_NUMERIC_ACCEPTED`

I3 is causally attributed and reproducible, but remains `Owner audit pending`.
Do not start I4.
