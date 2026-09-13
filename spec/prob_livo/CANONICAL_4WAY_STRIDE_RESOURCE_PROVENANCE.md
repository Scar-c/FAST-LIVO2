# Canonical 4-way stride/resource matrix provenance

## Authority and scope

This document records the derivation of
`CANONICAL_4WAY_STRIDE_RESOURCE_MATRIX.md` and
`CANONICAL_4WAY_STRIDE_RESOURCE_MATRIX.csv`. The matrix is documentation-only
and is derived exclusively from the final Prompt22 FEJ-OFF evidence. It does
not introduce a new estimator, runner, configuration, experiment, or
production authority.

Prompt22 authority:

```text
commit: 94fe832d826e464417ff90693d1f2aafae5b255a
tree:   2ef27db5a0a9ec63136d011992926048bd78af4c
cleanup anchor: c0921ed2884eae561eaebeb3596f22d327fc1418
```

The exact source files are:

```text
spec/prob_livo/PROMPT22_RUN_LEDGER.csv
spec/prob_livo/PROMPT22_FINAL_MATRIX.csv
spec/prob_livo/PROMPT22_SHARED_CONFIG_MANIFEST.csv
spec/prob_livo/PROMPT22_EVIDENCE.md
```

The final matrix contains 13 sequences × 4 arms × 1 aggregate cell, with
three admitted repetitions per cell:

```text
Prompt22 run-ledger rows: 156
matrix aggregate cells: 52
repetitions per cell: 3
invalid or missing final cells: 0
```

The arms are exactly `N-LIO`, `P-LIO`, `N-LIVO-STRIDE`, and
`P-LIVO-STRIDE`. The dataset split is 9 NTU sequences and 4 Oxford
sequences. All run IDs in the CSV are the three exact Prompt22 ledger IDs
for their cell; they are not regenerated labels.

## Metric derivation

ATE RMSE, estimator wall/CPU time, offline wall time, PSS, USS, visual call
and commit counts, backend rejection counts, trajectory rows, configuration
hashes, shared-semantic hashes, source heads, and validity are copied or
mechanically aggregated from the Prompt22 final matrix and run ledger. Every
aggregate cell was reverse-checked against its three source ledger rows.

Peak and final RSS are taken directly from the Prompt22 run ledger and
aggregated in Prompt39 using the same three-run cell grouping. RSS was not
copied from Prompt19 or any earlier resource matrix. For each metric, the
reported mean, median, and standard deviation use the Prompt22 convention:
population standard deviation (`ddof=0`). The convention was independently
reconstructed from the three admitted values and matched the Prompt22
aggregate fields; no sample-standard-deviation substitution was made.

The CSV preserves exact run-level traceability through its `run_ids` field.
It retains the three raw Prompt22 stride values in `prompt22_raw_stride` and
the three source-head values in `source_heads` when applicable. No metric,
timestamp, resource value, or workload count was inferred from a missing run.
The only semantic mapping is the documented effective camera-stride label:
LIO is `N/A (camera off)`, while LIVO retains the verified Prompt22 stride.

## Stride and modality policy

For the nine NTU sequences, Prompt22 verifies LIVO stride 1. For Oxford,
`Church_05` uses stride 1 and `College_03`, `Palace_01`, and `Quarter_01`
use stride 2. LIO is camera-off in all LIO rows, so its effective camera
stride is explicitly represented as `N/A (camera off)` rather than inventing
a numeric camera stride. The raw Prompt22 field remains in the CSV for
auditability.

## Superseded material and semantic exclusions

Prompt19 is historical Oxford stride/resource-development context and is not
the final Oxford authority. Earlier Prompt15/18/19 Oxford or stride matrices
are superseded for this final comparison by Prompt22's corrected shared
configuration and final four-way matrix. Prompt23 remains an ablation-only
archive and is not merged into these cells.

No mixed FEJ/CN/SA evidence is present. The matrix contains no FEJ-on,
common-linearization/CN, SA-CN, or SA-LIVO rows; all rows come from the
Prompt22 FEJ-OFF final controls. Prompt11 and Prompt23 archives remain
preserved and unchanged.

No dataset was rerun in Prompt39. No estimator semantic, production source,
configuration, runner, test, or build file was changed. All validation is
static derivation, exact aggregate reproduction, source-row traceability,
and production-tree/archive checks.

## Dependency note

The canonical build dependency `vikit_common` resolves after sourcing
`/home/lc/design_ws/devel/setup.bash` to:

```text
package path: /home/lc/design_ws/src/common/rpg_vikit/vikit_common
source repo: /home/lc/design_ws/src/common/rpg_vikit
remote: https://github.com/xuankuzcr/rpg_vikit.git
source HEAD: 6c886c8e5d83997806e00294826d528cea3581dd
source status --porcelain: empty (clean)
```

`rospack` emitted only the environment warning that it could not create its
cache under `/home/lc/.ros` because that directory is read-only; the package
lookup itself succeeded and no dependency checkout was modified.

## Machine-check summary

```text
aggregate_reproduction: PASS
rss_reproduction: PASS
run_id_traceability: PASS
Prompt22 rows: 156
matrix rows: 52
repetitions/cell: 3
new experiments: 0
```
