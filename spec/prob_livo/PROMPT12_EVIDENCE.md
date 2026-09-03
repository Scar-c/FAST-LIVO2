# Prompt12 Evidence

Status: `PROMPT12 DIAGNOSIS CLOSED / OWNER DECISION REQUIRED`

## Scope and entry state

This run stayed within Prompt12: N0 memory sanity, native runner integrity,
shutdown diagnosis, reference-counter semantics, the compile-time single-thread
matrix, final native LIVO comparison, and native LIO regression. No I8, P5,
multi-dataset benchmark, parameter tuning, scheduler rewrite, or second
numerical adaptation was started.

The native worktree was `/tmp/prompt11_fast_livo2_native`, branch
`prompt11-native-offline`, with clean primary `prob-livo` worktree at entry.
The Prompt11 parent was `be00d49d530157e93bb53282cab1007b13031aa2`; the native
baseline used for N0 was the clean Prompt10 authority
`0d2c0346107b75b59934975adec9a6eeeb913c64`.

Prompt source SHA256:

`b9d569e49c229cd5455ced1da3dc7e41c9b0047a4c3852161604607c1659cf53`

## G-P12.1 — N0 memory sanity

Fresh N0-FN normal-online run:

`results/prob_livo/runs/prompt12_n0_memory_sanity_r2`

It used the untouched Prompt10 native authority, eee_01, camera/visual OFF,
1x playback, estimator-PID-only `/proc` monitoring, 2 s sampling, 200 samples,
and 402.37 s total monitoring duration.

| Metric | Prompt10 historical N0 median (MiB) | Prompt12 peak (MiB) | Prompt12 final (MiB) | Difference |
|---|---:|---:|---:|---:|
| RSS | 2675.70 | 2674.70 | 2674.70 | <0.1% |
| PSS | 2656.50 | 2655.94 | 2655.94 | <0.1% |
| USS | 2655.22 | 2654.51 | 2654.51 | <0.1% |

The early 20–40 s window is warm-up and the late curve reaches the historical
plateau; the fresh run is not `MEMORY_ENVIRONMENT_NOISY`. Prompt11's roughly
89.46% N0 memory saving is accepted. The N0 trajectory had 3985 rows, SHA
`8906ba652f3862f9f6d11d7d9bdb785bdfd91557cd9ba010e8aee336b470033a`, and ATE
`0.030768325230497 m`.

## G-P12.2/G-P12.3 — runner integrity and 139 diagnosis

The old runner behavior was reproduced by a red test: a 139 process with no
completion sentinel was not classified, and the old scripts mapped a completed
139 to `run_rc: 0`. The corrective runner now preserves the true process RC in
`node_rc`, writes `run_status`, and returns zero only for `CLEAN_SUCCESS`.

The supported statuses are:

`CLEAN_SUCCESS`, `PROCESSING_COMPLETE_WITH_SHUTDOWN_FAULT`,
`INCOMPLETE_CRASH`, `TIMEOUT`, `CANCELLED`, and `CONTAMINATED`.

The status test covers RC0, completed RC139, incomplete RC139, timeout,
cancellation, contaminated source, and verifies that shutdown fault keeps the
runner's own return code non-zero. Both reader adversarial tests and runner
status tests pass in CTest.

The completion sentinel records `processing_complete`, flush status, expected
final epoch/row status, and offline EOF drain. A 139 is classified as shutdown
fault only after all required reports exist. Current full-bag corrected runs
recorded:

- LIO offline: `node_rc: 139`, `run_status: PROCESSING_COMPLETE_WITH_SHUTDOWN_FAULT`, `run_rc: 1`.
- LIO online: `node_rc: 139`, `run_status: PROCESSING_COMPLETE_WITH_SHUTDOWN_FAULT`, `run_rc: 1`.
- LIVO offline: `node_rc: 139`, `run_status: PROCESSING_COMPLETE_WITH_SHUTDOWN_FAULT`, `run_rc: 1`.
- LIVO online: `node_rc: 139`, `run_status: PROCESSING_COMPLETE_WITH_SHUTDOWN_FAULT`, `run_rc: 1`.

The gdb run was `/home/lc/super_livo/src/FAST-LIVO2/results/prob_livo/runs/prompt12_rc139_gdb_lio/gdb.log`.
The inferior reached SIGSEGV in teardown, with the relevant stack:

`__GI___libc_free` → `boost::detail::sp_counted_impl_p<pcl::PointCloud<pcl::PointXYZINormal>>::dispose()` → `boost::detail::sp_counted_base::release()` → `LIVMapper::~LIVMapper()` → `main`.

This is a destructor/lifetime teardown failure in PCL shared-pointer release,
after estimator output is complete. It is not an in-flight estimator crash.
Fixing it would require ownership redesign, which is outside Prompt12; the
runner therefore retains the explicit shutdown-fault classification.

## G-P12.4 — reference counter semantics

The former `reference_patch_updates_accepted` increment was a best-so-far
candidate replacement inside the scoring loop. It was renamed to
`reference_patch_candidate_replacements`. A separate
`reference_patch_commits` counter increments once after selection only when the
final selected reference pointer truly changes. Reference selection itself was
not changed.

The corrected LIVO offline report demonstrates the distinction:

- candidate replacements: `546229`
- final reference commits: `61299`

The counter instrumentation is trajectory-byte-inactive: corrected LIO offline
and online both produced the Prompt11 LIO trajectory SHA
`8906ba652f3862f9f6d11d7d9bdb785bdfd91557cd9ba010e8aee336b470033a`.

## G-P12.5 — compile-time native LIVO single-thread matrix

The diagnostic build was compiled with `FAST_LIVO_MP_PROC_NUM=1`, producing
`-DMP_EN -DMP_PROC_NUM=1` in the estimator targets. This was not an
`OMP_NUM_THREADS`-only test.

| Run | Source | Rows | SHA | ATE (m) | VisualPoints | Candidate replacements | Plane queries | Final visual map |
|---|---|---:|---|---:|---:|---:|---:|---:|
| O1 | normal online, t=1 | 3983 | `a5f6aece74037a7e1e9c4b9c1db543673bf26ef16221dc821e6a2523d92ef95a` | 0.032091832 | 358583 | 528290 | 110123 | 118 |
| O2 | normal online, t=1 | 3983 | `f09cddff74d8cac04aef08c95e3f485bd31d8c05daf104cc8a99aa80e848389b` | 0.032116920 | 358507 | 526444 | 110166 | 117 |
| F1 | offline, t=1 | 3983 | `b0e5c57b7f89fd59f9e35717a6dbeded83b435cd23546ec6405cf8ad02b1ddfa` | 0.032121625 | 358198 | 536075 | 111032 | 118 |
| F2 | offline, t=1 | 3983 | `80d60c3704dba6642a66a1489f74a7d07bc22a774092b4257e2713148a73ea1d` | 0.032081909 | 359039 | 529573 | 110193 | 117 |

The matrix was captured immediately before the counter-schema correction, so
the old visual sidecars did not contain true commit counts; corrected final
online/offline runs below provide those counts. All four matrix runs had the
same LIO/input/epoch counts: IMU callbacks 153347, LiDAR callbacks 3987,
image callbacks 3986, scheduler steps 7967, camera epochs 3983, visual calls
3983, visual state commits 3982, and trajectory rows 3983. Timing counts were
also identical across O1/O2/F1/F2: input preprocess 7972, IMU 7967, LiDAR
association 3984, map query 3983, map update 3983, visual processing 3982,
estimator 7967.

Strict repeated-run deltas were:

- O1 vs O2: timestamps exact; translation RMSE/max
  `0.0018789293798895492 / 0.012275900985263774 m`; rotation RMSE/max
  `8.407747920499716e-05 / 0.0006877763232471106 rad`.
- F1 vs F2: timestamps exact; translation RMSE/max
  `0.0021044046201175197 / 0.011956776990476993 m`; rotation RMSE/max
  `9.372384484130298e-05 / 0.0006315407027215525 rad`.

## G-P12.6 — classification and reduction gate

Because `O1 != O2` at compile-time `MP_PROC_NUM=1`, the result is:

`NON_OPENMP_NATIVE_NONDETERMINISM`

F1 also differs from F2, so the offline source is not deterministic either, but
the primary STOP condition is already Case C. The evidence is consistent with
an unordered traversal, Eigen/TBB behavior, shared mutable visual state, or
allocation/order dependency; it does not justify attributing the result to an
OpenMP reduction.

No reduction adaptation, reduction oracle, O3/O4 run, or second numerical patch
was performed. This is required by the Case C gate.

## G-P12.10 — final corrected native LIVO online/offline

Runs:

- `results/prob_livo/runs/prompt12_final_livo_online`
- `results/prob_livo/runs/prompt12_final_livo_offline`

Both runs processed 3983 rows with exact timestamp pairing, but were not byte
identical:

| Source | SHA | ATE (m) | VisualPoints | Candidate replacements | Commits | Plane queries | Final map |
|---|---|---:|---:|---:|---:|---:|---:|
| Online | `aa9568f6a14d5b251e9c613796c232e286ec25c19e8ecf8940aa16974296d990` | 0.031160949 | 363856 | 527752 | 59395 | 109332 | 137 |
| Offline | `520ef609b3e92c6108560e97ccd2a762bb833f907c7c77d2d86a15bc65c96e05` | 0.029787499 | 356641 | 546229 | 61299 | 112041 | 128 |

Raw online-vs-offline deltas were translation RMSE/max
`0.051814674711423835 / 0.09422328564638366 m` and rotation RMSE/max
`0.0026685319829017613 / 0.017130740851515728 rad`. LIO counters and timing
counts were equal (scheduler step 7967, camera epochs 3983, visual calls 3983,
visual commits 3982, trajectory rows 3983; timing counts 7972/7967/3984/
3983/3983/3982/7967). The source-specific scheduler poll count differed, as
expected: online 1537719 versus offline 161336.

Therefore final native LIVO online/offline parity is not PASS. This is the
Prompt12 owner-decision STOP, not a reason to modify scheduler cadence or add a
second numerical patch.

## G-P12.11 — scheduler cadence audit

Online executes `spinOnce()` and then the shared
`ProcessAvailableNativeEpochs()` scheduler in its normal loop; empty polls are
possible and are counted separately. Offline dispatches each bag record to the
original callback and invokes the same scheduler seam after the record, then
performs bounded EOF draining. The full LIVO offline run recorded
`eof_drain_steps: 0`, with all 3986 image callbacks and 3987 LiDAR callbacks
accounted for.

The large online/offline poll-count difference is source cadence, while the
single-thread repeated-run failure proves that changing cadence to force parity
would violate the Prompt12 gate. The scheduler was not changed.

## Native LIO regression

Corrected current-patch runs:

- offline: 3985 rows, SHA
  `8906ba652f3862f9f6d11d7d9bdb785bdfd91557cd9ba010e8aee336b470033a`;
- online: 3985 rows, the same SHA.

All input, estimator, epoch, map, and trajectory counters matched. Only the
source-specific `scheduler_poll_calls` differed (offline 157350 versus online
1689612 in these final runs). This preserves the Prompt11 native LIO result.

## Tests and build

- full production build: passed, default host policy `MP_PROC_NUM=4`;
- compile-time single-thread build: passed, `MP_PROC_NUM=1`;
- `native_offline_reader_tests`: passed, including record-order, ignored-topic,
  image-disabled, near-equal timestamp, and EOF-adjacent cases;
- `native_runner_status_tests`: passed;
- `git diff --check`: passed;
- gdb teardown diagnostic: captured the SIGSEGV stack above.

## Commits and handoff

- `f4c74fb` — expose compile-time diagnostic OpenMP worker override;
- `3ebcc4c` — classify shutdown faults and count reference commits;
- `624653e` — assert shutdown-fault exit remains non-zero.

The final native branch is `prompt11-native-offline`; the primary
`prob-livo` worktree remains unchanged. The branch is pushed to
`origin/prompt11-native-offline` after this report commit.

STOP. Do not start multi-dataset benchmark.
