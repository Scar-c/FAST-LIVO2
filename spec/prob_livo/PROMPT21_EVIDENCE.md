# Prompt21 — Visual Lifetime Authority + Matrix Closure Evidence

## Final status

`PROMPT21 CLOSED — VISUAL LIFETIME AUTHORITY + MATRIX CLOSURE COMPLETE`

Prompt source:
`/home/lc/super_livo/prompts/PROMPT21_VISUAL_LIFETIME_MATRIX_CLOSURE.md`

Prompt source SHA256:
`b661cab07bef548258c980fbf628de630d0757d69beb5cfde167510cc31524c0`

Primary repository:
`/home/lc/super_livo/src/FAST-LIVO2`

Native repository was inspected read-only and was not modified in Prompt21.

## 1. Startup and change boundary

At Prompt21 startup:

| Item | Value |
|---|---|
| Primary branch | `prob-livo` |
| Primary HEAD | `015c5a7` (`docs(prob-livo): close prompt20 visual memory evidence`) |
| Primary `origin/prob-livo` | `015c5a7` |
| Primary worktree | clean |
| Native branch | `prompt11-native-offline` |
| Native HEAD and `origin` | `ea7702b` (`fix(native): close visual patch temporary ownership`) |
| Native worktree | clean |
| leftover runner/rosmaster/estimator processes | none |

Prompt21 production code and seam test are committed locally as:

`073b759 fix(prob-livo): drive visual lifetime from Super parent eviction`

The final documentation/matrix commit is recorded below after the artifacts
were validated. No Native estimator source, Native build, Native benchmark,
or unrelated estimator path was changed.

The repository contains `prompts/prob_livo/README.md` but no top-level
`prompts/README.md`; the maintained prompt index was therefore updated at
`prompts/prob_livo/README.md`.

## 2. Actual geometry lifetime authority

The production geometry producer is:

`include/prob_livo/super_native/OctVoxMap/OctVoxMap.hpp`

Its authoritative container is `DATA_LIST data_`, a `std::list` whose front
is most-recent and whose back is the eviction victim. The map also owns the
`grids_` lookup and checks `data_.size() >= capacity_` in both production
`insert` overloads. The actual removal path is:

1. capture `data_.back().first` as the exact parent key;
2. erase that key from `grids_`;
3. pop the back list node;
4. invoke the installed parent-eviction callback with that same key.

The Prob backend owns this map through `ProbLioBackend::map_`. `LIVMapper`
installs the callback after backend construction and forwards the exact
`Eigen::Vector3i` key to
`VIOManager::eraseVisualParentBySuperEviction`.

The frozen geometry identity remains the 0.5 m parent key with eight local
0.25 m subvoxels. No subvoxel visual LRU was introduced.

## 3. Final visual lifetime architecture

`VisualParentRegistry` is now an unordered map of owning
`unique_ptr<VisualParentHost>` objects. Production no longer contains or
uses a visual `capacity_`, `lru_`, `lru_positions_`, `touch()`,
`evictToCapacity()`, or `prob_livo_visual_parent_lru_capacity`.

The production API is the owner-only contract:

- `findNoTouch(key)`;
- `getOrCreate(key)`;
- `eraseBySuperEviction(key)`;
- `clear(callback)`;
- `snapshot()`.

The visual stage name is `parent_owned`. Visual lookup and insertion do not
touch the geometry list. For an actual geometry eviction of `K`, the callback
increments the geometry-eviction counter, removes the borrowed `feat_map`
entries for `K`, erases exactly the owner host for `K`, and records either one
visual erase or a miss. The visual layer never selects a victim.

Ownership remains:

`VisualParentRegistry -> VisualParentHost -> VisualPoint -> Feature -> patch/image references`.

Spatial indexes and the current-frame view are non-owning. The borrowed view
is cleared before parent destruction; host destruction then releases child
points, features, observations, patches, and image references. This preserves
the Prompt20 rejected-candidate/accepted-transfer RAII fix.

## 4. Production-seam test and focused regressions

The mandatory integration test uses the real `LI2Sup::OctVoxMap` with a tiny
capacity of 3; it does not fake a VisualRegistry eviction. It inserts visual
parents A/B, re-accesses A through the real geometry map, verifies visual
lookup does not change the geometry order, inserts C and observes the exact
geometry victim B, then inserts D through the covariance overload and
observes the next exact victim A. The test checks non-victim preservation,
single callback behavior, exact-key erase, and final ownership release.

Build configuration and command:

```text
cmake -S /home/lc/super_livo/src -B /home/lc/super_livo/build -DFAST_LIVO_MP_PROC_NUM=4
cmake --build /home/lc/super_livo/build -j4 --target prob_livo_i7_visual_memory_tests prob_livo_i8_visual_lifetime_tests prob_livo_i6_visual_gate_tests prob_livo_offline
```

Results:

```text
PROMPT21 I7 visual ownership tests passed
PROMPT21 real geometry parent eviction tests passed
[PASS] G-I6 visual plane gate policy and sensor-range oracle checks=19
```

`git diff --check` passed. The build emitted only existing dependency/CMake
warnings and completed successfully.

## 5. Prompt21 runtime smokes

All runs were serial, Release, four workers, affinity `0,2,4,6`, native
blind-carry timing, native late-point deskew, leak-fix ownership, final
`parent_owned` lifecycle, and the corrected Oxford overlay where applicable.

| Run | wrapper/evaluator | trajectory | backend attempted/success/rejected | visual calls/commits | ATE RMSE | selected camera count/SHA | geometry evict / visual erase / miss |
|---|---|---:|---|---|---:|---|---|
| Oxford Palace_01 P-LIVO stride1 | RC0 / evaluator RC0 | 8031 | 8038/8038/0 | 8031/4053 | 0.2184 m | 8041 / `8db5c4...` | 0/0/0 |
| Oxford Palace_01 P-LIVO stride2 | RC0 / evaluator RC0 | 4014 | 4020/4020/0 | 4014/4012 | 0.1307 m | 4021 / `b313f8...` | 0/0/0 |
| NTU eee_01 P-LIVO canonical offline | RC0 / official evaluator RC0 | 3979 | 3984/3984/0 | 3979/3977 | 0.052246401963632165 m | 3986 / `215e5b...` | 0/0/0 |

The Palace values are inside the accepted Prompt20 run-to-run ATE envelope;
the eee_01 value is inside the established Prompt15 P-LIVO nondeterminism
acceptance. All three runs have complete trajectories, visual activity where
expected, and no backend rejection. The zero eviction counters are expected:
the canonical Palace/eee_01 runs do not reach the Super geometry capacity.
They validate that the new hook is inert when no geometry eviction occurs;
they do not claim a memory reduction.

Representative final lifecycle rows retain `parent_owned` and end with
geometry/visual-erase/miss counters `0,0,0`; the independent visual-capacity
column is gone.

## 6. NTU 9 × 4 matrix audit

The accepted formal artifact set is Prompt15's nine NTU sequences times four
arms times three repetitions. Prompt15 records 156/156 formal slots with
complete trajectory, evaluator, effective-rosparams, timing, memory, source,
and completion artifacts; the NTU subset is 36/36 valid cells. Prompt18's
rate audit established approximately matched NTU camera/LiDAR rates, and the
matrix therefore has no NTU stride axis.

| Coverage | N-LIO | P-LIO | N-LIVO | P-LIVO |
|---|---|---|---|---|
| 9 sequences | HISTORICAL_VALID (3 reps each) | HISTORICAL_VALID (3 reps each) | HISTORICAL_VALID (3 reps each) | HISTORICAL_VALID (3 reps each) |
| formal evidence | Prompt15 formal RC0/evaluator artifacts | Prompt15 formal RC0/evaluator artifacts | Prompt15 formal RC0/evaluator artifacts | Prompt15 formal RC0/evaluator artifacts; eee_01 final smoke added |
| stride needed | no | no | no | no |

Sequences are `eee_01/02/03`, `nya_01/02/03`, and `sbs_01/02/03`. The
cell-level records and representative run paths are in
`PROMPT21_FINAL_MATRIX.csv`. Final classification:

`NTU_ACCURACY_MATRIX_COMPLETE`

No NTU full-bag rerun was needed.

## 7. Oxford matrix audit

The canonical Oxford LIVO matrix reuses the complete Prompt19 primary
artifacts: P0 full rate, P0S frozen stride, N0 full rate, and N1 frozen
stride, each for all four sequences and three repetitions. Prompt18 P1/P2
strict reclassification remains ablation-only and is not substituted into the
canonical cells.

| Sequence | N-LIO | P-LIO corrected | N-LIVO full | P-LIVO full | N-LIVO stride | P-LIVO stride |
|---|---|---|---|---|---|---|
| Church_05 | HISTORICAL_VALID [Prompt15] | COMPLETE [Prompt17] | HISTORICAL_VALID [Prompt19 N0] | HISTORICAL_VALID [Prompt19 P0] | HISTORICAL_VALID [Prompt19 N1] | HISTORICAL_VALID [Prompt19 P0S] |
| College_03 | HISTORICAL_VALID [Prompt15] | COMPLETE [Prompt21 fill] | HISTORICAL_VALID [Prompt19 N0] | HISTORICAL_VALID [Prompt19 P0] | HISTORICAL_VALID [Prompt19 N1] | HISTORICAL_VALID [Prompt19 P0S] |
| Palace_01 | HISTORICAL_VALID [Prompt15] | COMPLETE [Prompt17] | HISTORICAL_VALID [Prompt19 N0] | HISTORICAL_VALID [Prompt19 P0] | HISTORICAL_VALID [Prompt19 N1] | HISTORICAL_VALID [Prompt19 P0S] |
| Quarter_01 | HISTORICAL_VALID [Prompt15] | COMPLETE [Prompt17] | HISTORICAL_VALID [Prompt19 N0] | HISTORICAL_VALID [Prompt19 P0] | HISTORICAL_VALID [Prompt19 N1] | HISTORICAL_VALID [Prompt19 P0S] |

Final classification:

`OXFORD_LIVO_MATRIX_COMPLETE`

The historical Oxford LIO gap was specifically corrected-config P-LIO for
`College_03`. No corrected College artifact existed in the prior search, so
only that one camera-off four-core run was newly filled:

`results/prob_livo/runs/prompt21_fill_p_lio_College_03_corrected`

It has RC0, evaluator RC0, 2861 trajectory rows, backend
2867/2867/0, ATE RMSE 0.0748 m, and effective
`filter_size_surf=0.5`, `min_eigen_value=0.01`. Prompt17 supplies the same
corrected-config authority for Church, Palace, and Quarter. The corrected
overlay SHA is
`b5917b705e856bdfa1f949201b57daada9be3645ceb677c0d03f40481405ce61`.

## 8. Native Oxford configuration authority

The checked-in Native and Prob base Oxford YAMLs have the same SHA
`87bd926073ed49c68ab330617e7ca52c0e0b0470ab2c63cdba89aac2312aca92`
only when read from the corresponding repository copy; the relevant verified
run metadata records that same base SHA. The actual Native effective artifact
was inspected directly:

`results/prob_livo/runs/prompt18_n1_native_Church_05_r1/effective_rosparams.yaml`

It contains:

```text
filter_size_surf: 0.1
min_eigen_value: 0.0025
```

The corrected Prob overlay artifact contains:

```text
filter_size_surf: 0.5
min_eigen_value: 0.01
```

Therefore the accepted Native Oxford cells are classified as:

`NATIVE_BASELINE_CONFIG_DISTINCT_BUT_FROZEN`

This is an effective-parameter distinction, not an evidence gap. Native N0/N1
were intentionally accepted under their own frozen Native baseline; they are
not represented as corrected-Prob-config runs. No Native rerun is scientifically
necessary for this Prompt21 closure.

## 9. Required answers

1. **Independent Visual LRU:** gone from production; the registry has no
   capacity, list, position map, touch policy, or victim selection.
2. **Same-key eviction:** yes. The real `OctVoxMap` captures its actual back
   key, erases geometry, and sends that exact key to the visual owner.
3. **Visual-only LRU mutation:** no. Visual lookup/insertion uses no-touch
   owner operations and cannot alter geometry order.
4. **Prompt20 patch leak fix:** still active; the explicit ownership tests and
   host destruction path remain green.
5. **Palace stride1/2 smokes:** both pass with RC0, complete trajectories,
   zero backend rejection, and zero eviction/erase/miss counters.
6. **eee_01 final P-LIVO smoke:** pass; RC0/evaluator RC0, complete trajectory,
   visual active, backend rejected zero.
7. **NTU 9×4 matrix:** complete, `NTU_ACCURACY_MATRIX_COMPLETE`.
8. **Oxford canonical LIVO full/stride matrix:** complete,
   `OXFORD_LIVO_MATRIX_COMPLETE`.
9. **Truly missing Oxford LIO cell:** corrected-config `College_03 / P-LIO`.
10. **College_03 corrected P-LIO:** newly filled, not reused; one deterministic
    canonical run was sufficient under Prompt21's rule.
11. **Native Oxford authority:** `NATIVE_BASELINE_CONFIG_DISTINCT_BUT_FROZEN`,
    effective `0.1/0.0025`, distinct from corrected Prob `0.5/0.01`.
12. **Additional full-pipeline rerun:** none is scientifically necessary;
    all required cells are accepted or explicitly filled.
13. **Ablation-only experiments:** Prompt18 strict-reclassification P1/P2;
    they do not redefine canonical P0/P0S/N0/N1.
14. **Out-of-scope visual-memory issue:** the multi-GiB persistent
    FAST-LIVO2-style visual/camera-state footprint. Prompt21 does not perform
    image-cache redesign, per-parent caps, long-term visual maps, adaptive
    selection, byte attribution, or memory optimization.

## 10. Stop boundary

Prompt21 is closed. Do not automatically run NTU full bags, the Oxford full
matrix, Native reruns, additional memory optimization, image-cache redesign,
per-parent visual caps, a long-term map, or new ablations.
