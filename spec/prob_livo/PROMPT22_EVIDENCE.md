# Prompt22 — Final Controlled NTU/Oxford Matrix Evidence

## Final status

`PROMPT22 CLOSED — FINAL CONTROLLED NTU/OXFORD MATRIX COMPLETE`

Prompt source: `/home/lc/super_livo/prompts/PROMPT22_FINAL_CONTROLLED_NTU_OXFORD_MATRIX.md`

Prompt source SHA256:
`405eebf6be05d951f67975d8fef80b68727ccadf192fd9c6c5812ef0686de1d8`

The exact run-level data are in
`spec/prob_livo/PROMPT22_RUN_LEDGER.csv`; the 52 final cells are in
`spec/prob_livo/PROMPT22_FINAL_MATRIX.csv`.

## A. State consensus

At Prompt22 startup the expected Prompt21 state was present: Prob `prob-livo`
at `ade2c15` and Native `prompt11-native-offline` at `ea7702b`, both clean
and synchronized. The Prompt22 benchmark tooling is now at local Prob
`cc3814a`; Native remained unchanged at `ea7702b` and remains synchronized.
The Prob worktree was clean before execution, and no estimator, offline
runner, ROS master, or roscore process was left running after execution.

The Prob offline binary was rebuilt successfully with:

`cmake --build /home/lc/super_livo/build --target prob_livo_offline -- -j4`

Compiler: GCC 9.4.0. The Native Release offline binary was already present
from the clean Native branch; no Native estimator source was changed.
Every new Oxford run used Release, affinity `0,2,4,6`, worker/MP limit 4,
serial slot execution, no RViz, no PCD output, and no dense-map output.

## B. Historical configuration bug reconstruction

The historical Oxford mismatch was real. Prompt15/18/19 Native effective
parameters used `preprocess/filter_size_surf=0.1` and
`lio/min_eigen_value=0.0025`. Prompt16 corrected Prob to the Oxford
dataset-author effective values but explicitly did not correct Native.
Prompt18/19 therefore froze a useful but non-controlled Native baseline.

The final Oxford profile uses `filter_size_surf=0.5` and
`min_eigen_value=0.01` for both backends. Old Native values remain archived
as historical-only; they do not enter the final matrix.

## C. NTU official authority audit

The final NTU authority is upstream FAST-LIVO2 commit
`0d2c0346107b75b59934975adec9a6eeeb913c64`, file `config/NTU_VIRAL.yaml`.
The [official NTU YAML](https://raw.githubusercontent.com/hku-mars/FAST-LIVO2/0d2c0346107b75b59934975adec9a6eeeb913c64/config/NTU_VIRAL.yaml)
has SHA256
`c8f94f130e599b928c3f02c3f3d3b2009ae01df76aec32f6ac96b6a987311ef3`,
which equals the local file byte-for-byte.

All nine NTU sequences (`eee_01..03`, `nya_01..03`, `sbs_01..03`) and all
three repetitions were audited. The 54 N/P repetition-pair audits (18
sequence/mode pairs × 3 repetitions) passed both the official-value audit
and shared-semantic identity audit. No NTU cell required rerunning, so the
108 valid Prompt15 NTU run artifacts are reused as the final 36 NTU cells.
The final NTU LIVO policy is relabelled `N/P-LIVO-STRIDE`, stride 1.

Prompt15 did not emit `selected_camera_timestamps.txt` for historical NTU
LIVO runs. Therefore this report claims the Prompt22-required NTU stride-1
classification and shared-config audit, but does not falsely claim the
Oxford-style file-level timestamp hash gate for those reused artifacts.

## D. Oxford dataset-author authority audit

The Oxford authority is `ori-drs/FAST-LIVO2`, branch `config-used-OSD`,
resolved at `f2c9abb72f82359bcb9190e31b8faa6b6b7b9a64`, file
`config/oxford_spires.yaml`. The [dataset-author Oxford YAML](https://raw.githubusercontent.com/ori-drs/FAST-LIVO2/config-used-OSD/config/oxford_spires.yaml)
has SHA256
`195a512df13734c69535c25ebd1bc3e033112c434796b8c2c4b5b4050c589db4`.

That source names `/alphasense_driver_ros/cam0/color/image` and contains the
source typo `common/image_en`; the local bags expose the decoded compressed
transport `/alphasense_driver_ros/cam0/debayered/image/compressed`. The final
profile records this as a transport adaptation, with the same decoded image
stream and header semantics in paired Native/Prob runs. Omitted effective
values were materialized from the audited source branch: the source
`LIVMapper.cpp` SHA is
`f8ea2da0ba793cf2d96e77bf0e46f9f1330370e7b6a13366475693e4da1d47b9` and the
source `voxel_map.cpp` SHA is
`9b5ee8914d6cc0cb96e2ba75ec034401370e38bed397442f49eb687dc6b9b856`.

The benchmark-owned effective profile is
`config/prob_livo/OXFORD_OSD_FINAL_SHARED.yaml`, SHA256
`1d49d420012c8d984cc354a88ee82c8bb59ebd523cc696b504cca8e737e60c7e`.
The camera profile is `config/camera_OXFORD_SPIRES.yaml`, SHA256
`f405c12d79b2e41f952f9cd818da2536de1a55ae9bb0fa87bb12377bc724822b`.

## E. Shared-semantic config crosswalk

The complete normalized crosswalk is
`PROMPT22_SHARED_CONFIG_MANIFEST.csv`. Shared topics, timestamp and
extrinsic semantics, IMU settings, LiDAR type/scan/blind/filter settings,
common LIO/visual thresholds, local-map policy, output disablement, and
camera stride were treated as `SHARED_EXACT` or explicitly `MODE_INACTIVE`.
Only backend-owned map/backend/plane-policy controls were `BACKEND_INTRINSIC`.

The Oxford final normalized SHA values are:

| Mode | Native/Prob shared SHA | Keys |
|---|---|---:|
| LIO | `013beef79fcb75fdd6680d3d107a235e9904afbac1d6cb17e57790c9c8e67420` | 62 |
| LIVO | `860ff168e9b2e89f291e4aeb0d741ce822e18ff2bc6cd57a37f7d58da4a7ddc0` | 74 |

The NTU audited SHA values are `76e7349c106135cd6afbc042251d9d81688d5aac5b6fd3976c429616ab764263`
for LIO and `a02de9fde32902582637605791cdf050028efa3b54171d07a92b216f289b5d90`
for LIVO.

## F. Backend-intrinsic whitelist

The only final differences not claimed as shared semantics are the Prob
backend selector, Prob Super parent/subvoxel map resolution/capacity,
Prob's visual plane gate, and Prob's native-blind-carry bucket policy. Native
and Prob do not expose one-to-one controls for those representations. Their
exact values and producers are recorded in the final rows of the shared
manifest; they were not used to excuse any common sensor, preprocess, IMU,
threshold, stride, or output mismatch.

## G. Config identity validator and negative mutation

`tools/prob_livo/validate_prompt22_config.py` normalizes the declared shared
keys and writes `shared_semantic_config.json` and
`shared_semantic_config.sha256` into each admitted run directory.
`validate_prompt22_pair.sh` additionally requires exact Oxford LIVO selected
camera timestamp file identity. The adversarial mutation test changed one
shared filter value and returned:

`PROMPT22 negative mutation test: PASS (shared identity gate rejects mutation)`

No Oxford formal slot entered the final matrix without the gate.

## H. Oxford canary

The College_03 canary was run once per canonical arm under the final profile.
All four returned RC0, complete trajectories, evaluator RC0, and visual-active
LIVO status. The canary selected-camera count was 2841 and the LIVO pair
identity gate passed.

| Arm | Status | Trajectory rows | Notes |
|---|---|---:|---|
| N-LIO | VALID | 2865 | camera off |
| P-LIO | VALID | 2861 | backend rejected 0 |
| N-LIVO-STRIDE | VALID | 2838 | visual commits 2837 |
| P-LIVO-STRIDE | VALID | 2834 | visual commits 2832 |

## I. Oxford formal run ledger

All 48 canonical Oxford runs were completed: 4 sequences × 4 arms × 3
repetitions. All 48 are `VALID`, all pair gates passed, and all four final
camera streams were exact within every Native/Prob LIVO pair:

| Sequence | Stride | Selected count | Selected timestamp SHA |
|---|---:|---:|---|
| Church_05 | 1 | 3961 | `ada0f71ab84fe97d78a9d7ae4c17418f08876e98599731687d32f5778ef30336` |
| College_03 | 2 | 2841 | `30df87679ddd892df4026617ebf7c051e788d4e65905bcbffd6191be7598b089` |
| Palace_01 | 2 | 4021 | `b313f82875991a00193731dbf99d240b5c3db8cdf21e0e2ce24611fe05352986` |
| Quarter_01 | 2 | 2873 | `4308c1d304108bf0ff767ac33bc467d3300deb479803233cce9030576f8d15fc` |

Two pre-formal execution failures are retained but excluded: one sandboxed
canary ROS XML-RPC `Operation not permitted` artifact and one first formal
Church P-LIO affinity-string parsing failure. Both occurred before estimator
execution and were superseded by the clean retry; neither is a final matrix
gap.

## J. NTU reused final matrix

Final classification: `NTU_FINAL_MATRIX_REUSED — OFFICIAL_CONFIG_IDENTITY VERIFIED`.
The values below are means ± population standard deviation over the three
Prompt15 repetitions; LIVO names are the Prompt22 stride-1 relabelling.

| Sequence | N-LIO | P-LIO | N-LIVO-STRIDE | P-LIVO-STRIDE |
|---|---:|---:|---:|---:|
| eee_01 | 0.030760±0.000000 | 0.049511±0.000000 | 0.030000±0.001716 | 0.052103±0.000099 |
| eee_02 | 0.021901±0.000000 | 0.017281±0.000000 | 0.035967±0.000085 | 0.027541±0.000167 |
| eee_03 | 0.031572±0.000000 | 0.029984±0.000000 | 0.030214±0.000025 | 0.029665±0.000000 |
| nya_01 | 0.027764±0.000000 | 0.023256±0.000000 | 0.037922±0.000060 | 0.038840±0.000008 |
| nya_02 | 0.029234±0.000000 | 0.029557±0.000000 | 0.036970±0.000001 | 0.046926±0.000052 |
| nya_03 | 0.028883±0.000000 | 0.025757±0.000000 | 0.033337±0.000007 | 0.031924±0.000021 |
| sbs_01 | 0.027766±0.000000 | 0.025729±0.000000 | 0.028969±0.000046 | 0.029509±0.000000 |
| sbs_02 | 0.026968±0.000000 | 0.024137±0.000000 | 0.028706±0.000018 | 0.028534±0.000007 |
| sbs_03 | 0.033383±0.000000 | 0.025554±0.000000 | 0.031164±0.000012 | 0.028208±0.000028 |

## K. Oxford corrected final accuracy matrix

| Sequence | N-LIO | P-LIO | N-LIVO-STRIDE | P-LIVO-STRIDE |
|---|---:|---:|---:|---:|
| Church_05 | 0.167800±0.000000 | 0.244000±0.000000 | 0.142933±0.002446 | 0.229933±0.003682 |
| College_03 | 0.048100±0.000000 | 0.074800±0.000000 | 0.070933±0.002819 | 0.182500±0.127776 |
| Palace_01 | 0.079500±0.000000 | 0.127200±0.000000 | 0.103133±0.000896 | 0.130767±0.000249 |
| Quarter_01 | 0.049500±0.000000 | 0.051000±0.000000 | 0.047200±0.000294 | 0.064933±0.001452 |

All values are translation ATE RMSE in metres from the frozen project
evaluator: rigid SE(3), no scale, one-to-one timestamp association,
`max_diff=0.05 s`, body frame.

## L. Final compute matrix

Values are `Native/Prob`, in seconds. `wall` and `CPU` are estimator seam
totals; `offline` is end-to-end offline wall time. The full per-run ledger
and mean/std/min/max values are authoritative in the CSV.

### NTU

| Sequence | LIO wall | LIO CPU | LIO offline | LIVO wall | LIVO CPU | LIVO offline |
|---|---:|---:|---:|---:|---:|---:|
| eee_01 | 44.5/37.5 | 173.4/140.5 | 48.5/42.1 | 77.4/68.0 | 267.9/224.3 | 82.7/72.4 |
| eee_02 | 32.7/27.4 | 129.0/102.9 | 35.7/30.9 | 62.1/55.3 | 218.2/187.2 | 67.5/60.3 |
| eee_03 | 20.3/17.3 | 77.0/64.7 | 22.3/19.5 | 34.3/30.8 | 120.0/105.3 | 37.0/33.0 |
| nya_01 | 34.6/24.3 | 139.6/91.8 | 38.4/29.2 | 71.8/56.1 | 260.1/198.0 | 79.6/61.9 |
| nya_02 | 38.0/27.3 | 153.4/103.3 | 42.1/32.6 | 74.7/60.6 | 275.3/214.5 | 83.1/65.5 |
| nya_03 | 37.2/26.8 | 150.3/101.0 | 41.1/33.0 | 75.0/58.9 | 275.3/213.6 | 83.3/64.0 |
| sbs_01 | 34.4/24.0 | 137.0/90.4 | 39.7/27.8 | 61.1/50.6 | 222.2/175.4 | 66.7/54.9 |
| sbs_02 | 41.8/24.9 | 166.1/93.8 | 48.0/29.0 | 69.3/56.8 | 246.8/197.1 | 74.9/62.5 |
| sbs_03 | 33.1/24.8 | 133.6/93.3 | 36.9/28.9 | 66.7/52.5 | 239.8/181.8 | 74.0/56.8 |

### Oxford

| Sequence | LIO wall | LIO CPU | LIO offline | LIVO wall | LIVO CPU | LIVO offline |
|---|---:|---:|---:|---:|---:|---:|
| Church_05 | 111.6/110.0 | 377.9/380.5 | 127.8/127.8 | 125.9/248.2 | 322.0/549.7 | 183.0/310.1 |
| College_03 | 55.1/51.6 | 139.7/174.4 | 69.0/63.2 | 83.6/125.1 | 210.8/315.2 | 124.4/167.4 |
| Palace_01 | 70.2/64.8 | 193.3/220.9 | 85.4/79.1 | 106.1/182.7 | 274.0/434.9 | 162.7/240.9 |
| Quarter_01 | 65.7/59.2 | 170.1/202.4 | 79.7/71.3 | 94.6/130.6 | 237.5/339.7 | 137.2/174.4 |

## M. Final memory matrix

Values are `Native/Prob` mean MiB. The primary channels are process PSS and
USS, not pure estimator-map memory. Each final Oxford run also retains the
full progress curve in its `memory.csv`.

### NTU

| Sequence | LIO peak PSS | LIO final PSS | LIO peak USS | LIVO peak PSS | LIVO final PSS | LIVO peak USS |
|---|---:|---:|---:|---:|---:|---:|
| eee_01 | 2567/319 | 2566/318 | 2564/316 | 4416/2090 | 4416/2089 | 4413/2088 |
| eee_02 | 1696/274 | 1695/274 | 1693/272 | 3091/1678 | 3091/1678 | 3089/1676 |
| eee_03 | 1312/230 | 1312/230 | 1309/227 | 2203/1049 | 2203/1049 | 2201/1047 |
| nya_01 | 557/247 | 557/246 | 554/244 | 2286/2023 | 2286/2022 | 2283/2020 |
| nya_02 | 543/260 | 543/259 | 541/257 | 2400/2169 | 2400/2169 | 2398/2166 |
| nya_03 | 628/261 | 628/260 | 625/258 | 2308/2105 | 2308/2104 | 2306/2102 |
| sbs_01 | 1828/282 | 1828/281 | 1825/279 | 3379/1867 | 3379/1867 | 3377/1865 |
| sbs_02 | 1788/284 | 1787/284 | 1785/281 | 3476/1980 | 3476/1979 | 3473/1977 |
| sbs_03 | 1788/307 | 1788/306 | 1785/304 | 3496/2057 | 3496/2057 | 3494/2054 |

### Oxford

| Sequence | LIO peak PSS | LIO final PSS | LIO peak USS | LIVO peak PSS | LIVO final PSS | LIVO peak USS |
|---|---:|---:|---:|---:|---:|---:|
| Church_05 | 976/804 | 975/800 | 973/801 | 8046/8080 | 8045/8080 | 8043/8077 |
| College_03 | 579/626 | 573/622 | 575/623 | 5892/5956 | 5892/5956 | 5888/5952 |
| Palace_01 | 933/652 | 927/645 | 930/648 | 8016/7872 | 8016/7870 | 8012/7869 |
| Quarter_01 | 1049/521 | 1043/515 | 1046/518 | 5986/6053 | 5986/6053 | 5982/6051 |

## N. P/N ratios and wins

Each ratio is Prob / Native in the order `ATE, estimator wall, estimator
CPU, peak USS, final USS`. Exact per-sequence ratios are retained here in
compact form and are reproducible from `PROMPT22_FINAL_MATRIX.csv`.

### NTU LIO

| Sequence | ATE | wall | CPU | peak USS | final USS |
|---|---:|---:|---:|---:|---:|
| eee_01 | 1.610 | 0.842 | 0.810 | 0.123 | 0.123 |
| eee_02 | 0.789 | 0.837 | 0.798 | 0.161 | 0.160 |
| eee_03 | 0.950 | 0.852 | 0.841 | 0.174 | 0.173 |
| nya_01 | 0.838 | 0.702 | 0.657 | 0.441 | 0.440 |
| nya_02 | 1.011 | 0.719 | 0.673 | 0.476 | 0.474 |
| nya_03 | 0.892 | 0.720 | 0.672 | 0.414 | 0.413 |
| sbs_01 | 0.927 | 0.696 | 0.659 | 0.153 | 0.152 |
| sbs_02 | 0.895 | 0.596 | 0.565 | 0.157 | 0.157 |
| sbs_03 | 0.765 | 0.749 | 0.698 | 0.170 | 0.169 |

### NTU LIVO-STRIDE

| Sequence | ATE | wall | CPU | peak USS | final USS |
|---|---:|---:|---:|---:|---:|
| eee_01 | 1.737 | 0.878 | 0.837 | 0.473 | 0.473 |
| eee_02 | 0.766 | 0.891 | 0.858 | 0.542 | 0.542 |
| eee_03 | 0.982 | 0.898 | 0.877 | 0.475 | 0.475 |
| nya_01 | 1.024 | 0.781 | 0.761 | 0.885 | 0.884 |
| nya_02 | 1.269 | 0.810 | 0.779 | 0.903 | 0.903 |
| nya_03 | 0.958 | 0.785 | 0.776 | 0.911 | 0.911 |
| sbs_01 | 1.019 | 0.828 | 0.789 | 0.552 | 0.552 |
| sbs_02 | 0.994 | 0.820 | 0.799 | 0.569 | 0.569 |
| sbs_03 | 0.905 | 0.787 | 0.758 | 0.588 | 0.588 |

NTU wins: Prob wins ATE on 7/9 LIO and 5/9 LIVO-STRIDE sequences. Median
ratios are LIO `0.895/0.720/0.673/0.170/0.169` and LIVO
`0.994/0.820/0.789/0.569/0.569`.

### Oxford ratios

| Mode/sequence | ATE | wall | CPU | peak USS | final USS |
|---|---:|---:|---:|---:|---:|
| LIO/Church_05 | 1.454 | 0.986 | 1.007 | 0.823 | 0.819 |
| LIO/College_03 | 1.555 | 0.935 | 1.248 | 1.082 | 1.085 |
| LIO/Palace_01 | 1.600 | 0.922 | 1.143 | 0.697 | 0.695 |
| LIO/Quarter_01 | 1.030 | 0.902 | 1.190 | 0.495 | 0.492 |
| LIVO/Church_05 | 1.609 | 1.972 | 1.707 | 1.004 | 1.004 |
| LIVO/College_03 | 2.573 | 1.497 | 1.496 | 1.011 | 1.011 |
| LIVO/Palace_01 | 1.268 | 1.722 | 1.587 | 0.982 | 0.982 |
| LIVO/Quarter_01 | 1.376 | 1.381 | 1.430 | 1.011 | 1.011 |

Oxford wins: Native wins ATE on all 4 LIO and all 4 LIVO-STRIDE sequences.
Median Prob/Native ratios are LIO `1.505/0.929/1.167/0.760/0.757` and
LIVO `1.492/1.609/1.541/1.008/1.008`. Thus Prob is faster and lower-memory
in Oxford LIO wall/most memory cells, but its estimator CPU is higher; in
Oxford LIVO it is slower and near-equal/slightly higher memory, while less
accurate on every sequence.

## O. Historical-to-final crosswalk

`PROMPT22_HISTORICAL_TO_FINAL_CROSSWALK.csv` records the sequence-level
crosswalk. NTU is `PROMPT15_REUSED_AFTER_FINAL_CONFIG_AUDIT`. Old Oxford
Native N-LIO/N-LIVO values are
`HISTORICAL_ONLY_WRONG_FINAL_SHARED_CONFIG`; old corrected Prob values are
`HISTORICAL_VALID_CORRECT_CONFIG` but are replaced in the same-era final
resource matrix. Prompt18 P1/P2 strict reclassification remains
`ABLATION_ONLY`.

## P. Stride input identity

Oxford selected-camera identities passed exactly for all 24 LIVO formal runs
(12 Native/Prob pairs across three repetitions). Church uses stride 1;
College, Palace, and Quarter use stride 2. There is no adaptive or
ATE-dependent stride. NTU is the accepted reused stride-1 family under the
Prompt22 reuse rule; its historical selected-timestamp files were not
available, as noted in section C.

## Q. Failure/deviation ledger

| Artifact | Classification | Admission |
|---|---|---|
| unsandboxed College canary | EXECUTION_FAIL before estimator (`Operation not permitted`) | excluded; escalated retry valid |
| first Church P-LIO formal | EXECUTION_FAIL before estimator (affinity parser received `0+2+4+6`) | excluded; corrected retry valid |
| all 48 corrected Oxford slots | VALID | admitted |
| all 108 reused NTU slots | VALID | admitted after official/config audit |

No estimator-math change, parameter tuning, strict-reclass arm, fifth
canonical arm, full-rate Oxford LIVO, 32-core run, or new stride sweep was
introduced.

## R. Final canonical configuration table

| Semantic key | NTU final | Oxford final | Authority |
|---|---|---|---|
| LiDAR / IMU / image topics | `/os1_cloud_node1/points` / `/imu/imu` / `/left/image_raw` | `/hesai/pandar` / `/alphasense_driver_ros/imu` / local decoded cam0 compressed transport | official NTU; Oxford OSD + transport adaptation |
| LiDAR type / scan lines | 3 / 16 | 5 / 64 | dataset authority |
| point filter / blind | 3 / 1.0 | 1 / 1.0 | dataset authority |
| surface filter | 0.1 | 0.5 | NTU YAML; Oxford `LIVMapper` default |
| LiDAR time offset | -0.1 | 0.0 | authority/default |
| IMU integration frame | 30 | 1 | authority/default |
| IMU acc/gyr covariance | 0.5 / 0.3 | 0.1 / 0.1 | authority/default |
| LIO max iterations / min eigen | 5 / 0.0025 | 5 / 0.01 | authority/default |
| voxel / max layer | 0.5 / 2 | 0.5 / 1 | authority/default |
| visual patch / pyramid / covariance | 8 / 3 / 100 | 8 / 4 / 100 | authority/default |
| camera stride | 1 for all nine | Church 1; others 2 | frozen Prompt18/19 policy |

These values are exact between Native and Prob within each dataset family;
they are not required to be identical between NTU and Oxford.

## S. Final owner-facing conclusions

1. The historical Oxford P/N shared-config mismatch was real: Native used
   `0.1/0.0025`, while the Oxford-author effective values are `0.5/0.01`.
2. It happened because Prompt16 corrected Prob only and explicitly excluded
   Native; Prompt18/19 then froze the old Native baseline.
3. Prompt22 uses the dataset-author Oxford profile plus materialized source
   defaults and the documented local compressed-image transport adaptation.
4. All shared-semantic Oxford P/N parameters are exact-identical; LIO and
   LIVO pair SHAs passed for every formal repetition.
5. Prompt15 NTU matches official `0d2c034` semantics across all audited
   shared keys and all 54 repetition-pair audits.
6. No NTU cell required rerunning.
7. The final per-sequence ATE results are in sections J/K and the final CSV.
8. Sections L and the CSV report estimator wall, estimator CPU, and offline
   wall separately.
9. Sections M and the CSV report Peak/Final PSS and USS; these are offline
   system process measurements, not pure map memory.
10. On NTU, Prob saves median wall/CPU and PSS/USS in both LIO and
    LIVO-STRIDE; on Oxford, Prob LIO saves median wall and memory but uses
    more CPU, while Prob LIVO is slower and roughly equal/slightly higher
    memory.
11. Prob wins NTU ATE 7/9 LIO and 5/9 LIVO; Native wins every Oxford final
    ATE comparison.
12. Corrected Oxford Native results are not adjudicated against the old
    wrong-config Native numbers; the old numbers are historical-only and
    the new corrected Native is the final authority.
13. Historical Prompt15/18/19 Oxford Native values are historical-only;
    historical corrected Prob values remain valid context but are replaced
    by this same-envelope matrix. P1/P2 remain ablation-only.
14. There is no final accuracy/compute/memory matrix gap. The only explicit
    provenance limitation is the missing historical NTU LIVO timestamp-file
    artifact, which is disclosed and is outside the Oxford-only file gate.
15. No further NTU/Oxford rerun is scientifically necessary. Prompt22 is
    closed and execution stops here.
