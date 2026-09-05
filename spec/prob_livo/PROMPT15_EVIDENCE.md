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

