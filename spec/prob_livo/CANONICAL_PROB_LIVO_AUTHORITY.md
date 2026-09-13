# Canonical Prob-LIVO authority

## Production decision

The canonical production algorithm is the Prompt22 FEJ-OFF Prob-LIVO tree at
commit
`94fe832d826e464417ff90693d1f2aafae5b255a` (tree
`2ef27db5a0a9ec63136d011992926048bd78af4c`). Prompt22's shared NTU/Oxford
matrix, configuration manifest, run order, and online/offline runner are the
production evidence authority.

The production contract is the existing P0–P4 Prob-LIVO backend with the
Prompt20/21 visual lifetime and memory corrections, `fast_native`/dataset
configuration as declared by the canonical files, and the existing offline
runner. Prompt27's RGB registered-cloud publication is retained as a
publication-only compatibility fix; it does not change estimator state,
association, covariance, scheduling, or trajectory semantics.

Prompt39 publishes a documentation-only, Prompt22-derived 4-way
Native/Prob LIO/LIVO-STRIDE stride/resource matrix. It is an audit and
comparison artifact, not a new production authority or estimator variant:
`CANONICAL_4WAY_STRIDE_RESOURCE_MATRIX.md`, the companion CSV, and the
provenance note identify the exact Prompt22 run rows and aggregation rules.

## Explicitly superseded work

- Prompt23 is retained only as an Oxford/NTU stride ablation archive.
- Prompt24–26 FEJ experiments are not production authority. Only explicitly
  marked FEJ-OFF control rows are preserved in
  `archive/canonical_controls/`.
- Prompt29–34 common-linearization/CN implementation and evidence are
  rejected and absent from production.
- Prompt35–38 SA-CN/SA-LIVO implementation and evidence are rejected and
  absent from production.
- Prompt11 Native offline code is historical archive material only; it is not
  linked into the canonical Prob-LIVO build.

## Production invariants

The production search scope is `include src config launch CMakeLists.txt`.
That scope contains no cross-modal FEJ, common-linearization, CN, or SA
implementation, configuration, launch, or test path. Historical descriptions
and audit material may name superseded experiments, but they are outside the
production search scope and are not executable dependencies.

The canonical branch must remain buildable with `catkin_make -j4`, use the
existing TBB-backed offline runner, and keep local `prob-livo` equal to
`origin/prob-livo` after publication.

The external `vikit_common` dependency is resolved from the clean local
official source `/home/lc/design_ws/src/common/rpg_vikit` at
`6c886c8e5d83997806e00294826d528cea3581dd`, remote
`https://github.com/xuankuzcr/rpg_vikit.git`, after sourcing
`/home/lc/design_ws/devel/setup.bash`.
