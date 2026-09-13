# Canonical Prob-LIVO repository cleanup report

## Decision

`CANONICAL CLEANUP CLOSED` is the intended final state of this report.
Canonical semantic: Prompt22 FEJ-OFF, anchored at
`94fe832d826e464417ff90693d1f2aafae5b255a` with tree
`2ef27db5a0a9ec63136d011992926048bd78af4c`.

The reconstructed production commit is `e803df2` (the final audit commit is
the commit containing this report). The old remote `prob-livo` value recorded
before publication was:

```text
8743e83c6691e25213e5a1674f88a3adb9c632d7
tree 5bfdf256b23853a3210c621bdee7d7d730690ae5
bench(prob-livo): record Prompt28 BIEVR eee01 canary
```

Publication must use the recorded old value as the force-with-lease expected
value. No amend, rebase, squash, cherry-pick, or new prompt cleanup branch is
used.

## Production contents retained

- Prompt22's canonical P0–P4 backend, configs, offline runner, matrix, and
  shared-config manifest remain the production base.
- Prompt20/21 visual ownership, lifetime, and memory tests remain intact.
- `src/LIVMapper.cpp` retains only the Prompt27 publication-only fix: the
  camera epoch publishes the current world scan through the RGB projection and
  accumulates it on `/cloud_registered`; the LIO callback avoids the duplicate
  empty LIVO publication. Estimator state, scheduling, association,
  covariance, and trajectory semantics are unchanged.
- `rviz_cfg/prompt27_fast_livo2.rviz` retains the verified `/cloud_registered`
  RGB8 and accumulated colored-map layout.
- `launch/prob_livo_online_avia.launch` is a sanitized canonical online launch
  with `fast_native`, native blind carry, the LIVO2 probability gate, and no
  rejected experimental semantic parameter.

## Archive and deletion policy

Prompt23 is preserved under `archive/ablation/prompt23/` as
`ABLATION_ONLY/NOT_CANONICAL`. Prompt24/25 FEJ-OFF rows and Prompt27/28 Bright
visual controls are extracted unchanged under
`archive/canonical_controls/`, with provenance. Prompt11 Native offline
reader material is historical archive only. All post-Prompt22 FEJ-on
implementation, common-linearization/CN implementation, SA-CN/SA-LIVO
implementation, associated tests/runners/configuration, and external
BIEVR/COIN material are excluded from production.

The complete pre-cleanup branch graph and dirty-state capture is preserved in
`CANONICAL_CLEANUP_BEFORE.txt`; the artifact classification is in
`CANONICAL_CLEANUP_CLASSIFICATION.csv`.

## Validation

Build command, executed from `/tmp/canonical_cleanup_ws` against the clean
worktree:

```text
source /home/lc/design_ws/devel/setup.bash
catkin_make -j4
```

Result: PASS. The first unsourced attempt stopped at the external catkin
dependency `vikit_common` before compilation; sourcing the existing local
official `rpg_vikit` build at `/home/lc/design_ws/devel` resolved it. No source
or estimator code was changed for this environment correction.

Unit/invariant executables: PASS — `prob_livo_i1_tests`,
`prob_livo_i2_tests`, `prob_livo_i3_tests`, `prob_livo_p4_tests`,
`prob_livo_i4_tests`, `prob_livo_i5_tests`, `prob_livo_i6_visual_gate_tests`,
`prob_livo_i7_visual_memory_tests`, and `prob_livo_i8_visual_lifetime_tests`.
Reported check totals are 28+15+41+45+16, 12+75+67+27+35, 79, 203, 57,
115, 19, and the Prompt21 I7/I8 ownership/lifetime suites respectively.

Configuration and pair gates: PASS for the canonical LIO identity and
negative mutation test; PASS for the Prompt22 Oxford College_03 LIVO pair,
including 2841 identical selected camera timestamps. An older Prompt15 NTU
LIVO pair was not relabeled: its historical timestamp files were absent and
therefore it fails the current pair gate as historical data, not as a clean
tree failure.

Canonical NTU P-LIO offline smoke/regression:

| Item | Result |
|---|---:|
| Run | `canonical_cleanup_ntu_p_lio_escalated` |
| Input | eee_01, bag SHA256 `7ea43946cffdd49c88d993ad3f192a4e90a8f6826eddc2ef1a9d4f5343ca6c17` |
| Backend | Prob-LIO P0–P4, camera OFF, `fast_native` |
| Successful epochs | 3987 / 3987 |
| Rejected/discarded | 0 / 0 |
| Trajectory rows | 3981 |
| ATE RMSE | `0.04951128171290361 m` |
| Offline wall time | 43 s; process report 42.002 s |
| Trajectory SHA256 | `adc63ad59ee3636b7fac9975800d3ac3ccb09433133f35fe77c97122e2b5f802` |

The ATE matches the Prompt22 P-LIO eee_01 reference
`0.049511281713 m` within recorded precision. The offline runner reported
`tbb_max_parallelism=4`, `omp_max_threads=4` for this validation run because
the canonical runner's worker setting is 4; no concurrent experiments were
running.

Production semantic search over `include src config launch CMakeLists.txt`:

```text
cross_modal_fej                 absent
legacy_lidar_anchor_fej        absent
common_linearization            absent
sa_common                       absent
sa_livo                         absent
SAIF                            absent
```

## Executable fingerprints

The following hashes are from the clean `catkin_make -j4` devel space:

```text
fastlivo_mapping                 2c4672e245bdc7b4baa8a561e0e3ab3ab62780bd161031b730ad1c8cd2b4c3f5
prob_livo_offline                5b0941e9684da1d8e78cc81267534ed07a52f3253f47bb05259b037a4cfe6f07
prob_livo_i1_tests               39fefdc23db3b298281679e5246deab2bfdf3febeffd4e9e2625543a4b385ebc
prob_livo_i2_tests               047f472e4418491891390d4d4c90f1d2d12e6afbfb437c033f246639016feac9
prob_livo_i3_tests               cc9aa070f208fe74f8dfb38443f9a27bf6bb2b1c5c841944a4e7316f4b789450
prob_livo_p4_tests               0d1274cbff948bd2ed373a29b20ff317f162cbf1a1464c3b3dc13cc8b5fd2beb
prob_livo_i4_tests               473ce794dee1ddd66911cc0429573692e0a6d788d2e6d9b6355b41c3a84da1e4
prob_livo_i5_tests               135138ae25ddbb28fd352bb5e080453e8f46c026d4ed762816ee1c9ab50a88ba
prob_livo_i6_visual_gate_tests   a964e5418c72ee508eee423387188688e2e347267e3b239952386780ddfd3111
prob_livo_i7_visual_memory_tests 7cb39d8284b5e24d5b1087981dbd048acb0d909d5062e809a77b655beb3636d4
prob_livo_i8_visual_lifetime_tests 0220072cc48df10437701e58f77e528b15e804f2391bc5254791e7936e276ee2
```

## Final closure checks

The final values below are filled after the authorized remote update and exact
branch deletion checks:

```text
FEJ production: absent
CN production: absent
SA production: absent
correct post-Prompt22 controls preserved: PASS
canonical tests: PASS
prob-livo local == remote: PASS (both 4343d4a before this final report commit)
worktree clean: PASS after this report commit
rejected experimental branches deleted: PASS (Prompt11 and Prompt29–38)
```

The final report-only commit is pushed with a force-with-lease expectation of
`4343d4aeea51e15551f9d13fba611f718e6728f5`; it changes no production source,
configuration, test, or evidence data.
