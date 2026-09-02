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

`runs/eee01_camera_off_p0_p4_retry/`

```text
bag SHA256: 7ea43946cffdd49c88d993ad3f192a4e90a8f6826eddc2ef1a9d4f5343ca6c17
host HEAD at run: ce805bb8e02eb234d004a6e2d1c54ed35bfc1ba5
camera: OFF
backend: ProbLioBackend P0-P4
play/node/GT/evaluator/run RC: 0/0/0/0/0
trajectory rows: 3595
official NTU ATE RMSE: 0.05290159739482509 m
official matched: 3016
```

The legacy P4 trajectory used for the required primary raw comparison is
`/home/lc/prob_lio/src/Super-LIO/results/prob_lio/p11_smoke_eee_p4_lc/trajectory.tum`.
The comparison matched 3594 rows and produced translation RMSE
`0.1272755745726123 m`, rotation RMSE `0.01831955642234057 rad`, with
`alignment: NONE_RAW_WORLD_FRAMES` and the single classification
`I3_TRAJECTORY_CLOSE_NONIDENTICAL`. The official evaluator's internal
`SE3_UMEYAMA_NO_SCALE` is not used for that raw comparison.

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
