# Prob-LIVO Results and Dataset Inventory

Prompt 0 contains no experiment results and no bag run. Runtime outputs belong
under ignored `runs/`, `logs/`, or `tmp/` directories.

## Dataset root

`/home/lc/super_livo/bag`

## NTU VIRAL

Every NTU sequence has camera calibration (`camera_left.yaml`,
`camera_right.yaml`), IMU calibration (`imu_v100.yaml`), LiDAR calibration
(`lidar_horz.yaml`, `lidar_vert.yaml`), Leica prism metadata
(`leica_prism.yaml`), and UWB metadata (`uwb_nodes.yaml`). Camera calibration
is therefore present in the inventory; camera-topic presence remains pending
an explicit rosbag topic audit. The files were only inventoried here.

| Sequence | Bag | Size (bytes) | Evaluator/config availability | Intended stage |
|---|---|---:|---|---|
| `eee_01` | `NTU/eee_01/eee_01.bag` | 9,294,291,860 | legacy NTU official evaluator; host config to be adapted later | I3 camera-OFF, then I6 |
| `eee_02` | `NTU/eee_02/eee_02.bag` | 7,482,627,250 | legacy NTU official evaluator; host config to be adapted later | I8 |
| `eee_03` | `NTU/eee_03/eee_03.bag` | 4,261,528,188 | legacy NTU official evaluator; host config to be adapted later | I8 |
| `nya_01` | `NTU/nya_01/nya_01.bag` | 9,277,407,427 | legacy NTU official evaluator; host config to be adapted later | I8 |
| `nya_02` | `NTU/nya_02/nya_02.bag` | 10,072,427,976 | legacy NTU official evaluator; host config to be adapted later | I8 |
| `nya_03` | `NTU/nya_03/nya_03.bag` | 9,619,323,787 | legacy NTU official evaluator; host config to be adapted later | I8 |
| `sbs_01` | `NTU/sbs_01/sbs_01.bag` | 8,322,101,107 | legacy NTU official evaluator; host config to be adapted later | I8 |
| `sbs_02` | `NTU/sbs_02/sbs_02.bag` | 8,768,337,329 | legacy NTU official evaluator; host config to be adapted later | I8 |
| `sbs_03` | `NTU/sbs_03/sbs_03.bag` | 9,146,010,044 | legacy NTU official evaluator; host config to be adapted later | I8 |

The legacy evaluator contract is available at
`/home/lc/prob_lio/src/Super-LIO/eval/prob_lio/eval_ntu_viral_official.py`.
Prompt 3 copied the evaluator and GT conversion wrapper on demand into the
host at `eval/prob_livo/`.

## Prompt 3 canonical EEE01 camera-OFF run

The whole-bag run used one FAST ROS node and the camera-OFF
`ProbLioBackend` P0–P4 path. Only `/imu/imu` and
`/os1_cloud_node1/points` were replayed. The ignored, unique evidence bundle
is:

`runs/eee01_camera_off_p0_p4_correction/`

```text
bag SHA256: 7ea43946cffdd49c88d993ad3f192a4e90a8f6826eddc2ef1a9d4f5343ca6c17
host HEAD at run: c36a96b9a3d88c9f6336edc98c2e52c86642fae2
config SHA256: c8f94f130e599b928c3f02c3f3d3b2009ae01df76aec32f6ac96b6a987311ef3
effective config SHA256: f0ad429db8c0c2bde96099c7131814a6b0587bc1232edea4097ea4376536767a
camera: OFF
backend: ProbLioBackend P0-P4
play/node/counter/GT/evaluator/run RC: 0/0/0/0/0/0
trajectory rows: 3595
trajectory timestamps: 1609059013.9799576 .. 1609059411.7837925
authority counters: successful=3602, IMU_INIT=3, MAP_INIT=4, RUN=3595,
map-init inserts=17017, map-update inserts=12485822,
undistorted=14702670, downsampled=12485822,
HKNN queries/returns=36666902/181435218,
QR attempted/valid=30034406/30034406, P4 weighted=37765549
official NTU ATE RMSE: 0.05290159739482509 m
official matched: 3016
```

The runtime authority sidecar is
`runs/eee01_camera_off_p0_p4_correction/trajectory.tum.counters.yaml`.
The evaluator is `eval/prob_livo/eval_ntu_viral_official.py` from the NTU
VIRAL dataset-author benchmark (`viral_eval` revision
`194dd4595b1fb5e8ae2a5a0c01255f816ab4082f`).

The legacy P4 trajectory used for the required primary raw comparison is
`/home/lc/prob_lio/src/Super-LIO/results/prob_lio/p11_smoke_eee_p4_lc/trajectory.tum`.
Its algorithm SHA is `621acbd8d9a67634d3782fe8ab56e8a49ec821a9`.
The comparison matched 3594 rows. Timestamp delta mean absolute/max was
`0.02349526841042104 / 0.06304597854614258 s`; translation
RMSE/median/max was `0.1272755745726123 / 0.10450333931631896 /
0.5874476253736715 m`; rotation RMSE/median/max was
`0.01831955642234057 / 0.0092050820666131 / 0.08735832181275227 rad`, with
`alignment: NONE_RAW_WORLD_FRAMES` and the single classification
`I3_TRAJECTORY_CLOSE_NONIDENTICAL`. The official evaluator's internal
`SE3_UMEYAMA_NO_SCALE` is not used for that raw comparison.

ATE comparison:

```text
old canonical Prob-LIO ATE: 0.08883155405698266 m
new FAST-host Prob-LIO ATE: 0.05290159739482509 m
absolute delta: -0.035929956662157571 m
percent delta: -40.447290429152716 %
old matched GT: 3329
new matched GT: 3016
```

The canonical command is `tools/prob_livo/run_eee01_camera_off.sh`; it
requires a clean worktree and refuses to overwrite an existing run directory.

## Oxford Spires

Calibration files are `OXFORD/Calibration/README.docx`,
`calibration_target.docx`, `cam-lidar-imu.yaml`, `cam0.yaml`, and `imu.yaml`.
The four sequences have camera data in the `_LIVO.bag` variant and TUM ground
truth in `gt-tum.txt`.

| Sequence | Bags | Sizes (bytes) | GT/config availability | Intended stage |
|---|---|---:|---|---|
| `Church_05` | `Church_05.bag`, `Church_05_LIVO.bag` | 6,291,326,026; 7,165,664,272 | local GT + calibration; legacy Oxford TUM evaluator/config path | I8 |
| `College_03` | `College_03.bag`, `College_03_LIVO.bag` | 4,808,014,426; 3,800,779,019 | local GT + calibration; legacy Oxford TUM evaluator/config path | I8 |
| `Palace_01` | `Palace_01.bag`, `Palace_01_LIVO.bag` | 6,325,760,912; 4,722,794,709 | local GT + calibration; legacy Oxford TUM evaluator/config path | I8 |
| `Quarter_01` | `Quarter_01.bag`, `Quarter_01_LIVO.bag` | 5,466,871,208; 3,941,295,957 | local GT + calibration; legacy Oxford TUM evaluator/config path | I8 |

The legacy Oxford evaluator/tooling is available under
`/home/lc/prob_lio/src/Super-LIO/eval/prob_lio/` and remains copy-on-demand.
No Oxford run was started.
