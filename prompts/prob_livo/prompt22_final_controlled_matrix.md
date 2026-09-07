# Prompt22 — Final Controlled NTU/Oxford Matrix

This file registers the Prompt22 execution contract and its final artifacts.

- Source prompt: `/home/lc/super_livo/prompts/PROMPT22_FINAL_CONTROLLED_NTU_OXFORD_MATRIX.md`
- Source SHA256: `405eebf6be05d951f67975d8fef80b68727ccadf192fd9c6c5812ef0686de1d8`
- Primary repository: `/home/lc/super_livo/src/FAST-LIVO2`
- Branch: `prob-livo`
- Canonical arms: `N-LIO`, `P-LIO`, `N-LIVO-STRIDE`, `P-LIVO-STRIDE`
- Oxford scope: four sequences × four arms × three repetitions = 48 new runs
- NTU scope: Prompt15 nine-sequence matrix audited and reused; no full rerun
- Required evidence: `spec/prob_livo/PROMPT22_EVIDENCE.md`
- Required ledger: `spec/prob_livo/PROMPT22_RUN_LEDGER.csv`
- Required final matrix: `spec/prob_livo/PROMPT22_FINAL_MATRIX.csv`
- Config manifest: `spec/prob_livo/PROMPT22_SHARED_CONFIG_MANIFEST.csv`
- Historical crosswalk: `spec/prob_livo/PROMPT22_HISTORICAL_TO_FINAL_CROSSWALK.csv`

The final status and owner-stop boundaries are recorded in the evidence
report. Prompt22 stops after publication; it authorizes no later tuning,
ablation, dataset expansion, or memory optimization.
