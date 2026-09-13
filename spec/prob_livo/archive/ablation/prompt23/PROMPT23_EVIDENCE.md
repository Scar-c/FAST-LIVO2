# Prompt23 — Oxford NTU-style LIVO-STRIDE Sensitivity Evidence

## Final status

`PROMPT23 CLOSED — OXFORD NTU-STYLE LIVO STRIDE ABLATION COMPLETE`

Prompt source:
`/home/lc/super_livo/prompts/Prompt 23 — Oxford NTU-Style LIVO-STRIDE Sensitivity.md`

Prompt source SHA256:
`f480e311b1532c6d20cac0c71097be7827fcf7b8442e8ca496f87dba21438e88`

Prompt22 remains the final canonical dataset-author benchmark. Prompt23 is
only a controlled Oxford LIVO-STRIDE sensitivity ablation.

## 1. State and scope

At entry Prob was clean at `94fe832`, synchronized with `origin/prob-livo`;
Native was clean at `ea7702b`, synchronized with
`origin/prompt11-native-offline`. No estimator, runner, ROS master, or roscore
process remained from Prompt22. Prompt23 changed no estimator math.

The scope was exactly four Oxford sequences, LIVO-STRIDE only, and four
comparison quantities: N-OSD, P-OSD, N-NTU, and P-NTU. No LIO, full-rate
LIVO, NTU, strict-reclass, or additional sweep was run.

## 2. Configuration contract

The Prompt23 profile
`config/prob_livo/OXFORD_NTU_STYLE_LIVO_STRIDE_SHARED.yaml` is derived from
the Prompt22 Oxford profile. Its SHA256 is
`8277649c80b735a16f753a40f5ecc21f9d80d00a732a9dbedf5ab06290f0dbf5`.
Exactly two shared semantic values changed:

| Key | Prompt22 OSD | Prompt23 NTU-style | Change |
|---|---:|---:|---:|
| `preprocess/filter_size_surf` | 0.5 | 0.1 | allowed ablation change |
| `lio/min_eigen_value` | 0.01 | 0.0025 | allowed ablation change |

All topics, LiDAR type/scan lines, blind, point filter, timing offsets,
extrinsics, IMU, visual, exposure, stride, and output/resource settings were
inherited unchanged. The exact diff is in
`PROMPT23_CONFIG_CROSSWALK.csv`.

The Prompt23 shared identity digest is
`cf1a1b293bbcb9de62819e1c7dac5335cfacfe362919ad14caca902d9021df0f` over 72
active shared/camera keys. The only excluded fields are the two Prompt22
manifest `MODE_INACTIVE` output defaults retained by the historical Native
artifact: `publish/blind_rgb_points` and `pcd_save/filter_size_pcd`. They are
not estimator inputs and were not silently used to hide an active semantic
mismatch.

## 3. Native historical reuse audit

Prompt18/19 Native stride artifacts were audited before launching Prob.
All 12 Native runs were complete, visual-active, evaluator RC0, 4-core,
Release, and had complete effective parameters, counters, timing, memory,
offline-system, trajectory, evaluator, and selected-camera artifacts.
Their effective active shared values match the Prompt23 profile. Therefore
all 12 were classified:

`N-NTU_REUSED_VALID_HISTORICAL`

The Native source was the historical clean branch state
`84757c4`; its raw historical config file SHA was
`87bd926073ed49c68ab330617e7ca52c0e0b0470ab2c63cdba89aac2312aca92`.
The normalized Prompt23 profile SHA above is the comparison identity.

## 4. Camera stride identity

The frozen policy was Church_05=1 and College_03/Palace_01/Quarter_01=2.
Every N-NTU/P-NTU pair passed exact selected-camera timestamp comparison:

| Sequence | Stride | Count | Timestamp SHA |
|---|---:|---:|---|
| Church_05 | 1 | 3961 | `ada0f71ab84fe97d78a9d7ae4c17418f08876e98599731687d32f5778ef30336` |
| College_03 | 2 | 2841 | `30df87679ddd892df4026617ebf7c051e788d4e65905bcbffd6191be7598b089` |
| Palace_01 | 2 | 4021 | `b313f82875991a00193731dbf99d240b5c3db8cdf21e0e2ce24611fe05352986` |
| Quarter_01 | 2 | 2873 | `4308c1d304108bf0ff767ac33bc467d3300deb479803233cce9030576f8d15fc` |

All 12 pair admissions passed the 72-key config identity gate and camera
identity gate.

## 5. Formal Prob runs and canary

The College_03 P-NTU canary returned RC0, complete trajectory, evaluator RC0,
visual-active output, and ATE RMSE `0.0932 m`. The formal matrix then ran
serially with Release, affinity `0,2,4,6`, worker limit 4, no RViz/PCD/dense
map, and no parallel package execution.

All 12 P-NTU formal runs returned `VALID`; all backend rejection counts were
zero. The run ledger contains both the 12 reused Native rows and the 12 new
Prob rows.

## 6. Final sensitivity table

Each cell is `mean ± population std; median; min/max`, in metres. N-OSD and
P-OSD are the Prompt22 final canonical values. N-NTU is the valid historical
Native reuse; P-NTU is the new formal ablation.

| Sequence | N-OSD | P-OSD | N-NTU | P-NTU | ΔN | ΔP | gap OSD | gap NTU |
|---|---|---|---|---|---:|---:|---:|---:|
| Church_05 | 0.142933±0.002446; 0.143800; 0.139600/0.145400 | 0.229933±0.003682; 0.229600; 0.225600/0.234600 | 0.209333±0.000736; 0.209400; 0.208400/0.210200 | 0.229867±0.001223; 0.230300; 0.228200/0.231100 | +0.066400 | -0.000067 | +0.087000 | +0.020533 |
| College_03 | 0.070933±0.002819; 0.071100; 0.067400/0.074300 | 0.182500±0.127776; 0.092900; 0.091400/0.363200 | 0.051367±0.000047; 0.051400; 0.051300/0.051400 | 0.090500±0.001273; 0.091400; 0.088700/0.091400 | -0.019567 | -0.092000 | +0.111567 | +0.039133 |
| Palace_01 | 0.103133±0.000896; 0.103500; 0.101900/0.104000 | 0.130767±0.000249; 0.130700; 0.130500/0.131100 | 0.154900±0.000748; 0.154700; 0.154100/0.155900 | 0.128367±0.000591; 0.128000; 0.127900/0.129200 | +0.051767 | -0.002400 | +0.027633 | -0.026533 |
| Quarter_01 | 0.047200±0.000294; 0.047300; 0.046800/0.047500 | 0.064933±0.001452; 0.065700; 0.062900/0.066200 | 0.066200±0.000000; 0.066200; 0.066200/0.066200 | 0.066533±0.001438; 0.067500; 0.064500/0.067600 | +0.019000 | +0.001600 | +0.017733 | +0.000333 |

The exact numeric source is
`PROMPT23_FINAL_MATRIX.csv`; the exact 24 run rows are in
`PROMPT23_RUN_LEDGER.csv`.

## 7. Required owner answers

1. **Can Native historical data be reused?** Yes. All 12 Prompt18/19 Native
   stride repetitions satisfy the active config, stride, 4-core, evaluator,
   completeness, visual-active, and artifact requirements. Two inactive
   output defaults were explicitly excluded by the Prompt23 gate.
2. **P-LIVO ATE at 0.1/0.0025:** Church `0.229867`, College `0.090500`,
   Palace `0.128367`, Quarter `0.066533` m.
3. **How many wins?** P-NTU beats N-NTU on 1/4 sequences: Palace_01.
4. **Winner changes vs Prompt22:** Palace_01 changes from Native winner under
   OSD to Prob winner under NTU-style settings. Church, College, and Quarter
   remain Native wins.
5. **Prob sensitivity:** ΔP is Church `-0.000067`, College `-0.092000`,
   Palace `-0.002400`, Quarter `+0.001600` m. The large College improvement
   is real but remains above N-NTU.
6. **Native sensitivity:** ΔN is Church `+0.066400`, College `-0.019567`,
   Palace `+0.051767`, Quarter `+0.019000` m. Native is also highly
   sequence-dependent under the same two changes.
7. **High sensitivity?** Yes, the Oxford result is configuration-sensitive,
   especially College_03 and Palace_01; the direction is not universal.
8. **Paper positioning:** This does not support a claim that Prob generally
   surpasses Native on Oxford. It is a controlled sensitivity/diagnostic
   ablation; Prompt22 OSD remains the fair canonical dataset-author result.
9. **Factorial scan needed?** No. Prompt23 explicitly stops before a separate
   filter/eigen factorial; no scientific decision here authorizes that sweep.
10. **Does Prompt22 remain canonical?** Yes, unchanged. Prompt23 does not
    replace or rewrite the Prompt22 final matrix.

## 8. Final closure

There are no failed formal P-NTU cells and no unresolved pair or stride gate.
The 0.1/0.0025 comparison is complete as an ablation. Stop here; do not
automatically launch a two-dimensional filter/eigen scan, new datasets, or
algorithm tuning.
