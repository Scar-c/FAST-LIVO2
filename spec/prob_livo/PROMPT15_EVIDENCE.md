# Prompt15 benchmark evidence

## Predeclaration snapshot

This file was created and committed before any Prompt15 estimator run.  It
freezes the authorities, inventory, resource budget, Oxford canary, and the
156-slot execution ledger.  Formal measurements and the final status are
appended only after the required gates.

### Authorities at entry

| Authority | Branch | Local | Origin | Entry tree |
|---|---|---|---|---|
| Prob | `prob-livo` | `9c3eb35ff0da92e774173d357dd82dd3800611ce` | same | clean |
| Native benchmark | `prompt11-native-offline` | `bf01385b13f897f3335840bf9f88e9331d887cec` | same | clean |
| Native upstream algorithm | reference only | `0d2c0346107b75b59934975adec9a6eeeb913c64` | n/a | n/a |

Native benchmark description:

> FAST-LIVO2 native algorithm with correctness, offline-execution and
> ABI-safety adaptations; estimator design and parameters unchanged.

The Prob and Native origin SHAs were checked with `git ls-remote` before this
predeclaration.  No Prompt15 algorithm result existed when the ledger was
generated.

### Dataset inventory

The inventory records the exact canonical input selected for this round.  NTU
ground truth is the `/leica/pose/relative` stream in the same bag.  Oxford
ground truth is the sequence-local `gt-tum.txt`; its canonical input is the
previously payload/order-validated three-topic `*_LIVO.bag`.

| Family | Sequence | Canonical bag (bytes; duration s) | LiDAR | IMU | Camera | GT | Calibration / adapter | Inventory status |
|---|---|---|---|---|---|---|---|---|
| NTU | eee_01 | `NTU/eee_01/eee_01.bag` (9294291860; 398.687010) | `/os1_cloud_node1/points` PointCloud2, 3987 | `/imu/imu` Imu, 153347 | `/left/image_raw` Image, 3986 | PoseStamped, 6616 | sequence YAML; none | READY |
| NTU | eee_02 | `NTU/eee_02/eee_02.bag` (7482627250; 321.001350) | PointCloud2, 3210 | Imu, 122984 | Image, 3209 | PoseStamped, 5512 | sequence YAML; none | READY |
| NTU | eee_03 | `NTU/eee_03/eee_03.bag` (4261528188; 181.353318) | PointCloud2, 1814 | Imu, 70475 | Image, 1813 | PoseStamped, 2990 | sequence YAML; none | READY |
| NTU | nya_01 | `NTU/nya_01/nya_01.bag` (9277407427; 396.217330) | PointCloud2, 3950 | Imu, 153239 | Image, 3947 | PoseStamped, 7769 | sequence YAML; none | READY |
| NTU | nya_02 | `NTU/nya_02/nya_02.bag` (10072427976; 428.644501) | PointCloud2, 4287 | Imu, 166428 | Image, 4285 | PoseStamped, 7679 | sequence YAML; none | READY |
| NTU | nya_03 | `NTU/nya_03/nya_03.bag` (9619323787; 411.222204) | PointCloud2, 4094 | Imu, 159131 | Image, 4093 | PoseStamped, 7793 | sequence YAML; none | READY |
| NTU | sbs_01 | `NTU/sbs_01/sbs_01.bag` (8322101107; 354.145769) | PointCloud2, 3542 | Imu, 137552 | Image, 3540 | PoseStamped, 5623 | sequence YAML; none | READY |
| NTU | sbs_02 | `NTU/sbs_02/sbs_02.bag` (8768337329; 373.201326) | PointCloud2, 3732 | Imu, 145288 | Image, 3731 | PoseStamped, 6062 | sequence YAML; none | READY |
| NTU | sbs_03 | `NTU/sbs_03/sbs_03.bag` (9146010044; 389.222871) | PointCloud2, 3893 | Imu, 151294 | Image, 3891 | PoseStamped, 5434 | sequence YAML; none | READY |
| Oxford | Church_05 | `OXFORD/Church_05/Church_05_LIVO.bag` (7165664272; 799.989255) | `/hesai/pandar` PointCloud2, 8007 | `/alphasense_driver_ros/imu` Imu, 319608 | cam0 CompressedImage, 3961 | TUM, 11503 rows | `OXFORD/Calibration`; decode only | READY |
| Oxford | College_03 | `OXFORD/College_03/College_03_LIVO.bag` (3800779019; 285.991387) | PointCloud2, 2867 | Imu, 114268 | cam0 CompressedImage, 5682 | TUM, 7343 rows | same; decode only | READY / CANARY |
| Oxford | Palace_01 | `OXFORD/Palace_01/Palace_01_LIVO.bag` (4722792709; 403.995257) | PointCloud2, 4052 | Imu, 161400 | cam0 CompressedImage, 8041 | TUM, 10290 rows | same; decode only | READY |
| Oxford | Quarter_01 | `OXFORD/Quarter_01/Quarter_01_LIVO.bag` (3941294957; 288.981624) | PointCloud2, 2894 | Imu, 115459 | cam0 CompressedImage, 5746 | TUM, 7430 rows | same; decode only | READY |

NTU bags also contain the right image, second Ouster, Ouster-internal IMUs,
magnetic/temperature/UWB and platform/visualization streams; these are outside
the frozen three-sensor estimator input.  Oxford source bags contain cam1 and
cam2; the canonical LZ4 bags deliberately contain exactly cam0, IMU, and
LiDAR.  Image transport is raw `sensor_msgs/Image` for NTU and
`sensor_msgs/CompressedImage` for Oxford.  The only Oxford transport adapter
allowed by the benchmark is decode to `sensor_msgs/Image` with the original
header timestamp.

Oxford evaluator contract is the project-audited rigid SE(3) Umeyama
translation APE, no scale, one-to-one nearest timestamp association with
`max_diff=0.05 s`, comparing the IMU/body `W_T_B` estimate to the supplied
TUM world-frame trajectory.  All four Oxford sequences have authoritative GT.

### Frozen resources and ordering

- Logical CPUs: `0,2,4,6`.
- Physical cores: `0,1,2,3`, socket 0, node 0.
- Affinity mask: `taskset -c 0,2,4,6` for the entire offline process.
- Native: compile-time `MP_PROC_NUM=4`.
- Prob: TBB global maximum 4 and `OMP_NUM_THREADS=4`.
- College_03 is the Oxford canary because no current-repository Oxford run
  exists and it is the smallest complete authoritative-GT bag.
- Formal order: 39 sequence/repetition blocks; sequence starts rotate by
  0/4/8 for repetitions 1/2/3 and the four variants rotate inside each block.
  Failures are retained; replacements may only append rows referencing the
  original run ID.
- The complete predeclared matrix is
  `spec/prob_livo/PROMPT15_RUN_LEDGER.csv` (156 PENDING slots).

### Gates known before execution

- Authority gate: PASS.
- Dataset presence/topic/type/count gate: PASS.
- Resource-selection gate: PASS; runtime enforcement evidence pending.
- Config identity, first-LiDAR initialization identity, offline parity,
  Native-LIVO MP=4 envelope, measurement channel, and Oxford canary gates:
  PENDING.

## Prompt15 execution closure

### Final status

`PROMPT15 PARTIAL — DATASET/ALGORITHM REDS RECORDED`

All 156 predeclared offline slots returned wrapper RC0, complete sentinel, and evaluator RC0. The final status is partial because Oxford P-LIO has two incomplete-trajectory sequence groups and Oxford P-LIVO has three algorithm-RED groups. No artifact was silently deleted/replaced. No tuning, MP=1 fallback, P5, H1/H2, or I8 work was started.

Every numeric accuracy value in this report and the ledger is an authoritative translation ATE RMSE in metres: NTU uses `translation_ate_rmse_m` from the dataset-author evaluator, and Oxford uses the `translation APE (m): RMSE` field from the frozen SE(3)-no-scale evaluator. The independent ledger column is `ate_rmse_m`; the evaluator output remains in each run directory.

### Authorities, commits, builds, and resources

| Item | Value |
|---|---|
| Prob benchmark HEAD | `00f1246` |
| Prob estimator source used | `c69bae4` (LIVO IMU look-ahead correction) |
| Native benchmark HEAD | `0f4881a` |
| Native upstream authority | `0d2c0346107b75b59934975adec9a6eeeb913c64` |
| Native build | Release; `-O3 -march=native -mtune=native -mno-avx -funroll-loops -DEIGEN_MAX_ALIGN_BYTES=16`, MP=4 |
| Prob build | Release; `-O3 -march=native -mtune=native -funroll-loops`, TBB=4, OMP=4 |
| Affinity | `taskset -c 0,2,4,6` |

### Coverage and measurement channels

Formal slots **156/156**; wrapper-valid **156/156**; adjudicated incomplete **6**; algorithm-RED runs **9** across **3** groups. Artifact coverage: `effective_rosparams.yaml` 156/156, `trajectory.tum` 156/156, `trajectory.tum.counters.yaml` 156/156, `trajectory.tum.timing.yaml` 156/156, `trajectory.tum.visual_counters.yaml` 156/156, `offline_source.yaml` 156/156, `offline_system.yaml` 156/156, `memory.csv` 156/156, `processing_complete.sentinel` 156/156. Estimator wall/process CPU are separate from bag reader/decode; memory is process RSS/PSS/USS, not pure map memory.

### Config and initialization

NTU: `config/NTU_VIRAL.yaml`, Ouster PointCloud2, `/imu/imu`, raw `/left/image_raw`. Oxford: `config/OXFORD_SPIRES.yaml`, Hesai PointCloud2, `/alphasense_driver_ros/imu`, cam0 CompressedImage decoded with original header. Shared topics, LiDAR/IMU fields, extrinsics/noise, scheduler, first-LiDAR gate, Release mode, affinity, and no PCD/dense-map output were fixed. Native FAST map/IEKF/visual definitions and Prob P0–P4 map/covariance/plane fields remain intrinsic; Prob map fields are `/prob_livo/map_resolution`, `/prob_livo/voxel_size`, `/prob_livo/map_capacity`. No `super_ntu_legacy` entered the matrix.

Formal init artifacts record first LiDAR/window/count/first-valid/output/state/gravity/P19 covariance. eee_01 FAST-native camera-off identity: first LiDAR `1609059013.0732379`; IMU window `1609059013.1756983..1609059013.264255`; 36 samples; first valid `1609059013.2689695`; identity R; zero p/v/bg/ba; gravity approximately `[-0.85180293,-0.00769921,9.77294595]`; retained P19 prior.

### eee_01 parity and Native MP=4 envelope

- Native N-LIO online↔offline: exact rows/timestamps/SHA/counters, 3985 rows, SHA prefix `8593887f`.
- Prob P-LIO online↔offline: exact rows/timestamps/SHA/counters, 3981 rows, SHA prefix `adc63ad5`.
- Prob P-LIVO after `c69bae4`: exact event/source rows/timestamps, 3979 rows; trajectory RMSE 0.006631 m; ATE 0.052060/0.052237 m. Before the fix both paths rejected 3978 RUN epochs due missing non-consuming IMU look-ahead; after it both have 3978 RUN epochs and 3977 visual commits, with expected EOF rejection.
- Native N-LIVO MP=4 online×3/offline×3: all six CLEAN_SUCCESS, 3983 rows, exact top event counts/timestamps, 3983 visual calls, 3982 visual commits, final visual map 132–142. Online-online RMSE max 0.111490 m; online-offline RMSE median 0.092126 m; acceptance `0.092126 <= 2*0.111490=0.222981` PASS. Combined ATE spread 0.005082 m; acceptance `0.005082 <= 2*0.005082=0.010164` PASS. `NATIVE_LIVO_MP4_ACCEPTED_ENVELOPE: PASS`.

### Oxford canary

College_03 all four offline variants were formally VALID, complete, evaluator RC0, and visual-active for LIVO. RMSE: N-LIO 0.0540 m, P-LIO 0.0748 m, N-LIVO 0.0745 m, P-LIVO 1.8350 m. P-LIVO quality behavior is retained as an algorithm result.

### Per-sequence/variant formal summary

| Family | Sequence | Variant | Adjudication | ATE RMSE mean m | ATE RMSE median m | Rows/rep | Est wall s | Est CPU s | Offline wall s | Peak USS KiB | Final USS KiB | Visual calls | Visual commits |
|---|---|---|---|---:|---:|---|---:|---:|---:|---:|---:|---|---|
| NTU | eee_01 | N-LIO | VALID | 0.030760 | 0.030760 | 3985,3985,3985 | 47.423 | 184.883 | 51.533 | 2625304 | 2623736 | 0,0,0 | 0,0,0 |
| NTU | eee_01 | P-LIO | VALID | 0.049511 | 0.049511 | 3981,3981,3981 | 37.124 | 139.791 | 41.466 | 324304 | 321972 | 0,0,0 | 0,0,0 |
| NTU | eee_01 | N-LIVO | VALID | 0.030000 | 0.028794 | 3983,3983,3983 | 81.305 | 283.515 | 86.995 | 4510492 | 4510492 | 3983,3983,3983 | 3982,3982,3982 |
| NTU | eee_01 | P-LIVO | VALID | 0.052103 | 0.052040 | 3979,3979,3979 | 73.448 | 242.729 | 78.236 | 2134092 | 2132620 | 3979,3979,3979 | 3977,3977,3977 |
| NTU | eee_02 | N-LIO | VALID | 0.021901 | 0.021901 | 3208,3208,3208 | 34.840 | 137.113 | 38.073 | 1734220 | 1732524 | 0,0,0 | 0,0,0 |
| NTU | eee_02 | P-LIO | VALID | 0.017281 | 0.017281 | 3204,3204,3204 | 28.597 | 107.791 | 32.247 | 279704 | 279084 | 0,0,0 | 0,0,0 |
| NTU | eee_02 | N-LIVO | VALID | 0.035967 | 0.035962 | 3206,3206,3206 | 66.621 | 236.646 | 71.176 | 3162028 | 3162028 | 3207,3207,3207 | 3205,3205,3205 |
| NTU | eee_02 | P-LIVO | VALID | 0.027541 | 0.027535 | 3202,3202,3202 | 57.270 | 194.280 | 61.174 | 1729332 | 1729332 | 3202,3202,3202 | 3200,3200,3200 |
| NTU | eee_03 | N-LIO | VALID | 0.031572 | 0.031572 | 1812,1812,1812 | 20.743 | 79.967 | 23.004 | 1340080 | 1340080 | 0,0,0 | 0,0,0 |
| NTU | eee_03 | P-LIO | VALID | 0.029984 | 0.029984 | 1808,1808,1808 | 16.369 | 61.608 | 18.547 | 231964 | 231964 | 0,0,0 | 0,0,0 |
| NTU | eee_03 | N-LIVO | VALID | 0.030214 | 0.030213 | 1810,1810,1810 | 36.572 | 129.564 | 39.213 | 2255344 | 2255344 | 1811,1811,1811 | 1809,1809,1809 |
| NTU | eee_03 | P-LIVO | VALID | 0.029665 | 0.029665 | 1806,1806,1806 | 33.399 | 113.988 | 35.702 | 1068352 | 1068352 | 1806,1806,1806 | 1804,1804,1804 |
| NTU | nya_01 | N-LIO | VALID | 0.027764 | 0.027764 | 3948,3948,3948 | 36.397 | 147.059 | 40.617 | 567808 | 567808 | 0,0,0 | 0,0,0 |
| NTU | nya_01 | P-LIO | VALID | 0.023256 | 0.023256 | 3944,3944,3944 | 26.085 | 98.012 | 31.160 | 249352 | 249352 | 0,0,0 | 0,0,0 |
| NTU | nya_01 | N-LIVO | VALID | 0.037922 | 0.037965 | 3945,3945,3945 | 78.868 | 284.762 | 88.212 | 2339600 | 2339600 | 3946,3946,3946 | 3944,3944,3944 |
| NTU | nya_01 | P-LIVO | VALID | 0.038840 | 0.038835 | 3941,3941,3941 | 59.657 | 212.057 | 64.556 | 2069264 | 2067700 | 3941,3941,3941 | 3939,3939,3939 |
| NTU | nya_02 | N-LIO | VALID | 0.029234 | 0.029234 | 4284,4284,4284 | 41.405 | 166.814 | 45.729 | 553708 | 553332 | 0,0,0 | 0,0,0 |
| NTU | nya_02 | P-LIO | VALID | 0.029557 | 0.029557 | 4280,4280,4280 | 29.593 | 111.953 | 35.084 | 262168 | 261912 | 0,0,0 | 0,0,0 |
| NTU | nya_02 | N-LIVO | VALID | 0.036970 | 0.036970 | 4283,4283,4283 | 82.773 | 305.572 | 90.348 | 2466780 | 2466780 | 4283,4283,4283 | 4282,4282,4282 |
| NTU | nya_02 | P-LIVO | VALID | 0.046926 | 0.046963 | 4279,4279,4279 | 65.725 | 232.733 | 71.102 | 2216764 | 2216764 | 4279,4279,4279 | 4277,4277,4277 |
| NTU | nya_03 | N-LIO | VALID | 0.028883 | 0.028883 | 4092,4092,4092 | 38.822 | 156.621 | 42.944 | 639596 | 639596 | 0,0,0 | 0,0,0 |
| NTU | nya_03 | P-LIO | VALID | 0.025757 | 0.025757 | 4088,4088,4088 | 27.955 | 105.896 | 32.865 | 276060 | 276060 | 0,0,0 | 0,0,0 |
| NTU | nya_03 | N-LIVO | VALID | 0.033337 | 0.033332 | 4090,4090,4090 | 76.407 | 282.764 | 83.336 | 2371344 | 2371344 | 4091,4091,4091 | 4089,4089,4089 |
| NTU | nya_03 | P-LIVO | VALID | 0.031924 | 0.031920 | 4086,4086,4086 | 62.921 | 225.992 | 68.588 | 2152572 | 2152572 | 4086,4086,4086 | 4084,4084,4084 |
| NTU | sbs_01 | N-LIO | VALID | 0.027766 | 0.027766 | 3540,3540,3540 | 37.802 | 150.724 | 42.029 | 1868924 | 1868924 | 0,0,0 | 0,0,0 |
| NTU | sbs_01 | P-LIO | VALID | 0.025729 | 0.025729 | 3536,3536,3536 | 24.892 | 94.303 | 28.861 | 286280 | 284704 | 0,0,0 | 0,0,0 |
| NTU | sbs_01 | N-LIVO | VALID | 0.028969 | 0.028938 | 3538,3538,3538 | 63.645 | 233.366 | 69.087 | 3458396 | 3458396 | 3538,3538,3538 | 3537,3537,3537 |
| NTU | sbs_01 | P-LIVO | VALID | 0.029509 | 0.029509 | 3534,3534,3534 | 53.956 | 186.562 | 58.389 | 1908000 | 1908000 | 3534,3534,3534 | 3532,3532,3532 |
| NTU | sbs_02 | N-LIO | VALID | 0.026968 | 0.026968 | 3729,3729,3729 | 38.306 | 153.859 | 42.474 | 1827272 | 1825972 | 0,0,0 | 0,0,0 |
| NTU | sbs_02 | P-LIO | VALID | 0.024137 | 0.024137 | 3725,3725,3725 | 25.855 | 97.767 | 30.368 | 287800 | 286920 | 0,0,0 | 0,0,0 |
| NTU | sbs_02 | N-LIVO | VALID | 0.028706 | 0.028718 | 3728,3728,3728 | 73.303 | 262.948 | 78.995 | 3557548 | 3557548 | 3728,3728,3728 | 3727,3727,3727 |
| NTU | sbs_02 | P-LIVO | VALID | 0.028534 | 0.028538 | 3724,3724,3724 | 58.481 | 204.420 | 63.082 | 2021892 | 2021892 | 3724,3724,3724 | 3722,3722,3722 |
| NTU | sbs_03 | N-LIO | VALID | 0.033383 | 0.033383 | 3891,3891,3891 | 30.437 | 123.326 | 33.945 | 1828172 | 1828172 | 0,0,0 | 0,0,0 |
| NTU | sbs_03 | P-LIO | VALID | 0.025554 | 0.025554 | 3887,3887,3887 | 22.324 | 84.065 | 25.893 | 311132 | 309696 | 0,0,0 | 0,0,0 |
| NTU | sbs_03 | N-LIVO | VALID | 0.031164 | 0.031161 | 3889,3889,3889 | 71.714 | 258.810 | 78.285 | 3593148 | 3593148 | 3890,3890,3890 | 3888,3888,3888 |
| NTU | sbs_03 | P-LIVO | VALID | 0.028208 | 0.028222 | 3885,3885,3885 | 50.166 | 173.245 | 54.295 | 2103276 | 2103276 | 3885,3885,3885 | 3883,3883,3883 |
| OXFORD | Church_05 | N-LIO | VALID | 0.226500 | 0.226500 | 8005,8005,8005 | 355.910 | 1107.448 | 370.837 | 1353856 | 1349828 | 0,0,0 | 0,0,0 |
| OXFORD | Church_05 | P-LIO | VALID | 0.244000 | 0.244000 | 8001,8001,8001 | 96.821 | 337.352 | 113.161 | 765720 | 761652 | 0,0,0 | 0,0,0 |
| OXFORD | Church_05 | N-LIVO | VALID | 0.211100 | 0.210800 | 3959,3959,3959 | 361.091 | 948.302 | 411.478 | 8582904 | 8578860 | 3960,3960,3960 | 3959,3959,3959 |
| OXFORD | Church_05 | P-LIVO | VALID | 0.228733 | 0.229200 | 3956,3956,3956 | 87.330 | 237.786 | 136.370 | 8279232 | 8279232 | 3956,3956,3956 | 3953,3953,3953 |
| OXFORD | College_03 | N-LIO | VALID | 0.054000 | 0.054000 | 2865,2865,2865 | 109.855 | 323.257 | 117.710 | 804676 | 804676 | 0,0,0 | 0,0,0 |
| OXFORD | College_03 | P-LIO | VALID | 0.074800 | 0.074800 | 2861,2861,2861 | 36.525 | 123.515 | 45.417 | 568316 | 568316 | 0,0,0 | 0,0,0 |
| OXFORD | College_03 | N-LIVO | VALID | 0.074600 | 0.074600 | 5673,5673,5673 | 166.763 | 441.709 | 223.180 | 11764564 | 11764564 | 5679,5679,5679 | 5672,5672,5672 |
| OXFORD | College_03 | P-LIVO | VALID + ALGORITHM_RED | 2.108167 | 2.181800 | 5673,5673,5673 | 34.263 | 77.365 | 95.447 | 10451572 | 10451572 | 5673,5673,5673 | 87,84,87 |
| OXFORD | Palace_01 | N-LIO | VALID | 0.147600 | 0.147600 | 4051,4051,4051 | 162.643 | 489.343 | 173.175 | 1311184 | 1308324 | 0,0,0 | 0,0,0 |
| OXFORD | Palace_01 | P-LIO | INCOMPLETE_TRAJECTORY | 111871.772500 | 111871.772500 | 1419,1419,1419 | 24.464 | 80.447 | 35.959 | 4454860 | 4454628 | 0,0,0 | 0,0,0 |
| OXFORD | Palace_01 | N-LIVO | VALID | 0.129367 | 0.129500 | 8030,8030,8030 | 255.168 | 685.208 | 347.012 | 16348464 | 16348464 | 8037,8037,8037 | 8029,8029,8029 |
| OXFORD | Palace_01 | P-LIVO | VALID + ALGORITHM_RED | 1.845100 | 1.845100 | 8031,8031,8031 | 45.479 | 103.025 | 132.910 | 14127892 | 14127892 | 8031,8031,8031 | 121,121,121 |
| OXFORD | Quarter_01 | N-LIO | VALID | 0.062800 | 0.062800 | 2893,2893,2893 | 163.052 | 491.829 | 172.321 | 1667880 | 1661992 | 0,0,0 | 0,0,0 |
| OXFORD | Quarter_01 | P-LIO | INCOMPLETE_TRAJECTORY | 22634.745900 | 22634.745900 | 1173,1173,1173 | 22.318 | 74.274 | 32.567 | 3450500 | 3450500 | 0,0,0 | 0,0,0 |
| OXFORD | Quarter_01 | N-LIVO | VALID | 0.072167 | 0.070900 | 5736,5736,5736 | 219.377 | 590.054 | 284.482 | 12052432 | 12048212 | 5743,5743,5743 | 5735,5735,5735 |
| OXFORD | Quarter_01 | P-LIVO | VALID + ALGORITHM_RED | 5.935600 | 5.935600 | 5737,5737,5737 | 37.959 | 87.912 | 108.671 | 10498168 | 10498168 | 5737,5737,5737 | 74,74,74 |

### Per-run lightweight metrics

L/I/C is source LiDAR/IMU/Image count; USS is MiB; visual is calls/commits.

| Slot | Dataset | Sequence | Variant | Rep | Status | RED | ATE RMSE m | GT matches | Rows | L/I/C | Est wall s | Est CPU s | Offline wall s | Peak USS MiB | Final USS MiB | Visual | Backend rejected |
|---|---|---|---|---:|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|
| P15-eee_01-N-LIO-r1 | NTU | eee_01 | N-LIO | 1 | VALID |  | 0.030760 | 3327 | 3985 | 3987/153347/0 | 37.624 | 146.701 | 41.308 | 2566.2 | 2566.2 | 0/0 | 0 |
| P15-eee_01-P-LIO-r1 | NTU | eee_01 | P-LIO | 1 | VALID |  | 0.049511 | 3327 | 3981 | 3987/153347/0 | 28.383 | 106.893 | 31.959 | 317.9 | 317.9 | 0/0 | 0 |
| P15-eee_01-N-LIVO-r1 | NTU | eee_01 | N-LIVO | 1 | VALID |  | 0.028778 | 3326 | 3983 | 3987/153347/3986 | 65.788 | 221.861 | 70.049 | 4404.8 | 4404.8 | 3983/3982 | 0 |
| P15-eee_01-P-LIVO-r1 | NTU | eee_01 | P-LIVO | 1 | VALID |  | 0.052243 | 3326 | 3979 | 3987/153347/3986 | 56.580 | 185.335 | 60.391 | 2096.9 | 2095.8 | 3979/3977 | 1 |
| P15-eee_02-P-LIO-r1 | NTU | eee_02 | P-LIO | 1 | VALID |  | 0.017281 | 2778 | 3204 | 3210/122984/0 | 21.561 | 81.084 | 24.615 | 273.1 | 272.5 | 0/0 | 0 |
| P15-eee_02-N-LIVO-r1 | NTU | eee_02 | N-LIVO | 1 | VALID |  | 0.035866 | 2777 | 3206 | 3210/122984/3209 | 51.522 | 177.041 | 55.023 | 3087.9 | 3087.9 | 3207/3205 | 0 |
| P15-eee_02-P-LIVO-r1 | NTU | eee_02 | P-LIVO | 1 | VALID |  | 0.027535 | 2777 | 3202 | 3210/122984/3209 | 44.956 | 151.786 | 48.089 | 1688.8 | 1688.8 | 3202/3200 | 0 |
| P15-eee_02-N-LIO-r1 | NTU | eee_02 | N-LIO | 1 | VALID |  | 0.021901 | 2778 | 3208 | 3210/122984/0 | 27.628 | 108.793 | 30.120 | 1695.3 | 1695.3 | 0/0 | 0 |
| P15-eee_03-N-LIVO-r1 | NTU | eee_03 | N-LIVO | 1 | VALID |  | 0.030244 | 1502 | 1810 | 1814/70475/1813 | 28.455 | 96.781 | 31.201 | 2202.9 | 2202.9 | 1811/1809 | 0 |
| P15-eee_03-P-LIVO-r1 | NTU | eee_03 | P-LIVO | 1 | VALID |  | 0.029665 | 1502 | 1806 | 1814/70475/1813 | 25.317 | 86.720 | 27.175 | 1053.1 | 1053.1 | 1806/1804 | 1 |
| P15-eee_03-N-LIO-r1 | NTU | eee_03 | N-LIO | 1 | VALID |  | 0.031572 | 1502 | 1812 | 1814/70475/0 | 16.185 | 63.114 | 17.810 | 1311.7 | 1311.7 | 0/0 | 0 |
| P15-eee_03-P-LIO-r1 | NTU | eee_03 | P-LIO | 1 | VALID |  | 0.029984 | 1502 | 1808 | 1814/70475/0 | 12.531 | 47.197 | 14.270 | 229.9 | 229.1 | 0/0 | 0 |
| P15-nya_01-P-LIVO-r1 | NTU | nya_01 | P-LIVO | 1 | VALID |  | 0.038851 | 3888 | 3941 | 3950/153239/3947 | 47.031 | 163.863 | 54.620 | 2021.4 | 2021.4 | 3941/3939 | 1 |
| P15-nya_01-N-LIO-r1 | NTU | nya_01 | N-LIO | 1 | VALID |  | 0.027764 | 3892 | 3948 | 3950/153239/0 | 28.717 | 116.097 | 31.816 | 555.8 | 555.8 | 0/0 | 0 |
| P15-nya_01-P-LIO-r1 | NTU | nya_01 | P-LIO | 1 | VALID |  | 0.023256 | 3888 | 3944 | 3950/153239/0 | 20.354 | 77.153 | 24.446 | 247.1 | 246.3 | 0/0 | 0 |
| P15-nya_01-N-LIVO-r1 | NTU | nya_01 | N-LIVO | 1 | VALID |  | 0.037965 | 3891 | 3945 | 3950/153239/3947 | 56.050 | 200.588 | 60.664 | 2284.8 | 2284.8 | 3946/3944 | 0 |
| P15-nya_02-N-LIO-r1 | NTU | nya_02 | N-LIO | 1 | VALID |  | 0.029234 | 3857 | 4284 | 4287/166428/0 | 29.760 | 121.554 | 33.431 | 542.6 | 542.6 | 0/0 | 0 |
| P15-nya_02-P-LIO-r1 | NTU | nya_02 | P-LIO | 1 | VALID |  | 0.029557 | 3857 | 4280 | 4287/166428/0 | 22.250 | 84.286 | 26.567 | 259.5 | 258.6 | 0/0 | 0 |
| P15-nya_02-N-LIVO-r1 | NTU | nya_02 | N-LIVO | 1 | VALID |  | 0.036970 | 3858 | 4283 | 4287/166428/4285 | 57.995 | 211.626 | 63.011 | 2414.3 | 2414.3 | 4283/4282 | 0 |
| P15-nya_02-P-LIVO-r1 | NTU | nya_02 | P-LIVO | 1 | VALID |  | 0.046963 | 3858 | 4279 | 4287/166428/4285 | 48.037 | 171.706 | 52.283 | 2170.9 | 2170.9 | 4279/4277 | 0 |
| P15-nya_03-P-LIO-r1 | NTU | nya_03 | P-LIO | 1 | VALID |  | 0.025757 | 3892 | 4088 | 4094/159131/0 | 21.676 | 82.235 | 29.971 | 233.8 | 233.8 | 0/0 | 0 |
| P15-nya_03-N-LIVO-r1 | NTU | nya_03 | N-LIVO | 1 | VALID |  | 0.033332 | 3895 | 4090 | 4094/159131/4093 | 60.196 | 218.969 | 64.941 | 2328.9 | 2328.9 | 4091/4089 | 0 |
| P15-nya_03-P-LIVO-r1 | NTU | nya_03 | P-LIVO | 1 | VALID |  | 0.031900 | 3892 | 4086 | 4094/159131/4093 | 46.276 | 170.787 | 50.438 | 2102.1 | 2102.1 | 4086/4084 | 1 |
| P15-nya_03-N-LIO-r1 | NTU | nya_03 | N-LIO | 1 | VALID |  | 0.028883 | 3896 | 4092 | 4094/159131/0 | 30.462 | 123.464 | 33.672 | 626.5 | 626.5 | 0/0 | 0 |
| P15-sbs_01-N-LIVO-r1 | NTU | sbs_01 | N-LIVO | 1 | VALID |  | 0.028938 | 2811 | 3538 | 3542/137552/3540 | 49.309 | 175.554 | 53.875 | 3378.0 | 3378.0 | 3538/3537 | 0 |
| P15-sbs_01-P-LIVO-r1 | NTU | sbs_01 | P-LIVO | 1 | VALID |  | 0.029509 | 2811 | 3534 | 3542/137552/3540 | 40.164 | 139.664 | 43.686 | 1871.4 | 1871.4 | 3534/3532 | 0 |
| P15-sbs_01-N-LIO-r1 | NTU | sbs_01 | N-LIO | 1 | VALID |  | 0.027766 | 2810 | 3540 | 3542/137552/0 | 26.889 | 108.661 | 30.039 | 1827.7 | 1827.7 | 0/0 | 0 |
| P15-sbs_01-P-LIO-r1 | NTU | sbs_01 | P-LIO | 1 | VALID |  | 0.025729 | 2810 | 3536 | 3542/137552/0 | 19.060 | 71.964 | 22.246 | 280.9 | 279.2 | 0/0 | 0 |
| P15-sbs_02-P-LIVO-r1 | NTU | sbs_02 | P-LIVO | 1 | VALID |  | 0.028525 | 3054 | 3724 | 3732/145288/3731 | 44.321 | 153.234 | 51.734 | 1974.2 | 1974.2 | 3724/3722 | 0 |
| P15-sbs_02-N-LIO-r1 | NTU | sbs_02 | N-LIO | 1 | VALID |  | 0.026968 | 3053 | 3729 | 3732/145288/0 | 28.653 | 116.265 | 31.917 | 1786.2 | 1786.2 | 0/0 | 0 |
| P15-sbs_02-P-LIO-r1 | NTU | sbs_02 | P-LIO | 1 | VALID |  | 0.024137 | 3053 | 3725 | 3732/145288/0 | 19.789 | 74.260 | 23.103 | 281.5 | 281.5 | 0/0 | 0 |
| P15-sbs_02-N-LIVO-r1 | NTU | sbs_02 | N-LIVO | 1 | VALID |  | 0.028718 | 3054 | 3728 | 3732/145288/3731 | 57.504 | 200.700 | 61.787 | 3474.2 | 3474.2 | 3728/3727 | 0 |
| P15-sbs_03-N-LIO-r1 | NTU | sbs_03 | N-LIO | 1 | VALID |  | 0.033383 | 2900 | 3891 | 3893/151294/0 | 29.910 | 120.967 | 33.558 | 1785.3 | 1785.3 | 0/0 | 0 |
| P15-sbs_03-P-LIO-r1 | NTU | sbs_03 | P-LIO | 1 | VALID |  | 0.025554 | 2900 | 3887 | 3893/151294/0 | 21.044 | 79.341 | 24.521 | 303.8 | 303.2 | 0/0 | 0 |
| P15-sbs_03-N-LIVO-r1 | NTU | sbs_03 | N-LIVO | 1 | VALID |  | 0.031150 | 2911 | 3889 | 3893/151294/3891 | 54.505 | 192.264 | 59.120 | 3510.4 | 3510.4 | 3890/3888 | 0 |
| P15-sbs_03-P-LIVO-r1 | NTU | sbs_03 | P-LIVO | 1 | VALID |  | 0.028222 | 2911 | 3885 | 3893/151294/3891 | 45.226 | 157.440 | 49.072 | 2057.4 | 2057.4 | 3885/3883 | 0 |
| P15-Church_05-P-LIO-r1 | OXFORD | Church_05 | P-LIO | 1 | VALID |  | 0.244000 | 7780 | 8001 | 8007/319608/0 | 71.621 | 249.753 | 87.178 | 747.8 | 743.8 | 0/0 | 0 |
| P15-Church_05-N-LIVO-r1 | OXFORD | Church_05 | N-LIVO | 1 | VALID |  | 0.212800 | 3959 | 3959 | 8007/319608/3961 | 290.791 | 747.863 | 333.631 | 8367.2 | 8367.2 | 3960/3959 | 0 |
| P15-Church_05-P-LIVO-r1 | OXFORD | Church_05 | P-LIVO | 1 | VALID |  | 0.229200 | 3956 | 3956 | 8007/319608/3961 | 81.480 | 220.610 | 127.690 | 8088.5 | 8088.5 | 3956/3953 | 0 |
| P15-Church_05-N-LIO-r1 | OXFORD | Church_05 | N-LIO | 1 | VALID |  | 0.226500 | 7784 | 8005 | 8007/319608/0 | 251.569 | 771.825 | 263.012 | 1322.1 | 1318.2 | 0/0 | 0 |
| P15-College_03-N-LIVO-r1 | OXFORD | College_03 | N-LIVO | 1 | VALID |  | 0.074600 | 5673 | 5673 | 2867/114268/5682 | 156.087 | 412.524 | 210.102 | 11476.8 | 11476.8 | 5679/5672 | 0 |
| P15-College_03-P-LIVO-r1 | OXFORD | College_03 | P-LIVO | 1 | VALID | ALGORITHM_RED | 1.157100 | 5673 | 5673 | 2867/114268/5682 | 32.569 | 72.536 | 91.839 | 10206.6 | 10206.6 | 5673/87 | 2814 |
| P15-College_03-N-LIO-r1 | OXFORD | College_03 | N-LIO | 1 | VALID |  | 0.054000 | 2864 | 2865 | 2867/114268/0 | 104.881 | 308.970 | 111.984 | 778.7 | 778.7 | 0/0 | 0 |
| P15-College_03-P-LIO-r1 | OXFORD | College_03 | P-LIO | 1 | VALID |  | 0.074800 | 2860 | 2861 | 2867/114268/0 | 33.033 | 111.234 | 41.167 | 555.2 | 555.2 | 0/0 | 0 |
| P15-Palace_01-P-LIVO-r1 | OXFORD | Palace_01 | P-LIVO | 1 | VALID | ALGORITHM_RED | 1.845100 | 8031 | 8031 | 4052/161400/8041 | 42.895 | 96.214 | 127.898 | 13921.3 | 13921.3 | 8031/121 | 3980 |
| P15-Palace_01-N-LIO-r1 | OXFORD | Palace_01 | N-LIO | 1 | VALID |  | 0.147600 | 4051 | 4051 | 4052/161400/0 | 138.587 | 414.023 | 147.620 | 1281.3 | 1278.5 | 0/0 | 0 |
| P15-Palace_01-P-LIO-r1 | OXFORD | Palace_01 | P-LIO | 1 | INCOMPLETE_TRAJECTORY | INCOMPLETE_TRAJECTORY | 111871.772500 | 1419 | 1419 | 4052/161400/0 | 24.464 | 80.447 | 35.959 | 4350.4 | 4350.4 | 0/0 | 2628 |
| P15-Palace_01-N-LIVO-r1 | OXFORD | Palace_01 | N-LIVO | 1 | VALID |  | 0.129500 | 8030 | 8030 | 4052/161400/8041 | 257.325 | 695.646 | 350.478 | 15962.5 | 15962.5 | 8037/8029 | 0 |
| P15-Quarter_01-N-LIO-r1 | OXFORD | Quarter_01 | N-LIO | 1 | VALID |  | 0.062800 | 2892 | 2893 | 2894/115459/0 | 147.672 | 444.396 | 156.289 | 1628.8 | 1623.0 | 0/0 | 0 |
| P15-Quarter_01-P-LIO-r1 | OXFORD | Quarter_01 | P-LIO | 1 | INCOMPLETE_TRAJECTORY | INCOMPLETE_TRAJECTORY | 22634.745900 | 1172 | 1173 | 2894/115459/0 | 22.116 | 73.439 | 31.898 | 3369.0 | 3369.0 | 0/0 | 1716 |
| P15-Quarter_01-N-LIVO-r1 | OXFORD | Quarter_01 | N-LIVO | 1 | VALID |  | 0.075300 | 5736 | 5736 | 2894/115459/5746 | 219.253 | 586.856 | 284.482 | 11783.2 | 11783.2 | 5743/5735 | 0 |
| P15-Quarter_01-P-LIVO-r1 | OXFORD | Quarter_01 | P-LIVO | 1 | VALID | ALGORITHM_RED | 5.935800 | 5737 | 5737 | 2894/115459/5746 | 37.999 | 89.059 | 108.671 | 10252.1 | 10252.1 | 5737/74 | 2843 |
| P15-nya_02-P-LIO-r2 | NTU | nya_02 | P-LIO | 2 | VALID |  | 0.029557 | 3857 | 4280 | 4287/166428/0 | 30.126 | 113.706 | 36.052 | 255.8 | 255.8 | 0/0 | 0 |
| P15-nya_02-N-LIVO-r2 | NTU | nya_02 | N-LIVO | 2 | VALID |  | 0.036970 | 3858 | 4283 | 4287/166428/4285 | 82.773 | 308.712 | 90.348 | 2409.0 | 2409.0 | 4283/4282 | 0 |
| P15-nya_02-P-LIVO-r2 | NTU | nya_02 | P-LIVO | 2 | VALID |  | 0.046852 | 3858 | 4279 | 4287/166428/4285 | 67.934 | 239.021 | 73.225 | 2162.3 | 2162.3 | 4279/4277 | 0 |
| P15-nya_02-N-LIO-r2 | NTU | nya_02 | N-LIO | 2 | VALID |  | 0.029234 | 3857 | 4284 | 4287/166428/0 | 42.863 | 171.978 | 47.117 | 540.7 | 540.4 | 0/0 | 0 |
| P15-nya_03-N-LIVO-r2 | NTU | nya_03 | N-LIVO | 2 | VALID |  | 0.033332 | 3895 | 4090 | 4094/159131/4093 | 88.466 | 324.223 | 101.599 | 2273.0 | 2273.0 | 4091/4089 | 0 |
| P15-nya_03-P-LIVO-r2 | NTU | nya_03 | P-LIVO | 2 | VALID |  | 0.031920 | 3892 | 4086 | 4094/159131/4093 | 67.411 | 243.980 | 72.944 | 2105.4 | 2105.4 | 4086/4084 | 1 |
| P15-nya_03-N-LIO-r2 | NTU | nya_03 | N-LIO | 2 | VALID |  | 0.028883 | 3896 | 4092 | 4094/159131/0 | 42.312 | 170.942 | 46.713 | 624.6 | 624.6 | 0/0 | 0 |
| P15-nya_03-P-LIO-r2 | NTU | nya_03 | P-LIO | 2 | VALID |  | 0.025757 | 3892 | 4088 | 4094/159131/0 | 30.730 | 114.841 | 36.171 | 271.6 | 269.7 | 0/0 | 0 |
| P15-sbs_01-P-LIVO-r2 | NTU | sbs_01 | P-LIVO | 2 | VALID |  | 0.029509 | 2811 | 3534 | 3542/137552/3540 | 57.747 | 199.997 | 62.766 | 1859.1 | 1859.1 | 3534/3532 | 0 |
| P15-sbs_01-N-LIO-r2 | NTU | sbs_01 | N-LIO | 2 | VALID |  | 0.027766 | 2810 | 3540 | 3542/137552/0 | 37.802 | 151.658 | 42.029 | 1825.1 | 1825.1 | 0/0 | 0 |
| P15-sbs_01-P-LIO-r2 | NTU | sbs_01 | P-LIO | 2 | VALID |  | 0.025729 | 2810 | 3536 | 3542/137552/0 | 27.932 | 104.802 | 32.400 | 279.6 | 278.0 | 0/0 | 0 |
| P15-sbs_01-N-LIVO-r2 | NTU | sbs_01 | N-LIVO | 2 | VALID |  | 0.029034 | 2811 | 3538 | 3542/137552/3540 | 70.427 | 257.743 | 77.234 | 3377.3 | 3377.3 | 3538/3537 | 0 |
| P15-sbs_02-N-LIO-r2 | NTU | sbs_02 | N-LIO | 2 | VALID |  | 0.026968 | 3053 | 3729 | 3732/145288/0 | 58.389 | 228.062 | 69.534 | 1784.4 | 1783.2 | 0/0 | 0 |
| P15-sbs_02-P-LIO-r2 | NTU | sbs_02 | P-LIO | 2 | VALID |  | 0.024137 | 3053 | 3725 | 3732/145288/0 | 29.109 | 109.435 | 33.608 | 280.2 | 280.2 | 0/0 | 0 |
| P15-sbs_02-N-LIVO-r2 | NTU | sbs_02 | N-LIVO | 2 | VALID |  | 0.028680 | 3054 | 3728 | 3732/145288/3731 | 77.151 | 276.762 | 83.796 | 3475.0 | 3475.0 | 3728/3727 | 0 |
| P15-sbs_02-P-LIVO-r2 | NTU | sbs_02 | P-LIVO | 2 | VALID |  | 0.028538 | 3054 | 3724 | 3732/145288/3731 | 67.702 | 233.688 | 72.781 | 1980.9 | 1980.7 | 3724/3722 | 0 |
| P15-sbs_03-P-LIO-r2 | NTU | sbs_03 | P-LIO | 2 | VALID |  | 0.025554 | 2900 | 3887 | 3893/151294/0 | 31.004 | 116.455 | 36.295 | 304.5 | 302.2 | 0/0 | 0 |
| P15-sbs_03-N-LIVO-r2 | NTU | sbs_03 | N-LIVO | 2 | VALID |  | 0.031180 | 2911 | 3889 | 3893/151294/3891 | 71.714 | 258.810 | 78.285 | 3508.9 | 3508.9 | 3890/3888 | 0 |
| P15-sbs_03-P-LIVO-r2 | NTU | sbs_03 | P-LIVO | 2 | VALID |  | 0.028168 | 2911 | 3885 | 3893/151294/3891 | 62.166 | 214.599 | 67.059 | 2050.0 | 2050.0 | 3885/3883 | 0 |
| P15-sbs_03-N-LIO-r2 | NTU | sbs_03 | N-LIO | 2 | VALID |  | 0.033383 | 2900 | 3891 | 3893/151294/0 | 38.944 | 156.377 | 43.223 | 1783.4 | 1783.4 | 0/0 | 0 |
| P15-Church_05-N-LIVO-r2 | OXFORD | Church_05 | N-LIVO | 2 | VALID |  | 0.209700 | 3959 | 3959 | 8007/319608/3961 | 398.448 | 1058.758 | 469.563 | 8390.7 | 8388.3 | 3960/3959 | 0 |
| P15-Church_05-P-LIVO-r2 | OXFORD | Church_05 | P-LIVO | 2 | VALID |  | 0.229700 | 3956 | 3956 | 8007/319608/3961 | 102.291 | 284.243 | 157.018 | 8085.2 | 8085.2 | 3956/3953 | 0 |
| P15-Church_05-N-LIO-r2 | OXFORD | Church_05 | N-LIO | 2 | VALID |  | 0.226500 | 7784 | 8005 | 8007/319608/0 | 355.910 | 1107.448 | 370.837 | 1322.4 | 1318.5 | 0/0 | 0 |
| P15-Church_05-P-LIO-r2 | OXFORD | Church_05 | P-LIO | 2 | VALID |  | 0.244000 | 7780 | 8001 | 8007/319608/0 | 105.097 | 364.109 | 122.864 | 745.1 | 741.1 | 0/0 | 0 |
| P15-College_03-P-LIVO-r2 | OXFORD | College_03 | P-LIVO | 2 | VALID | ALGORITHM_RED | 2.985600 | 5673 | 5673 | 2867/114268/5682 | 39.571 | 88.015 | 110.113 | 10227.7 | 10227.7 | 5673/84 | 2814 |
| P15-College_03-N-LIO-r2 | OXFORD | College_03 | N-LIO | 2 | VALID |  | 0.054000 | 2864 | 2865 | 2867/114268/0 | 144.981 | 430.385 | 154.599 | 785.8 | 785.8 | 0/0 | 0 |
| P15-College_03-P-LIO-r2 | OXFORD | College_03 | P-LIO | 2 | VALID |  | 0.074800 | 2860 | 2861 | 2867/114268/0 | 54.814 | 184.580 | 67.418 | 554.4 | 553.7 | 0/0 | 0 |
| P15-College_03-N-LIVO-r2 | OXFORD | College_03 | N-LIVO | 2 | VALID |  | 0.074400 | 5673 | 5673 | 2867/114268/5682 | 205.660 | 551.841 | 271.858 | 11488.8 | 11488.8 | 5679/5672 | 0 |
| P15-Palace_01-N-LIO-r2 | OXFORD | Palace_01 | N-LIO | 2 | VALID |  | 0.147600 | 4051 | 4051 | 4052/161400/0 | 175.182 | 528.038 | 186.620 | 1280.2 | 1277.4 | 0/0 | 0 |
| P15-Palace_01-P-LIO-r2 | OXFORD | Palace_01 | P-LIO | 2 | INCOMPLETE_TRAJECTORY | INCOMPLETE_TRAJECTORY | 111871.772500 | 1419 | 1419 | 4052/161400/0 | 32.983 | 107.983 | 48.683 | 4354.3 | 4348.3 | 0/0 | 2628 |
| P15-Palace_01-N-LIVO-r2 | OXFORD | Palace_01 | N-LIVO | 2 | VALID |  | 0.128900 | 8030 | 8030 | 4052/161400/8041 | 255.168 | 685.208 | 347.012 | 15967.2 | 15967.2 | 8037/8029 | 0 |
| P15-Palace_01-P-LIVO-r2 | OXFORD | Palace_01 | P-LIVO | 2 | VALID | ALGORITHM_RED | 1.845100 | 8031 | 8031 | 4052/161400/8041 | 55.862 | 121.963 | 162.775 | 13728.1 | 13728.1 | 8031/121 | 3980 |
| P15-Quarter_01-P-LIO-r2 | OXFORD | Quarter_01 | P-LIO | 2 | INCOMPLETE_TRAJECTORY | INCOMPLETE_TRAJECTORY | 22634.745900 | 1172 | 1173 | 2894/115459/0 | 22.318 | 74.274 | 32.567 | 3369.6 | 3369.6 | 0/0 | 1716 |
| P15-Quarter_01-N-LIVO-r2 | OXFORD | Quarter_01 | N-LIVO | 2 | VALID |  | 0.070300 | 5736 | 5736 | 2894/115459/5746 | 219.377 | 590.054 | 283.353 | 11739.2 | 11739.2 | 5743/5735 | 0 |
| P15-Quarter_01-P-LIVO-r2 | OXFORD | Quarter_01 | P-LIVO | 2 | VALID | ALGORITHM_RED | 5.935600 | 5737 | 5737 | 2894/115459/5746 | 36.685 | 86.180 | 105.131 | 10243.1 | 10243.0 | 5737/74 | 2843 |
| P15-Quarter_01-N-LIO-r2 | OXFORD | Quarter_01 | N-LIO | 2 | VALID |  | 0.062800 | 2892 | 2893 | 2894/115459/0 | 164.956 | 496.969 | 174.016 | 1628.9 | 1623.1 | 0/0 | 0 |
| P15-eee_01-P-LIO-r2 | NTU | eee_01 | P-LIO | 2 | VALID |  | 0.049511 | 3327 | 3981 | 3987/153347/0 | 46.866 | 174.956 | 52.738 | 316.7 | 314.4 | 0/0 | 0 |
| P15-eee_01-N-LIVO-r2 | NTU | eee_01 | N-LIVO | 2 | VALID |  | 0.032427 | 3326 | 3983 | 3987/153347/3986 | 81.305 | 283.515 | 86.995 | 4435.8 | 4435.8 | 3983/3982 | 0 |
| P15-eee_01-P-LIVO-r2 | NTU | eee_01 | P-LIVO | 2 | VALID |  | 0.052040 | 3326 | 3979 | 3987/153347/3986 | 73.448 | 242.729 | 78.236 | 2082.6 | 2082.6 | 3979/3977 | 1 |
| P15-eee_01-N-LIO-r2 | NTU | eee_01 | N-LIO | 2 | VALID |  | 0.030760 | 3327 | 3985 | 3987/153347/0 | 47.423 | 184.883 | 51.533 | 2562.2 | 2562.0 | 0/0 | 0 |
| P15-eee_02-N-LIVO-r2 | NTU | eee_02 | N-LIVO | 2 | VALID |  | 0.036074 | 2777 | 3206 | 3210/122984/3209 | 68.144 | 240.907 | 76.281 | 3087.7 | 3087.7 | 3207/3205 | 0 |
| P15-eee_02-P-LIVO-r2 | NTU | eee_02 | P-LIVO | 2 | VALID |  | 0.027749 | 2777 | 3202 | 3210/122984/3209 | 57.270 | 194.280 | 61.174 | 1693.7 | 1693.7 | 3202/3200 | 0 |
| P15-eee_02-N-LIO-r2 | NTU | eee_02 | N-LIO | 2 | VALID |  | 0.021901 | 2778 | 3208 | 3210/122984/0 | 34.840 | 137.113 | 38.073 | 1690.7 | 1690.7 | 0/0 | 0 |
| P15-eee_02-P-LIO-r2 | NTU | eee_02 | P-LIO | 2 | VALID |  | 0.017281 | 2778 | 3204 | 3210/122984/0 | 32.003 | 119.778 | 35.958 | 268.6 | 268.6 | 0/0 | 0 |
| P15-eee_03-P-LIVO-r2 | NTU | eee_03 | P-LIVO | 2 | VALID |  | 0.029665 | 1502 | 1806 | 1814/70475/1813 | 33.561 | 115.191 | 36.070 | 1043.3 | 1043.3 | 1806/1804 | 1 |
| P15-eee_03-N-LIO-r2 | NTU | eee_03 | N-LIO | 2 | VALID |  | 0.031572 | 1502 | 1812 | 1814/70475/0 | 23.990 | 87.781 | 26.220 | 1307.3 | 1307.3 | 0/0 | 0 |
| P15-eee_03-P-LIO-r2 | NTU | eee_03 | P-LIO | 2 | VALID |  | 0.029984 | 1502 | 1808 | 1814/70475/0 | 22.976 | 85.264 | 25.760 | 225.5 | 225.5 | 0/0 | 0 |
| P15-eee_03-N-LIVO-r2 | NTU | eee_03 | N-LIVO | 2 | VALID |  | 0.030213 | 1502 | 1810 | 1814/70475/1813 | 37.756 | 133.800 | 40.487 | 2198.2 | 2198.2 | 1811/1809 | 0 |
| P15-nya_01-N-LIO-r2 | NTU | nya_01 | N-LIO | 2 | VALID |  | 0.027764 | 3892 | 3948 | 3950/153239/0 | 36.397 | 147.059 | 40.617 | 552.8 | 552.8 | 0/0 | 0 |
| P15-nya_01-P-LIO-r2 | NTU | nya_01 | P-LIO | 2 | VALID |  | 0.023256 | 3888 | 3944 | 3950/153239/0 | 26.085 | 98.012 | 31.160 | 242.2 | 241.9 | 0/0 | 0 |
| P15-nya_01-N-LIVO-r2 | NTU | nya_01 | N-LIVO | 2 | VALID |  | 0.037965 | 3891 | 3945 | 3950/153239/3947 | 80.589 | 294.842 | 88.212 | 2279.6 | 2279.6 | 3946/3944 | 0 |
| P15-nya_01-P-LIVO-r2 | NTU | nya_01 | P-LIVO | 2 | VALID |  | 0.038835 | 3888 | 3941 | 3950/153239/3947 | 59.657 | 212.057 | 64.556 | 2020.8 | 2019.2 | 3941/3939 | 1 |
| P15-sbs_03-N-LIVO-r3 | NTU | sbs_03 | N-LIVO | 3 | VALID |  | 0.031161 | 2911 | 3889 | 3893/151294/3891 | 73.903 | 268.308 | 84.693 | 3463.0 | 3463.0 | 3890/3888 | 0 |
| P15-sbs_03-P-LIVO-r3 | NTU | sbs_03 | P-LIVO | 3 | VALID |  | 0.028233 | 2911 | 3885 | 3893/151294/3891 | 50.166 | 173.245 | 54.295 | 2054.0 | 2054.0 | 3885/3883 | 0 |
| P15-sbs_03-N-LIO-r3 | NTU | sbs_03 | N-LIO | 3 | VALID |  | 0.033383 | 2900 | 3891 | 3893/151294/0 | 30.437 | 123.326 | 33.945 | 1787.5 | 1787.5 | 0/0 | 0 |
| P15-sbs_03-P-LIO-r3 | NTU | sbs_03 | P-LIO | 3 | VALID |  | 0.025554 | 2900 | 3887 | 3893/151294/0 | 22.324 | 84.065 | 25.893 | 302.4 | 302.4 | 0/0 | 0 |
| P15-Church_05-P-LIVO-r3 | OXFORD | Church_05 | P-LIVO | 3 | VALID |  | 0.227300 | 3956 | 3956 | 8007/319608/3961 | 87.330 | 237.786 | 136.370 | 8043.9 | 8043.9 | 3956/3953 | 0 |
| P15-Church_05-N-LIO-r3 | OXFORD | Church_05 | N-LIO | 3 | VALID |  | 0.226500 | 7784 | 8005 | 8007/319608/0 | 380.081 | 1184.671 | 396.386 | 1319.8 | 1315.9 | 0/0 | 0 |
| P15-Church_05-P-LIO-r3 | OXFORD | Church_05 | P-LIO | 3 | VALID |  | 0.244000 | 7780 | 8001 | 8007/319608/0 | 96.821 | 337.352 | 113.161 | 758.7 | 754.8 | 0/0 | 0 |
| P15-Church_05-N-LIVO-r3 | OXFORD | Church_05 | N-LIVO | 3 | VALID |  | 0.210800 | 3959 | 3959 | 8007/319608/3961 | 361.091 | 948.302 | 411.478 | 8381.7 | 8377.8 | 3960/3959 | 0 |
| P15-College_03-N-LIO-r3 | OXFORD | College_03 | N-LIO | 3 | VALID |  | 0.054000 | 2864 | 2865 | 2867/114268/0 | 109.855 | 323.257 | 117.710 | 786.0 | 786.0 | 0/0 | 0 |
| P15-College_03-P-LIO-r3 | OXFORD | College_03 | P-LIO | 3 | VALID |  | 0.074800 | 2860 | 2861 | 2867/114268/0 | 36.525 | 123.515 | 45.417 | 555.0 | 555.0 | 0/0 | 0 |
| P15-College_03-N-LIVO-r3 | OXFORD | College_03 | N-LIVO | 3 | VALID |  | 0.074800 | 5673 | 5673 | 2867/114268/5682 | 166.763 | 441.709 | 223.180 | 11496.1 | 11496.1 | 5679/5672 | 0 |
| P15-College_03-P-LIVO-r3 | OXFORD | College_03 | P-LIVO | 3 | VALID | ALGORITHM_RED | 2.181800 | 5673 | 5673 | 2867/114268/5682 | 34.263 | 77.365 | 95.447 | 10194.8 | 10194.8 | 5673/87 | 2814 |
| P15-Palace_01-P-LIO-r3 | OXFORD | Palace_01 | P-LIO | 3 | INCOMPLETE_TRAJECTORY | INCOMPLETE_TRAJECTORY | 111871.772500 | 1419 | 1419 | 4052/161400/0 | 21.572 | 71.436 | 31.798 | 4350.2 | 4350.2 | 0/0 | 2628 |
| P15-Palace_01-N-LIVO-r3 | OXFORD | Palace_01 | N-LIVO | 3 | VALID |  | 0.129700 | 8030 | 8030 | 4052/161400/8041 | 216.689 | 582.632 | 298.758 | 15965.3 | 15965.3 | 8037/8029 | 0 |
| P15-Palace_01-P-LIVO-r3 | OXFORD | Palace_01 | P-LIVO | 3 | VALID | ALGORITHM_RED | 1.845100 | 8031 | 8031 | 4052/161400/8041 | 45.479 | 103.025 | 132.910 | 13796.8 | 13796.8 | 8031/121 | 3980 |
| P15-Palace_01-N-LIO-r3 | OXFORD | Palace_01 | N-LIO | 3 | VALID |  | 0.147600 | 4051 | 4051 | 4052/161400/0 | 162.643 | 489.343 | 173.175 | 1280.5 | 1277.7 | 0/0 | 0 |
| P15-Quarter_01-N-LIVO-r3 | OXFORD | Quarter_01 | N-LIVO | 3 | VALID |  | 0.070900 | 5736 | 5736 | 2894/115459/5746 | 225.554 | 604.947 | 292.563 | 11770.0 | 11765.8 | 5743/5735 | 0 |
| P15-Quarter_01-P-LIVO-r3 | OXFORD | Quarter_01 | P-LIVO | 3 | VALID | ALGORITHM_RED | 5.935400 | 5737 | 5737 | 2894/115459/5746 | 37.959 | 87.912 | 110.459 | 10282.2 | 10282.2 | 5737/74 | 2843 |
| P15-Quarter_01-N-LIO-r3 | OXFORD | Quarter_01 | N-LIO | 3 | VALID |  | 0.062800 | 2892 | 2893 | 2894/115459/0 | 163.052 | 491.829 | 172.321 | 1626.8 | 1620.8 | 0/0 | 0 |
| P15-Quarter_01-P-LIO-r3 | OXFORD | Quarter_01 | P-LIO | 3 | INCOMPLETE_TRAJECTORY | INCOMPLETE_TRAJECTORY | 22634.745900 | 1172 | 1173 | 2894/115459/0 | 22.513 | 75.061 | 32.681 | 3369.9 | 3369.9 | 0/0 | 1716 |
| P15-eee_01-N-LIVO-r3 | NTU | eee_01 | N-LIVO | 3 | VALID |  | 0.028794 | 3326 | 3983 | 3987/153347/3986 | 85.062 | 298.442 | 91.010 | 4399.9 | 4399.9 | 3983/3982 | 0 |
| P15-eee_01-P-LIVO-r3 | NTU | eee_01 | P-LIVO | 3 | VALID |  | 0.052025 | 3326 | 3979 | 3987/153347/3986 | 73.867 | 244.690 | 78.679 | 2084.1 | 2082.5 | 3979/3977 | 1 |
| P15-eee_01-N-LIO-r3 | NTU | eee_01 | N-LIO | 3 | VALID |  | 0.030760 | 3327 | 3985 | 3987/153347/0 | 48.359 | 188.645 | 52.605 | 2563.8 | 2562.2 | 0/0 | 0 |
| P15-eee_01-P-LIO-r3 | NTU | eee_01 | P-LIO | 3 | VALID |  | 0.049511 | 3327 | 3981 | 3987/153347/0 | 37.124 | 139.791 | 41.466 | 314.2 | 314.2 | 0/0 | 0 |
| P15-eee_02-P-LIVO-r3 | NTU | eee_02 | P-LIVO | 3 | VALID |  | 0.027339 | 2777 | 3202 | 3210/122984/3209 | 63.770 | 215.524 | 71.526 | 1644.0 | 1644.0 | 3202/3200 | 0 |
| P15-eee_02-N-LIO-r3 | NTU | eee_02 | N-LIO | 3 | VALID |  | 0.021901 | 2778 | 3208 | 3210/122984/0 | 35.655 | 141.049 | 39.013 | 1693.6 | 1691.9 | 0/0 | 0 |
| P15-eee_02-P-LIO-r3 | NTU | eee_02 | P-LIO | 3 | VALID |  | 0.017281 | 2778 | 3204 | 3210/122984/0 | 28.597 | 107.791 | 32.247 | 275.2 | 273.2 | 0/0 | 0 |
| P15-eee_02-N-LIVO-r3 | NTU | eee_02 | N-LIVO | 3 | VALID |  | 0.035962 | 2777 | 3206 | 3210/122984/3209 | 66.621 | 236.646 | 71.176 | 3090.9 | 3090.9 | 3207/3205 | 0 |
| P15-eee_03-N-LIO-r3 | NTU | eee_03 | N-LIO | 3 | VALID |  | 0.031572 | 1502 | 1812 | 1814/70475/0 | 20.743 | 79.967 | 23.004 | 1308.7 | 1308.7 | 0/0 | 0 |
| P15-eee_03-P-LIO-r3 | NTU | eee_03 | P-LIO | 3 | VALID |  | 0.029984 | 1502 | 1808 | 1814/70475/0 | 16.369 | 61.608 | 18.547 | 226.5 | 226.5 | 0/0 | 0 |
| P15-eee_03-N-LIVO-r3 | NTU | eee_03 | N-LIVO | 3 | VALID |  | 0.030184 | 1502 | 1810 | 1814/70475/1813 | 36.572 | 129.564 | 39.213 | 2202.5 | 2202.5 | 1811/1809 | 0 |
| P15-eee_03-P-LIVO-r3 | NTU | eee_03 | P-LIVO | 3 | VALID |  | 0.029665 | 1502 | 1806 | 1814/70475/1813 | 33.399 | 113.988 | 35.702 | 1043.3 | 1043.3 | 1806/1804 | 1 |
| P15-nya_01-P-LIO-r3 | NTU | nya_01 | P-LIO | 3 | VALID |  | 0.023256 | 3888 | 3944 | 3950/153239/0 | 26.513 | 100.154 | 32.053 | 243.5 | 243.5 | 0/0 | 0 |
| P15-nya_01-N-LIVO-r3 | NTU | nya_01 | N-LIVO | 3 | VALID |  | 0.037838 | 3891 | 3945 | 3950/153239/3947 | 78.868 | 284.762 | 89.791 | 2285.5 | 2285.5 | 3946/3944 | 0 |
| P15-nya_01-P-LIVO-r3 | NTU | nya_01 | P-LIVO | 3 | VALID |  | 0.038835 | 3888 | 3941 | 3950/153239/3947 | 61.629 | 218.041 | 66.657 | 2017.5 | 2017.5 | 3941/3939 | 1 |
| P15-nya_01-N-LIO-r3 | NTU | nya_01 | N-LIO | 3 | VALID |  | 0.027764 | 3892 | 3948 | 3950/153239/0 | 38.786 | 155.672 | 42.708 | 554.5 | 554.5 | 0/0 | 0 |
| P15-nya_02-N-LIVO-r3 | NTU | nya_02 | N-LIVO | 3 | VALID |  | 0.036972 | 3858 | 4283 | 4287/166428/4285 | 83.415 | 305.572 | 95.883 | 2371.6 | 2371.6 | 4283/4282 | 0 |
| P15-nya_02-P-LIVO-r3 | NTU | nya_02 | P-LIVO | 3 | VALID |  | 0.046963 | 3858 | 4279 | 4287/166428/4285 | 65.725 | 232.733 | 71.102 | 2164.8 | 2164.8 | 4279/4277 | 0 |
| P15-nya_02-N-LIO-r3 | NTU | nya_02 | N-LIO | 3 | VALID |  | 0.029234 | 3857 | 4284 | 4287/166428/0 | 41.405 | 166.814 | 45.729 | 538.5 | 538.5 | 0/0 | 0 |
| P15-nya_02-P-LIO-r3 | NTU | nya_02 | P-LIO | 3 | VALID |  | 0.029557 | 3857 | 4280 | 4287/166428/0 | 29.593 | 111.953 | 35.084 | 256.0 | 255.0 | 0/0 | 0 |
| P15-nya_03-P-LIVO-r3 | NTU | nya_03 | P-LIVO | 3 | VALID |  | 0.031951 | 3892 | 4086 | 4094/159131/4093 | 62.921 | 225.992 | 68.588 | 2097.9 | 2096.4 | 4086/4084 | 1 |
| P15-nya_03-N-LIO-r3 | NTU | nya_03 | N-LIO | 3 | VALID |  | 0.028883 | 3896 | 4092 | 4094/159131/0 | 38.822 | 156.621 | 42.944 | 622.8 | 622.8 | 0/0 | 0 |
| P15-nya_03-P-LIO-r3 | NTU | nya_03 | P-LIO | 3 | VALID |  | 0.025757 | 3892 | 4088 | 4094/159131/0 | 27.955 | 105.896 | 32.865 | 269.6 | 269.6 | 0/0 | 0 |
| P15-nya_03-N-LIVO-r3 | NTU | nya_03 | N-LIVO | 3 | VALID |  | 0.033347 | 3895 | 4090 | 4094/159131/4093 | 76.407 | 282.764 | 83.336 | 2315.8 | 2315.8 | 4091/4089 | 0 |
| P15-sbs_01-N-LIO-r3 | NTU | sbs_01 | N-LIO | 3 | VALID |  | 0.027766 | 2810 | 3540 | 3542/137552/0 | 38.610 | 150.724 | 47.094 | 1823.6 | 1823.2 | 0/0 | 0 |
| P15-sbs_01-P-LIO-r3 | NTU | sbs_01 | P-LIO | 3 | VALID |  | 0.025729 | 2810 | 3536 | 3542/137552/0 | 24.892 | 94.303 | 28.861 | 277.7 | 277.0 | 0/0 | 0 |
| P15-sbs_01-N-LIVO-r3 | NTU | sbs_01 | N-LIVO | 3 | VALID |  | 0.028936 | 2811 | 3538 | 3542/137552/3540 | 63.645 | 233.366 | 69.087 | 3374.4 | 3374.4 | 3538/3537 | 0 |
| P15-sbs_01-P-LIVO-r3 | NTU | sbs_01 | P-LIVO | 3 | VALID |  | 0.029509 | 2811 | 3534 | 3542/137552/3540 | 53.956 | 186.562 | 58.389 | 1863.3 | 1863.3 | 3534/3532 | 0 |
| P15-sbs_02-P-LIO-r3 | NTU | sbs_02 | P-LIO | 3 | VALID |  | 0.024137 | 3053 | 3725 | 3732/145288/0 | 25.855 | 97.767 | 30.368 | 281.1 | 279.9 | 0/0 | 0 |
| P15-sbs_02-N-LIVO-r3 | NTU | sbs_02 | N-LIVO | 3 | VALID |  | 0.028719 | 3054 | 3728 | 3732/145288/3731 | 73.303 | 262.948 | 78.995 | 3469.4 | 3469.4 | 3728/3727 | 0 |
| P15-sbs_02-P-LIVO-r3 | NTU | sbs_02 | P-LIVO | 3 | VALID |  | 0.028538 | 3054 | 3724 | 3732/145288/3731 | 58.481 | 204.420 | 63.082 | 1974.5 | 1974.5 | 3724/3722 | 0 |
| P15-sbs_02-N-LIO-r3 | NTU | sbs_02 | N-LIO | 3 | VALID |  | 0.026968 | 3053 | 3729 | 3732/145288/0 | 38.306 | 153.859 | 42.474 | 1784.0 | 1782.3 | 0/0 | 0 |

### Accuracy ranking (translation ATE RMSE)

Sequence means over three repetitions of the authoritative translation ATE RMSE; incomplete P-LIO groups are excluded from valid ranking.

#### NTU

| Sequence | N-LIO ATE RMSE | P-LIO ATE RMSE | N-LIVO ATE RMSE | P-LIVO ATE RMSE | LIO winner | LIVO winner |
|---|---:|---:|---:|---:|---|---|
| eee_01 | 0.030760 | 0.049511 | 0.030000 | 0.052103 | NATIVE | NATIVE |
| eee_02 | 0.021901 | 0.017281 | 0.035967 | 0.027541 | PROB | PROB |
| eee_03 | 0.031572 | 0.029984 | 0.030214 | 0.029665 | PROB | PROB |
| nya_01 | 0.027764 | 0.023256 | 0.037922 | 0.038840 | PROB | NATIVE |
| nya_02 | 0.029234 | 0.029557 | 0.036970 | 0.046926 | NATIVE | NATIVE |
| nya_03 | 0.028883 | 0.025757 | 0.033337 | 0.031924 | PROB | PROB |
| sbs_01 | 0.027766 | 0.025729 | 0.028969 | 0.029509 | PROB | NATIVE |
| sbs_02 | 0.026968 | 0.024137 | 0.028706 | 0.028534 | PROB | PROB |
| sbs_03 | 0.033383 | 0.025554 | 0.031164 | 0.028208 | PROB | PROB |

Wins: LIO Native=2 Prob=7 tie=0; LIVO Native=4 Prob=5 tie=0.

#### Oxford

| Sequence | N-LIO ATE RMSE | P-LIO ATE RMSE | N-LIVO ATE RMSE | P-LIVO ATE RMSE | LIO winner | LIVO winner |
|---|---:|---:|---:|---:|---|---|
| Church_05 | 0.226500 | 0.244000 | 0.211100 | 0.228733 | NATIVE | NATIVE |
| College_03 | 0.054000 | 0.074800 | 0.074600 | 2.108167 | NATIVE | NATIVE |
| Palace_01 | 0.147600 | — | 0.129367 | 1.845100 | N/A | NATIVE |
| Quarter_01 | 0.062800 | — | 0.072167 | 5.935600 | N/A | NATIVE |

Wins: LIO Native=2 Prob=0 tie=0; LIVO Native=4 Prob=0 tie=0.

### Runtime/CPU/memory ratios

Prob/Native sequence-level median ratio; median and IQR across sequences; >1 means Prob higher.

| Dataset | Mode | Est wall median | IQR | Est CPU median | IQR | Peak USS median | IQR | Final USS median | IQR |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| NTU | LIO | 0.720 | 0.068 | 0.676 | 0.090 | 0.170 | 0.274 | 0.169 | 0.274 |
| NTU | LIVO | 0.823 | 0.066 | 0.799 | 0.059 | 0.568 | 0.338 | 0.568 | 0.337 |
| OXFORD | LIO | 0.211 | 0.140 | 0.235 | 0.163 | 1.388 | 1.730 | 1.391 | 1.738 |
| OXFORD | LIVO | 0.192 | 0.038 | 0.163 | 0.044 | 0.880 | 0.038 | 0.880 | 0.038 |

### Native LIVO repeat stability

| Dataset | Sequence | Stability | Top counters exact | Timestamps exact | Rows |
|---|---|---|---|---|---|
| NTU | eee_01 | PASS | yes | yes | 3983,3983,3983 |
| NTU | eee_02 | PASS | yes | yes | 3206,3206,3206 |
| NTU | eee_03 | PASS | yes | yes | 1810,1810,1810 |
| NTU | nya_01 | PASS | yes | yes | 3945,3945,3945 |
| NTU | nya_02 | PASS | yes | yes | 4283,4283,4283 |
| NTU | nya_03 | PASS | yes | yes | 4090,4090,4090 |
| NTU | sbs_01 | PASS | yes | yes | 3538,3538,3538 |
| NTU | sbs_02 | PASS | yes | yes | 3728,3728,3728 |
| NTU | sbs_03 | PASS | yes | yes | 3889,3889,3889 |
| OXFORD | Church_05 | PASS | yes | yes | 3959,3959,3959 |
| OXFORD | College_03 | PASS | yes | yes | 5673,5673,5673 |
| OXFORD | Palace_01 | PASS | yes | yes | 8030,8030,8030 |
| OXFORD | Quarter_01 | PASS | yes | yes | 5736,5736,5736 |

### Explicit runtime, CPU, and memory result channels

Per-sequence/variant medians over the three formal repetitions are below. `est wall/event` and `est CPU/event` use the frozen `estimator_compute_count` emitted at the estimator timing seam; in LIVO this is the scheduler-package count, while trajectory rows and LiDAR epochs remain separately recorded above. CPU utilization is the peak monitor `raw_cpu_pct`; normalized utilization divides it by the fixed four-core budget. `OFFLINE_SYSTEM_PROCESS_MEMORY` is in MiB and includes the offline reader/decoder/adapter plus estimator. `RTF = offline wall / bag duration`.

| Dataset | Sequence | Variant | Adjudication | Est wall/event s | Est CPU/event s | Est CPU/bag s | Offline wall s | RTF | Peak raw CPU % | Peak normalized 4-core % | Peak RSS | Final RSS | Peak PSS | Final PSS | Peak USS | Final USS | USS growth |
|---|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| NTU | eee_01 | N-LIO | VALID | 0.012 | 0.046 | 0.495 | 51.533 | 0.138 | 394.855 | 98.714 | 2587.051 | 2585.453 | 2566.581 | 2564.979 | 2563.773 | 2562.242 | 2468.621 |
| NTU | eee_01 | P-LIO | VALID | 0.009 | 0.035 | 0.375 | 41.466 | 0.111 | 352.015 | 88.004 | 339.773 | 337.586 | 319.610 | 317.263 | 316.703 | 314.426 | 223.934 |
| NTU | eee_01 | N-LIVO | VALID | 0.010 | 0.036 | 0.760 | 86.995 | 0.233 | 396.753 | 99.188 | 4427.027 | 4427.027 | 4406.759 | 4406.759 | 4404.777 | 4404.777 | 4306.625 |
| NTU | eee_01 | P-LIVO | VALID | 0.009 | 0.030 | 0.650 | 78.236 | 0.210 | 368.397 | 92.099 | 2107.324 | 2105.652 | 2086.850 | 2085.213 | 2084.074 | 2082.637 | 1988.355 |
| NTU | eee_02 | P-LIO | VALID | 0.009 | 0.034 | 0.289 | 32.247 | 0.086 | 354.115 | 88.529 | 294.586 | 292.613 | 275.263 | 274.587 | 273.148 | 272.543 | 178.883 |
| NTU | eee_02 | N-LIVO | VALID | 0.010 | 0.037 | 0.634 | 71.176 | 0.191 | 395.485 | 98.871 | 3110.363 | 3110.363 | 3090.550 | 3090.550 | 3087.918 | 3087.918 | 2993.910 |
| NTU | eee_02 | P-LIVO | VALID | 0.009 | 0.030 | 0.521 | 61.174 | 0.164 | 376.828 | 94.207 | 1711.258 | 1711.258 | 1690.841 | 1690.841 | 1688.801 | 1688.801 | 1591.223 |
| NTU | eee_02 | N-LIO | VALID | 0.011 | 0.043 | 0.367 | 38.073 | 0.102 | 393.050 | 98.263 | 1715.766 | 1714.086 | 1696.568 | 1694.842 | 1693.574 | 1691.918 | 1597.773 |
| NTU | eee_03 | N-LIVO | VALID | 0.010 | 0.036 | 0.347 | 39.213 | 0.105 | 395.853 | 98.963 | 2221.691 | 2221.691 | 2203.935 | 2203.935 | 2202.484 | 2202.484 | 2105.309 |
| NTU | eee_03 | P-LIVO | VALID | 0.009 | 0.031 | 0.305 | 35.702 | 0.096 | 379.953 | 94.988 | 1065.883 | 1065.883 | 1046.298 | 1046.298 | 1043.312 | 1043.312 | 950.168 |
| NTU | eee_03 | N-LIO | VALID | 0.011 | 0.044 | 0.214 | 23.004 | 0.062 | 390.188 | 97.547 | 1331.227 | 1331.227 | 1311.626 | 1311.626 | 1308.672 | 1308.672 | 1214.398 |
| NTU | eee_03 | P-LIO | VALID | 0.009 | 0.034 | 0.165 | 18.547 | 0.050 | 351.522 | 87.880 | 249.234 | 249.234 | 229.493 | 229.493 | 226.527 | 226.527 | 135.617 |
| NTU | nya_01 | P-LIVO | VALID | 0.008 | 0.027 | 0.568 | 64.556 | 0.173 | 387.504 | 96.876 | 2043.824 | 2042.051 | 2023.454 | 2022.139 | 2020.766 | 2019.238 | 1924.355 |
| NTU | nya_01 | N-LIO | VALID | 0.009 | 0.037 | 0.394 | 40.617 | 0.109 | 392.916 | 98.229 | 577.152 | 577.152 | 557.533 | 557.533 | 554.500 | 554.500 | 459.730 |
| NTU | nya_01 | P-LIO | VALID | 0.007 | 0.025 | 0.263 | 31.160 | 0.083 | 349.656 | 87.414 | 266.238 | 266.238 | 246.493 | 246.493 | 243.508 | 243.508 | 152.621 |
| NTU | nya_01 | N-LIVO | VALID | 0.010 | 0.036 | 0.763 | 88.212 | 0.236 | 397.713 | 99.428 | 2307.461 | 2307.461 | 2286.851 | 2286.851 | 2284.766 | 2284.766 | 2187.914 |
| NTU | nya_02 | N-LIO | VALID | 0.010 | 0.039 | 0.447 | 45.729 | 0.123 | 394.351 | 98.588 | 562.164 | 561.711 | 542.760 | 542.322 | 540.730 | 540.363 | 444.469 |
| NTU | nya_02 | P-LIO | VALID | 0.007 | 0.026 | 0.300 | 35.084 | 0.094 | 350.820 | 87.705 | 279.047 | 279.047 | 259.196 | 258.452 | 256.023 | 255.773 | 164.121 |
| NTU | nya_02 | N-LIVO | VALID | 0.010 | 0.036 | 0.819 | 90.348 | 0.242 | 396.205 | 99.051 | 2432.148 | 2432.148 | 2411.651 | 2411.651 | 2408.965 | 2408.965 | 2314.254 |
| NTU | nya_02 | P-LIVO | VALID | 0.008 | 0.027 | 0.624 | 71.102 | 0.191 | 383.902 | 95.975 | 2186.754 | 2186.754 | 2167.861 | 2167.861 | 2164.809 | 2164.809 | 2069.875 |
| NTU | nya_03 | P-LIO | VALID | 0.007 | 0.026 | 0.284 | 32.865 | 0.088 | 346.940 | 86.735 | 291.797 | 291.797 | 272.670 | 272.508 | 269.590 | 269.590 | 178.137 |
| NTU | nya_03 | N-LIVO | VALID | 0.009 | 0.035 | 0.758 | 83.336 | 0.223 | 396.405 | 99.101 | 2337.836 | 2337.836 | 2318.841 | 2318.841 | 2315.766 | 2315.766 | 2221.488 |
| NTU | nya_03 | P-LIVO | VALID | 0.008 | 0.028 | 0.606 | 68.588 | 0.184 | 385.905 | 96.476 | 2124.512 | 2124.512 | 2105.050 | 2105.050 | 2102.121 | 2102.121 | 2005.227 |
| NTU | nya_03 | N-LIO | VALID | 0.009 | 0.038 | 0.420 | 42.944 | 0.115 | 395.428 | 98.857 | 647.684 | 647.684 | 627.424 | 627.424 | 624.605 | 624.605 | 529.438 |
| NTU | sbs_01 | N-LIVO | VALID | 0.009 | 0.033 | 0.625 | 69.087 | 0.185 | 395.998 | 98.999 | 3397.160 | 3397.160 | 3378.845 | 3378.845 | 3377.340 | 3377.340 | 3280.422 |
| NTU | sbs_01 | P-LIVO | VALID | 0.008 | 0.026 | 0.500 | 58.389 | 0.156 | 379.148 | 94.787 | 1882.102 | 1882.102 | 1864.741 | 1864.741 | 1863.281 | 1863.281 | 1765.855 |
| NTU | sbs_01 | N-LIO | VALID | 0.011 | 0.043 | 0.404 | 42.029 | 0.113 | 395.483 | 98.871 | 1847.930 | 1847.930 | 1827.906 | 1827.906 | 1825.121 | 1825.121 | 1730.121 |
| NTU | sbs_01 | P-LIO | VALID | 0.007 | 0.027 | 0.253 | 28.861 | 0.077 | 350.819 | 87.705 | 303.102 | 301.465 | 282.478 | 280.868 | 279.570 | 278.031 | 186.309 |
| NTU | sbs_02 | P-LIVO | VALID | 0.008 | 0.027 | 0.548 | 63.082 | 0.169 | 376.190 | 94.047 | 1997.039 | 1997.039 | 1977.631 | 1977.631 | 1974.504 | 1974.504 | 1880.406 |
| NTU | sbs_02 | N-LIO | VALID | 0.010 | 0.041 | 0.412 | 42.474 | 0.114 | 397.463 | 99.366 | 1806.520 | 1804.973 | 1787.185 | 1785.793 | 1784.445 | 1783.176 | 1689.121 |
| NTU | sbs_02 | P-LIO | VALID | 0.007 | 0.026 | 0.262 | 30.368 | 0.081 | 349.127 | 87.282 | 303.418 | 303.203 | 284.233 | 283.153 | 281.055 | 280.195 | 189.469 |
| NTU | sbs_02 | N-LIVO | VALID | 0.010 | 0.035 | 0.705 | 78.995 | 0.212 | 396.648 | 99.162 | 3496.637 | 3496.637 | 3477.100 | 3477.100 | 3474.168 | 3474.168 | 3378.219 |
| NTU | sbs_03 | N-LIO | VALID | 0.008 | 0.032 | 0.330 | 33.945 | 0.091 | 396.405 | 99.101 | 1806.438 | 1806.438 | 1788.225 | 1788.225 | 1785.324 | 1785.324 | 1689.270 |
| NTU | sbs_03 | P-LIO | VALID | 0.006 | 0.022 | 0.225 | 25.893 | 0.069 | 348.784 | 87.196 | 326.480 | 325.332 | 306.848 | 305.401 | 303.840 | 302.438 | 211.195 |
| NTU | sbs_03 | N-LIVO | VALID | 0.009 | 0.033 | 0.693 | 78.285 | 0.210 | 396.913 | 99.228 | 3531.840 | 3531.840 | 3511.896 | 3511.896 | 3508.934 | 3508.934 | 3413.840 |
| NTU | sbs_03 | P-LIVO | VALID | 0.006 | 0.022 | 0.464 | 54.295 | 0.145 | 376.571 | 94.143 | 2076.203 | 2076.203 | 2056.937 | 2056.937 | 2053.980 | 2053.980 | 1959.438 |
| OXFORD | Church_05 | P-LIO | VALID | 0.012 | 0.042 | 0.904 | 113.161 | 0.303 | 339.976 | 84.994 | 770.168 | 766.109 | 750.744 | 746.701 | 747.773 | 743.801 | 651.000 |
| OXFORD | Church_05 | N-LIVO | VALID | 0.046 | 0.120 | 2.541 | 411.478 | 1.103 | 267.607 | 66.902 | 8403.789 | 8399.805 | 8384.899 | 8380.880 | 8381.742 | 8377.793 | 8286.688 |
| OXFORD | Church_05 | P-LIVO | VALID | 0.011 | 0.030 | 0.637 | 136.370 | 0.365 | 276.080 | 69.020 | 8107.484 | 8107.484 | 8088.168 | 8088.168 | 8085.188 | 8085.188 | 7994.121 |
| OXFORD | Church_05 | N-LIO | VALID | 0.044 | 0.138 | 2.967 | 370.837 | 0.994 | 323.593 | 80.898 | 1344.707 | 1340.883 | 1325.056 | 1321.052 | 1322.125 | 1318.191 | 1225.598 |
| OXFORD | College_03 | N-LIVO | VALID | 0.015 | 0.039 | 1.184 | 223.180 | 0.598 | 271.387 | 67.847 | 11511.613 | 11511.613 | 11491.714 | 11491.714 | 11488.832 | 11488.832 | 11398.977 |
| OXFORD | College_03 | P-LIVO | ALGORITHM_RED | 0.003 | 0.007 | 0.207 | 95.447 | 0.256 | 174.146 | 43.536 | 10229.160 | 10229.160 | 10209.380 | 10209.380 | 10206.613 | 10206.613 | 10113.680 |
| OXFORD | College_03 | N-LIO | VALID | 0.038 | 0.113 | 0.866 | 117.710 | 0.315 | 302.595 | 75.649 | 805.719 | 805.719 | 787.371 | 787.371 | 785.816 | 785.816 | 692.914 |
| OXFORD | College_03 | P-LIO | VALID | 0.013 | 0.043 | 0.331 | 45.417 | 0.122 | 315.096 | 78.774 | 576.707 | 576.609 | 557.965 | 557.965 | 554.996 | 554.996 | 463.453 |
| OXFORD | Palace_01 | P-LIVO | ALGORITHM_RED | 0.003 | 0.006 | 0.276 | 132.910 | 0.356 | 171.685 | 42.921 | 13818.082 | 13818.082 | 13799.910 | 13799.910 | 13796.770 | 13796.770 | 13705.285 |
| OXFORD | Palace_01 | N-LIO | VALID | 0.040 | 0.121 | 1.311 | 173.175 | 0.464 | 328.081 | 82.020 | 1303.945 | 1301.066 | 1283.277 | 1280.414 | 1280.453 | 1277.660 | 1186.906 |
| OXFORD | Palace_01 | P-LIO | INCOMPLETE | 0.006 | 0.020 | 0.216 | 35.959 | 0.096 | 283.631 | 70.908 | 4372.676 | 4372.332 | 4353.320 | 4353.290 | 4350.449 | 4350.223 | 4258.809 |
| OXFORD | Palace_01 | N-LIVO | VALID | 0.016 | 0.043 | 1.836 | 347.012 | 0.930 | 272.361 | 68.090 | 15986.965 | 15986.965 | 15968.400 | 15968.400 | 15965.297 | 15965.297 | 15873.785 |
| OXFORD | Quarter_01 | N-LIO | VALID | 0.056 | 0.170 | 1.318 | 172.321 | 0.462 | 306.714 | 76.678 | 1652.113 | 1646.441 | 1631.789 | 1625.984 | 1628.789 | 1623.039 | 1532.062 |
| OXFORD | Quarter_01 | P-LIO | INCOMPLETE | 0.008 | 0.026 | 0.199 | 32.567 | 0.087 | 299.657 | 74.914 | 3393.027 | 3393.027 | 3372.440 | 3372.440 | 3369.629 | 3369.629 | 3279.902 |
| OXFORD | Quarter_01 | N-LIVO | VALID | 0.019 | 0.051 | 1.581 | 284.482 | 0.762 | 272.795 | 68.199 | 11792.836 | 11788.742 | 11772.740 | 11768.549 | 11769.953 | 11765.832 | 11675.969 |
| OXFORD | Quarter_01 | P-LIVO | ALGORITHM_RED | 0.003 | 0.008 | 0.236 | 108.671 | 0.291 | 169.600 | 42.400 | 10275.098 | 10275.098 | 10254.774 | 10254.774 | 10252.117 | 10252.117 | 10161.379 |

### Component timing and bag I/O/decode channels

All 156 formal runs emitted `trajectory.tum.timing.yaml` and `offline_source.yaml`. The following medians expose the secondary component fields and the reader/decode seam; they are not summed into the primary cross-algorithm metric. `visual_s` is zero for LIO variants, and Oxford `decode_s` is the CompressedImage adapter channel.

| Dataset | Sequence | Variant | Adjudication | IMU s | LiDAR association/update s | Map query s | Map update s | Visual s | Reader wall s | Image decode s | I/O/decode residual s |
|---|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| NTU | eee_01 | N-LIO | VALID | 2.992 | 44.185 | 33.150 | 7.668 | 0.000 | 51.340 | 0.000 | 1.757 |
| NTU | eee_01 | P-LIO | VALID | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 | 41.274 | 0.000 | 1.841 |
| NTU | eee_01 | N-LIVO | VALID | 3.025 | 44.615 | 33.289 | 7.938 | 31.582 | 86.781 | 0.000 | 2.387 |
| NTU | eee_01 | P-LIVO | VALID | 0.000 | 0.000 | 0.000 | 0.000 | 25.083 | 77.991 | 0.000 | 2.256 |
| NTU | eee_02 | P-LIO | VALID | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 | 32.075 | 0.000 | 1.532 |
| NTU | eee_02 | N-LIVO | VALID | 2.602 | 34.912 | 26.444 | 5.580 | 27.370 | 70.997 | 0.000 | 1.982 |
| NTU | eee_02 | P-LIVO | VALID | 0.000 | 0.000 | 0.000 | 0.000 | 23.125 | 60.996 | 0.000 | 1.919 |
| NTU | eee_02 | N-LIO | VALID | 2.382 | 32.287 | 24.356 | 5.222 | 0.000 | 37.900 | 0.000 | 1.428 |
| NTU | eee_03 | N-LIVO | VALID | 1.467 | 19.413 | 14.279 | 3.560 | 14.732 | 39.075 | 0.000 | 1.144 |
| NTU | eee_03 | P-LIVO | VALID | 0.000 | 0.000 | 0.000 | 0.000 | 12.673 | 35.564 | 0.000 | 1.103 |
| NTU | eee_03 | N-LIO | VALID | 1.451 | 19.209 | 14.105 | 3.506 | 0.000 | 22.867 | 0.000 | 0.899 |
| NTU | eee_03 | P-LIO | VALID | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 | 18.405 | 0.000 | 0.897 |
| NTU | nya_01 | P-LIVO | VALID | 0.000 | 0.000 | 0.000 | 0.000 | 29.410 | 64.269 | 0.000 | 2.368 |
| NTU | nya_01 | N-LIO | VALID | 3.009 | 33.220 | 26.844 | 2.995 | 0.000 | 40.309 | 0.000 | 1.765 |
| NTU | nya_01 | P-LIO | VALID | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 | 30.892 | 0.000 | 1.983 |
| NTU | nya_01 | N-LIVO | VALID | 3.366 | 38.217 | 31.082 | 3.452 | 35.023 | 87.853 | 0.000 | 3.079 |
| NTU | nya_02 | N-LIO | VALID | 3.478 | 37.736 | 30.538 | 3.308 | 0.000 | 45.400 | 0.000 | 1.880 |
| NTU | nya_02 | P-LIO | VALID | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 | 34.740 | 0.000 | 2.129 |
| NTU | nya_02 | N-LIVO | VALID | 3.724 | 42.533 | 34.752 | 3.718 | 34.048 | 89.999 | 0.000 | 2.727 |
| NTU | nya_02 | P-LIVO | VALID | 0.000 | 0.000 | 0.000 | 0.000 | 28.487 | 70.757 | 0.000 | 2.369 |
| NTU | nya_03 | P-LIO | VALID | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 | 32.575 | 0.000 | 2.112 |
| NTU | nya_03 | N-LIVO | VALID | 3.245 | 36.761 | 29.740 | 3.398 | 34.187 | 83.046 | 0.000 | 2.608 |
| NTU | nya_03 | P-LIVO | VALID | 0.000 | 0.000 | 0.000 | 0.000 | 29.261 | 68.269 | 0.000 | 2.501 |
| NTU | nya_03 | N-LIO | VALID | 3.288 | 35.321 | 28.295 | 3.322 | 0.000 | 42.648 | 0.000 | 1.851 |
| NTU | sbs_01 | N-LIVO | VALID | 2.689 | 33.404 | 25.531 | 5.233 | 25.798 | 68.814 | 0.000 | 2.248 |
| NTU | sbs_01 | P-LIVO | VALID | 0.000 | 0.000 | 0.000 | 0.000 | 21.480 | 58.137 | 0.000 | 2.127 |
| NTU | sbs_01 | N-LIO | VALID | 2.952 | 34.677 | 26.123 | 5.670 | 0.000 | 41.744 | 0.000 | 1.736 |
| NTU | sbs_01 | P-LIO | VALID | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 | 28.603 | 0.000 | 1.701 |
| NTU | sbs_02 | P-LIVO | VALID | 0.000 | 0.000 | 0.000 | 0.000 | 25.735 | 62.815 | 0.000 | 2.428 |
| NTU | sbs_02 | N-LIO | VALID | 2.874 | 35.201 | 26.964 | 5.322 | 0.000 | 42.207 | 0.000 | 1.749 |
| NTU | sbs_02 | P-LIO | VALID | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 | 30.092 | 0.000 | 1.906 |
| NTU | sbs_02 | N-LIVO | VALID | 2.862 | 36.419 | 28.134 | 5.424 | 32.214 | 78.716 | 0.000 | 2.404 |
| NTU | sbs_03 | N-LIO | VALID | 2.343 | 27.942 | 21.088 | 4.439 | 0.000 | 33.713 | 0.000 | 1.780 |
| NTU | sbs_03 | P-LIO | VALID | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 | 25.659 | 0.000 | 1.609 |
| NTU | sbs_03 | N-LIVO | VALID | 3.011 | 37.261 | 28.780 | 5.427 | 29.492 | 77.995 | 0.000 | 2.495 |
| NTU | sbs_03 | P-LIVO | VALID | 0.000 | 0.000 | 0.000 | 0.000 | 21.131 | 54.045 | 0.000 | 2.088 |
| OXFORD | Church_05 | P-LIO | VALID | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 | 113.012 | 0.000 | 10.681 |
| OXFORD | Church_05 | N-LIVO | VALID | 18.623 | 271.939 | 214.486 | 21.602 | 52.475 | 411.378 | 34.905 | 44.610 |
| OXFORD | Church_05 | P-LIVO | VALID | 0.000 | 0.000 | 0.000 | 0.000 | 29.294 | 136.282 | 32.332 | 41.468 |
| OXFORD | Church_05 | N-LIO | VALID | 20.246 | 335.191 | 263.139 | 28.239 | 0.000 | 370.726 | 0.000 | 9.547 |
| OXFORD | College_03 | N-LIVO | VALID | 9.011 | 110.758 | 86.284 | 9.243 | 36.803 | 223.144 | 44.546 | 51.618 |
| OXFORD | College_03 | P-LIVO | ALGORITHM_RED | 0.000 | 0.000 | 0.000 | 0.000 | 17.990 | 95.411 | 45.719 | 51.951 |
| OXFORD | College_03 | N-LIO | VALID | 9.276 | 100.330 | 74.982 | 9.353 | 0.000 | 117.674 | 0.000 | 4.917 |
| OXFORD | College_03 | P-LIO | VALID | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 | 45.380 | 0.000 | 5.184 |
| OXFORD | Palace_01 | P-LIVO | ALGORITHM_RED | 0.000 | 0.000 | 0.000 | 0.000 | 24.227 | 132.857 | 66.293 | 74.293 |
| OXFORD | Palace_01 | N-LIO | VALID | 13.023 | 149.405 | 112.249 | 14.859 | 0.000 | 173.125 | 0.000 | 6.623 |
| OXFORD | Palace_01 | P-LIO | INCOMPLETE | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 | 35.903 | 0.000 | 6.171 |
| OXFORD | Palace_01 | N-LIVO | VALID | 13.548 | 171.015 | 134.108 | 14.925 | 54.684 | 346.938 | 73.778 | 83.612 |
| OXFORD | Quarter_01 | N-LIO | VALID | 10.676 | 151.845 | 113.777 | 16.984 | 0.000 | 172.272 | 0.000 | 5.784 |
| OXFORD | Quarter_01 | P-LIO | INCOMPLETE | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 | 32.523 | 0.000 | 5.424 |
| OXFORD | Quarter_01 | N-LIVO | VALID | 10.166 | 155.288 | 121.944 | 15.680 | 42.666 | 284.440 | 52.577 | 60.133 |
| OXFORD | Quarter_01 | P-LIVO | ALGORITHM_RED | 0.000 | 0.000 | 0.000 | 0.000 | 18.885 | 108.627 | 53.075 | 60.332 |

Channel completeness: `trajectory.tum.timing.yaml`, `offline_source.yaml`, `offline_system.yaml`, `memory.csv`, `processing_complete.sentinel`, and all evaluator/counter/init artifacts are present for 156/156 formal slots. Raw per-sample memory/CPU curves and full component fields remain in each run directory listed in the ledger.


### RED/failure cases

- `INCOMPLETE_TRAJECTORY`: P-LIO Palace_01, 1419 rows versus 4052 LiDAR; P-LIO Quarter_01, 1173 versus 2894. Wrapper/evaluator RC0 but trajectories are incomplete and excluded from valid ranking.
- `ALGORITHM_RED_RECORDED`: P-LIVO College_03 (2814 rejected/5680 backend attempts; 84–87 commits/5673 visual calls), Palace_01 (3980/8038; 121/8031), Quarter_01 (2843/5744; 74/5737). Complete artifacts retained.
- No GT absence, config mismatch, IMU-init mismatch, evaluator failure, execution failure, timeout, contamination, dataset-broken, or Native-LIVO stability failure.

### Architecture deviations and evidence paths

Native includes accepted offline, ABI-safe, and instrumentation adaptations and is not an unmodified upstream binary. Prob uses one FAST/LIVO2 scheduler/visual shell, one Prob backend/ProbESKF19/map/plane provider and FAST-native adapters. Offline changes only event source to in-process rosbag record-order dispatch; Oxford compressed cam0 is decoded preserving its timestamp. No pose-copy bridge, second ESKF, cloned map, or offline-specific plane authority exists.

Final ledger: `spec/prob_livo/PROMPT15_RUN_LEDGER.csv`; evidence: `spec/prob_livo/PROMPT15_EVIDENCE.md`; driver: `results/prob_livo/prompt15/formal/matrix_driver.log`; artifacts: `results/prob_livo/prompt15/formal/`, canary `results/prob_livo/prompt15/canary/`, preflight `results/prob_livo/prompt15/preflight/`.
