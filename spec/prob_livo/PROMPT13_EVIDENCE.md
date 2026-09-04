# Prompt13 Evidence

Status: `PROMPT13 DIAGNOSIS CLOSED / OWNER DECISION REQUIRED`

## Scope and gate result

This run followed Prompt13's order: reproducibility/UB checks first, then the
single-thread hard gate. The gate failed because repeated native LIVO runs were
not byte-identical even with compile-time `MP_PROC_NUM=1`. Prompt13 therefore
requires STOP. No reduction adaptation, normal-thread O3/O4/O5/F3 runs, or
Phase-B teardown change was started.

The native branch is `prompt11-native-offline`; the primary `prob-livo`
worktree remained clean. Prompt13 source SHA256:

`325cdef22dd92353db1714596bb439e7fefabfa9d69ec3d2ce7aea7d698e181b`

## Phase A — cached visual state and reference-patch safety

The visual NCC path had two unsafe cached-mean declarations: `ref_mean` and
`other_mean` could be read before initialization when the cache was empty. They
now read the stored cache value first and compute/store the mean only when the
cache is empty. The NCC formula, threshold, and reference-selection policy
were not changed.

`VisualPoint::ref_patch` is now explicitly initialized to `nullptr`. Reads and
destruction comparisons are guarded by `has_ref_patch_` and a non-null pointer.
The existing candidate counter remains the in-loop
`reference_patch_candidate_replacements`; `reference_patch_commits` is a
separate post-selection counter that increments only when the selected pointer
actually changes. No selection behavior was changed.

The independent cached-path oracle test is red before the production fix and
green after it. The corrected test suite output was:

`cached-mean legacy red proof and cached-path oracle passed`

Production build and CTest also passed:

- `native_offline_reader_tests`
- `native_runner_status_tests`
- `cached_mean_oracle_tests`

The diagnostic build used the compile-time override
`FAST_LIVO_MP_PROC_NUM=1`; CMake confirmed `MP_PROC_NUM=1`.

## Phase-A single-thread LIVO matrix

All four runs used the full `eee_01` bag, camera ON, and playback factor 1.
Each processed 3983 trajectory rows and reached the completion sentinel, but
the runner correctly returned 1 because the process exited with the known
teardown signal 139.

| Run | Mode | SHA | ATE (m) | VisualPoints | Candidate replacements | Commits | Plane queries | Final map |
|---|---|---|---:|---:|---:|---:|---:|---:|
| O1 | normal online | `3cd7fb71ce2d693d3b9861131555edd1248367cc76939501529371f348c9c692` | 0.032080266 | 360407 | 394191 | 50195 | 110256 | 126 |
| O2 | normal online | `06dc26568e4e9ad12b40808d27ea34f2e1017400ee3bc920b2c44c7e4abb858c` | 0.032105743 | 360083 | 389817 | 50105 | 109791 | 128 |
| F1 | offline | `d219499426fafe7e0b36858141120ea0d87985c762aca331aab654ca63b5c889` | 0.032069614 | 360607 | 391161 | 50064 | 109809 | 126 |
| F2 | offline | `0ce45c4e35498a7dab5d90919c7c86f9afc8d882b1de4c675ff34a175f6becb0` | 0.032078435 | 360636 | 392845 | 49978 | 110040 | 127 |

All four runs had identical input/LIO/epoch totals: IMU callbacks 153347,
LiDAR callbacks 3987, image callbacks 3986, IMU enqueued 153323, LiDAR
enqueued 3987, image enqueued 3985, ignored 25, scheduler steps 7967,
scheduler sync packages 7967, LiDAR epochs 3984, camera epochs 3983, IMU
processing 7967, LiDAR updates 3983, map queries 3983, map updates 3983,
visual calls 3983, visual state commits 3982, and trajectory rows 3983.
Timing counts were also identical: input preprocess 7972, IMU 7967, LiDAR
association 3984, map query 3983, map update 3983, visual processing 3982,
and estimator 7967.

The repeated-run comparisons were:

- O1 vs O2: exact timestamps; translation RMSE/max
  `0.001226685531082315 / 0.00938597379071562 m`; rotation RMSE/max
  `6.904191894321272e-05 / 0.0006835377698424153 rad`.
- F1 vs F2: exact timestamps; translation RMSE/max
  `0.0013930322677781198 / 0.01203093038796252 m`; rotation RMSE/max
  `7.048043141398272e-05 / 0.00047126220734892976 rad`.

Therefore the result is `NON_OPENMP_NATIVE_NONDETERMINISM`, not a justified
OpenMP-reduction diagnosis. The cached-mean UB fix is covered by the red/green
oracle, but it did not close the reproducibility gate.

## Counter instrumentation A/B

To test the camera-ON requirement, an isolated diagnostic worktree disabled
only visual counter writes at compile time. It was not added to production.
The counter-off offline run still processed 3983 rows and produced:

- trajectory SHA `27776fec9be3e55ab05d1b7972d183bd6fe462b3329d365ab7e30246b3f7c4e3`;
- ATE `0.032081541 m`;
- the same input/LIO/epoch totals as the matrix;
- zeroed visual counters, as intended.

The counter-on comparison was F1. Because the counter-on baseline itself is
non-deterministic, this one-run A/B does not prove byte identity. Source and
dataflow inspection shows these writes are observational, but Prompt13's
stronger trajectory-byte-inactive criterion is not closed.

## Phase B — teardown

Phase B was not entered because the Phase-A single-thread gate failed. No
ownership table or teardown patch was started.

The existing Prompt12 gdb evidence remains the relevant diagnosis:

`__GI___libc_free` →
`boost::detail::sp_counted_impl_p<pcl::PointCloud<pcl::PointXYZINormal>>::dispose()`
→ `boost::detail::sp_counted_base::release()` → `LIVMapper::~LIVMapper()` →
`main`.

The runner continues to preserve `node_rc: 139`, classify a completed run as
`PROCESSING_COMPLETE_WITH_SHUTDOWN_FAULT`, and return nonzero. No `139` to
`RC0` conversion and no claim of clean teardown was made.

## Commit and handoff

- `9496141` — initialize cached visual references safely and add the
  cached-mean oracle test;
- the present evidence report is committed immediately after that fix and
  pushed on `origin/prompt11-native-offline`.

`PROMPT13 DIAGNOSIS CLOSED / OWNER DECISION REQUIRED`

STOP. Do not start reduction tuning, multi-dataset benchmarking, or Phase-B
teardown redesign until an owner decides how to address the non-OpenMP native
nondeterminism and the separate counter-inactivity proof requirement.
