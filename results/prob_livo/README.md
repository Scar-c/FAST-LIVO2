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
It was not copied in Prompt 0. Dataset topics/config provenance must be
rechecked for the host integration before use.

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
