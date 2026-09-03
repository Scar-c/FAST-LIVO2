## Prompt 9 Final Report

- Initial HEAD: `af3c82f6dd5e429536707aa952605ab75f2130fd` (Prompt8 close); the required pre-edit normal-online H0 was run at this HEAD.
- Final HEAD: implementation frontier `1cec675ac25c95a458bb56d6c4e63eef0082f756`; the final evidence/docs commit is the handoff HEAD recorded after this report.
- origin HEAD: fast-forwarded to the final handoff HEAD after verification.
- worktree clean: yes at every canonical run; final status is recorded at handoff.
- Prompt registered: `prompts/prob_livo/prompt9_i6_corrective_closure.md`, SHA256 `841faf3240c8e6ac43d38acfac60b1091520100864647bc51b88fc346c159a48`.
- Architecture deviations: none. The permitted seams only add the exact `point_W` gate input, split provider validity, remove repeated query eigensolvers, and make the online callback-step runner parameter explicit. No I8, P5, downsample, FEJ, scheduler, map-ownership, or visual-core redesign was introduced.
- Numerical/determinism adaptations: no parity shim was used for the Prompt9 normal-online check. Offline remains the in-process FAST-LIVO2 callback/scheduler path with TBB maximum parallelism 32; normal online uses the real ROS subscriber queue and default `ros::spinOnce()` with `/common/prob_livo_one_callback_step=false`.

### G-P9.1 Correct QR gate

- production formula: `J_nd=[point_W^T,1]`; `sigma_plane^2=J_nd Sigma_nd J_nd^T`; `sigma_point^2=n^T Sigma_point n`; H1 accepts only `abs(residual) < 3*sqrt(sigma_plane^2+sigma_point^2)`.
- point_W source: the exact `p_w` used by `VIOManager::updateReferencePatch()` is copied into `VisualPlaneGateInput::point_W`; it is not reconstructed from the normal or center.
- independent oracle: non-unit/nonparallel `point_W=(2.0,1.5,3.0)`, non-cI 4x4 QR covariance with cross terms, and non-cI 3x3 point covariance with cross terms; inside, exact-boundary, and outside thresholds are checked independently.
- wrong-J mutation: replacing `[point_W^T,1]` with `[normal^T,1]` changes the fixture variance by more than `1e-3` and fails the red mutation test on the pre-fix implementation.
- result: PASS. The old implementation first failed with `A must use the independent QR-native [point_W^T,1] variance`; the corrected implementation passes G-I6 with 19 checks.

### G-P9.2 Geometry / uncertainty split

- provider behavior: QR geometry is produced from valid HKNN support even when `ComputeProbQrPlane()` cannot produce finite covariance; `geometry_valid` and `uncertainty_valid` are independent, while legacy `valid` remains a geometry alias.
- H1 behavior: consumes geometry but fails closed unless uncertainty and all required finite variance inputs are valid.
- H2 behavior: consumes geometry and the common radius gate without requiring covariance validity; it preserves `length > 81*residual^2`.
- adversarial invalid-cov case: valid tilted support with all-NaN support covariances yields `query_ok=true`, `geometry_valid=true`, `uncertainty_valid=false`; H1 rejects and H2 accepts.
- result: PASS. G-I5 is green with 115 checks and the production callback no longer collapses geometry validity into covariance validity.

### G-P9.3 Visual hot-path eigensolver

- before: `EvaluateVisualPlaneGate()` called `IsFinitePositiveSemidefinite()` on both plane and point covariance matrices, causing repeated `SelfAdjointEigenSolver` work per visual gate.
- after: the gate uses upstream validity plus `allFinite()` and scalar variance/non-negativity checks. The covariance producer's existing PSD validation remains outside the gate hot path.
- evidence: the gate decision region contains no `SelfAdjointEigenSolver` or `IsFinitePositiveSemidefinite` call; `src/prob_livo/prob_plane_provider.cpp` contains no `SelfAdjointEigenSolver`.
- result: PASS. The upstream VisualPoint covariance producer retains its existing validity test; no repeated validation remains in the visual gate decision path.

### G-P9.4 I5 radius cleanup

- original radius source: one `SelfAdjointEigenSolver<Eigen::Matrix3d>` over the support covariance per provider query, followed by `sqrt(lambda_max)`.
- new source: an exact closed-form largest-eigenvalue calculation for the symmetric 3x3 support covariance, followed by the same `sqrt(max(0, lambda_max))`; it has no persistent query state.
- semantic parity: the independent Eigen oracle reports maximum fixture difference `5.551115123125783e-17`; the rerun H2 trajectory is byte-identical to Prompt8 H2.
- cache added: NO
- 1000-query before/after: Prompt7 before `2.760623 ms`; Prompt9 corrected implementation measured `2.133600`, `2.519477`, and `2.429241 ms` across three runs (same bounded fixture; timing variation is host-load dependent).
- result: PASS. Exact radius semantics and downstream behavior are preserved, with the redundant provider eigensolver removed.

### G-P9.5 Corrected H1

- rows: 3979; matched GT: 3326.
- visual calls: 3979; camera epochs/images received: 3985/3985.
- photometric commits: 3977; visual state commits: 3977; rollbacks: 5196.
- plane queries: 57485; geometry-valid/uncertainty-valid: 49909/49909.
- reference updates: 28090 accepted out of 57485 attempts.
- ATE: `0.08795092773331592 m`.
- result: PASS. The corrected H1 offline run completed with all node/counter/GT/evaluator return codes zero; trajectory SHA256 is `9939bb6bc4688d9685cc156e6a473e2f7413e79a15585b8f72aba7170184f53a`.

### G-P9.6 H0/H2 preservation

- H0 reused/rerun: reused Prompt8 H0 as the visual-inactive control and confirmed it with the required pre-edit normal-online H0. Both have 3979 rows, ATE `0.09138258970792523 m`, SHA256 `9f9eae6fe119d23260d23c4c10a0fba900ba32798d0a58239861432a11d3c53f`, and zero visual process/plane/commit counters. The corrective seams are inactive in this control.
- proof/result: the pre-edit normal-online H0 at `af3c82f6` matched Prompt8 H0 offline byte-for-byte; visual callbacks were inactive, so no post-corrective algorithm path was exposed.
- H2 reused/rerun: rerun offline after the provider radius change as `results/prob_livo/runs/prompt9_h2_superlegacy_radius_cleanup_offline`.
- proof/result: 3979 rows, ATE `0.08793104514326024 m`, 44856 plane queries, 36820 geometry/uncertainty-valid queries, radius pass/reject `36817/3`, second-gate pass/reject `36817/3`, SHA256 `812d1bb9de1abeba0471632fa7515ee7afff682cbcad87bbab2786b109f5f6ca`; it is byte-identical to Prompt8 H2.

### G-P9.7 Normal online vs offline

- normal-online callback mode: real ROS subscribers/callback queue, `rosbag play`, and the normal `ros::spinOnce()` branch; the runner default was changed from a hardcoded shim to an explicit parameter defaulting false.
- one_callback_step: `false` in `meta.txt` and `/common/prob_livo_one_callback_step: false` in `effective_rosparams.yaml`; no `PROB_LIVO_ONE_CALLBACK_STEP` override was set.
- online rows/counters/ATE: 3979 rows; authority counters and all visual counters exactly match offline; ATE `0.08795092773331592 m`.
- offline rows/counters/ATE: 3979 rows; same counters; ATE `0.08795092773331592 m`.
- timestamp equality: 3979/3979 paired, timestamp delta RMSE/max `0/0 s`, no unmatched rows.
- counter equality: authority sidecars byte-identical; visual sidecars byte-identical.
- field diff: strict comparator translation RMSE/median/max `0/0/0 m`; quaternion diagnostic max `4.2146848510894035e-08 rad`, attributable to `acos` roundoff; trajectory files themselves are byte-identical.
- SHA: online and offline both `9939bb6bc4688d9685cc156e6a473e2f7413e79a15585b8f72aba7170184f53a`.
- first divergence if any: none.
- result: PASS. The corrected normal-online/offline hard gate passes with exact event counts, timestamps, fields, and bytes.

### I1–I6 regression

- build: `cmake --build /home/lc/super_livo/build --target all -- -j4`, return code 0.
- tests: `prob_livo_i1_tests`, `prob_livo_i2_tests`, `prob_livo_i3_tests`, `prob_livo_p4_tests`, `prob_livo_i4_tests`, `prob_livo_i5_tests`, and `prob_livo_i6_visual_gate_tests`; all returned 0. Prompt9 focused results include G-I5 `checks=115` and G-I6 `checks=19`.
- result: PASS.

### Canonical ablation

| Variant | Meaning | ATE | Validity |
|---|---|---:|---|
| H0 | scheduler ON / visual OFF | 0.091382590 | valid |
| H1-old | wrong QR Jacobian | 0.088624546 | **INVALID FOR ABLATION** |
| H1-corrected | correct probabilistic 3σ | 0.087950928 | valid |
| H2 | Super legacy visual gate | 0.087931045 | valid; radius cleanup rerun |

The Prompt8 H1 value `0.08862454637627792 m` and SHA
`315c8b0ec74e409bb1a22ac08de6725d1ee042c563e546ec1591b52ef80825aa` are
preserved as historical evidence but are explicitly invalid for the
probabilistic ablation because the old implementation used the wrong plane
Jacobian.

### Final I6 decision

`I6 CLOSED`

All G-P9.1–G-P9.7 hard gates pass. Corrected H1 is valid, H0 is preserved,
H2 is preserved after radius cleanup, and the normal-online/offline exact
parity gate passes. I7 downsample ablation remains cancelled; the visual gate
ablation is complete inside I6. **禁止进入 I8。**

### Final hygiene

- Bag SHA256: `7ea43946cffdd49c88d993ad3f192a4e90a8f6826eddc2ef1a9d4f5343ca6c17`.
- Config overlay SHA256: `a1d775f552a3d13dd6750a6099a8ea80503e2510d08052d38c2e92c0218e2587`.
- Tracked large-file audit: only the pre-existing `Supplementary/LIVO2_supplementary.pdf` exceeds 10 MB; no bag, trajectory, node log, build, devel, or temporary run artifact is tracked.
- Final git status: clean after the evidence commit; local and `origin/prob-livo` point to the same final handoff HEAD.
