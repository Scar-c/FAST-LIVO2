# Preserved post-Prompt22 controls

The CSV files in this directory are exact row extractions from later mixed
ledgers. Only rows explicitly marked FEJ-OFF (and the Bright visual control
rows) were retained; headers and retained field values are unchanged. These
artifacts document compatibility checks and do not define a new estimator
authority.

Provenance:

- `PROMPT24_*_FEJ_OFF.csv`: source commit `a20ac9d`, rows where `fej=off`.
- `PROMPT25_ULTIMATE_FEJ_MATRIX_FEJ_OFF.csv`: source commit `5d38780`, rows
  where `fej=off`; Prompt22 rows are retained as a cross-check.
- `PROMPT27_RUN_LEDGER_BRIGHT_NATIVE_FEJ_OFF.csv` and `visual/`: source
  commit `2f4249c`, Bright Native FEJ-OFF visualization control.
- `PROMPT28_RUN_LEDGER_BRIGHT_PROB_FEJ_OFF.csv` and `visual/`: source commit
  `8743e83`, Bright Prob FEJ-OFF finite-output control.

The GEODE degeneracy artifacts, external BIEVR/COIN resources, and all FEJ
implementation files were intentionally excluded.
