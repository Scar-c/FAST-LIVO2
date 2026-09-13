# Preserved post-Prompt22 evidence

Prompt22 remains the numerical and semantic authority. Later work was
reviewed artifact-by-artifact and only evidence that is compatible with that
authority was retained.

## Retained

- Prompt20 and Prompt21 evidence and tests remain in their original paths.
- Prompt23 numeric/configuration material is under
  `archive/ablation/prompt23/` and is explicitly ablation-only.
- Prompt24 FEJ-OFF rows are extracted unchanged under
  `archive/canonical_controls/`; FEJ-ON rows and implementation are not
  production evidence.
- Prompt25 FEJ-OFF rows from the mixed ultimate matrix are extracted unchanged
  under `archive/canonical_controls/`; its FEJ-ON runner and implementation
  are not retained in production.
- Prompt27 Bright Native FEJ-OFF visual evidence and RViz screenshots are
  retained as a visualization control. The canonical launch is a sanitized
  `launch/prob_livo_online_avia.launch`, and the RViz layout is
  `rviz_cfg/prompt27_fast_livo2.rviz`.
- Prompt28's Bright Prob FEJ-OFF finite-output row and screenshots are
  retained as a visual canary. Its external BIEVR/COIN adapter, resource, and
  parity material is excluded.
- Prompt11's unique Native offline reader evidence is preserved in the
  historical archive, without making Native offline production-authoritative.

## Excluded

All post-Prompt22 executable FEJ, common-linearization/CN, and SA-CN/SA-LIVO
source, headers, tests, launch/configuration parameters, runners, and their
implementation-bound evidence were excluded from the reconstructed production
tree. The canonical Prompt22 files listed in the cleanup prompt are retained
unchanged.
