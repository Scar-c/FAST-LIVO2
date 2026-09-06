# Prompt16 Oxford P-LIO Official-Config Corrective Evidence

## Final status

`PROMPT16 CHURCH PASSED / PALACE-QUARTER ROOT CAUSE IDENTIFIED`

Prompt16 is complete. The corrected Oxford P-LIO configuration was applied and the Church sanity gate passed. Palace and Quarter remain incomplete, but their first failure and recurrent producer have been isolated with bounded traces. No P-LIVO corrective work was started.

## Scope and owner correction

This round is Oxford P-LIO only: camera off, `fast_native` input semantics, `ProbLioBackend P0-P4`, and the audited FAST-LIVO2/LIVO2 effective configuration. Native, P-LIVO, estimator mathematics, map, association gates, scheduler behavior, visual stride, and parameter sweeps were not modified.

Owner correction recorded: the earlier statement interpreting 32 logical CPUs as 32 physical CPUs was wrong. Prompt16 diagnostic runs are authorized to use 32 logical CPUs (`0-31`) with TBB/OMP cap 32. This diagnostic runtime is not compared with Prompt15's 4-core formal results.

All Prompt16 compilation was finally completed with `catkin_make -C /home/lc/super_livo -DCMAKE_BUILD_TYPE=Release -j4`. Earlier `-j32` and `-j16` attempts exhausted the 31 GiB RAM + 2 GiB swap machine and caused compiler processes to be OOM-killed; no offline/test run was launched concurrently by the agent. Future compilation is restricted to `-j4`.

## Corrected configuration

Prompt15's Oxford base values and the Prompt16 official effective values were:

| Key | Prompt15 base | Prompt16 official effective | Evidence |
|---|---:|---:|---|
| `preprocess.filter_size_surf` | `0.1` | `0.5` | `config/prob_livo/OXFORD_Prompt16_official.yaml:4` |
| `lio.min_eigen_value` | `0.0025` | `0.01` | `config/prob_livo/OXFORD_Prompt16_official.yaml:6` |

The overlay is applied over `config/OXFORD_SPIRES.yaml` by the existing runner. SHA256:

| File | SHA256 |
|---|---|
| `config/OXFORD_SPIRES.yaml` | `87bd926073ed49c68ab330617e7ca52c0e0b0470ab2c63cdba89aac2312aca92` |
| `config/prob_livo/OXFORD_Prompt16_official.yaml` | `b5917b705e856bdfa1f949201b57daada9be3645ceb677c0d03f40481405ce61` |

The effective ROS parameter dumps for all three sequences report `filter_size_surf: 0.5` and `min_eigen_value: 0.01`.

## Church sanity gate

Run: `results/prob_livo/prompt16/P16-Church_05-P-LIO-official-32log`

| Check | Result |
|---|---|
| Dataset | `Church_05_LIVO.bag` |
| LiDAR / IMU | `8007 / 319608` |
| Backend attempted / success / rejected | `8007 / 8007 / 0` |
| Trajectory rows | `8001` |
| Queues at shutdown | pending `0`, discarded `0` |
| Source EOF / run RC | clean sentinel, `RC0` |
| Evaluator RC | `0` |
| Matched samples / duration | `7780 / 799.315 s` |
| Translation APE RMSE | `0.2440 m` |
| Prompt15 reference difference | `0.0000 m` (within `0.030 m`) |
| Trajectory SHA256 | `a85400ba437ed456dd78694828001e0047a03f77c814290558ddd5931a7d35ca` |

Church passed all required gate conditions, so Palace and Quarter were run.

## Palace and Quarter corrected-config results

Both runs used camera off, `fast_native`, logical CPU affinity `0-31`, TBB/OMP cap `32`, and the corrected overlay. Their trace3 runs completed with `RC0`, but the trajectories were incomplete and therefore their ATE values are diagnostic only.

| Sequence | LiDAR / IMU | Attempted / success / rejected | Rows | ATE RMSE | Final `last_error` |
|---|---:|---:|---:|---:|---|
| Palace_01 | `4052 / 161400` | `4052 / 1424 / 2628` | `1419` | `111871.7725 m` | `map-update covariance failed validation` |
| Quarter_01 | `2894 / 115459` | `2894 / 1178 / 1716` | `1173` | `22634.7459 m` | empty at final epoch |

The Palace and Quarter trajectory hashes are respectively:

- Palace: `e4e38635bf3ed83168ae6b14792bad9d5a049a94b313eb56a6b4b97b5daf2280`
- Quarter: `6a6c9a4cd703e5f56e90622d6b12e306d4f35c85c32ed1b8408f35873ac17397`

The large ATE values must not be interpreted as a completed-sequence benchmark: both outputs stopped short of the full input sequence.

## Bounded first-failure diagnosis

The authoritative traces are:

- `results/prob_livo/prompt16/P16-Palace_01-P-LIO-official-32log-trace3/prompt16_failure_trace.csv`
- `results/prob_livo/prompt16/P16-Quarter_01-P-LIO-official-32log-trace3/prompt16_failure_trace.csv`

The first failing row is `backend_epoch=6` (zero-based trace row index 5) for both sequences. The trace uses one-based `backend_epoch` values below.

### Palace_01

| Epoch | LIO epoch `[start,end]` | Point time min/max | IMU count/window end | Raw/pre | Map q/success | Plane / assoc / P2P | State/cov finite | Gate / error |
|---:|---|---|---:|---:|---:|---:|---|---|
| N-2 = 4 | `[1710406747.2761192, 1710406747.3763020]` | `[1710406747.2762105, 1710406747.3763020]` | `121 / 1710406747.3756752` | `32603 / 32603` | `0 / 0` | `0 / 0 / 0` | `1 / 1` | `0 / empty` |
| N-1 = 5 | `[1710406747.3763020, 1710406747.4762950]` | `[1710406747.3763959, 1710406747.4762950]` | `161 / 1710406747.4758048` | `32572 / 32572` | `0 / 0` | `0 / 0 / 0` | `1 / 1` | `0 / empty` |
| N = 6 | `[1710406747.4762950, 1710406747.5761681]` | `[1710406747.4763887, 1710406747.5761683]` | `201 / 1710406747.5759311` | `32449 / 32449` | `0 / 0` | `0 / 0 / 0` | `1 / 1` | `0 / scheduler point time is outside the LIO epoch` |
| N+1 = 7 | `[1710406747.4762950, 1710406747.6762431]` | `[1710406747.5762615, 1710406747.6762452]` | `241 / 1710406747.6760662` | `32428 / 32428` | `0 / 0` | `0 / 0 / 0` | `1 / 1` | `0 / same error` |

Palace has `2628/2628` rejected rows with `point_time_max > epoch_end`; the first excess is about `0.2 us`, while the adapter tolerance is `2e-8 s` (`20 ns`). Rejections recur through epoch 4051 in `1121` failure runs, with successful epochs interleaved; this is recurrent/persistent producer-level rejection, not an isolated event and not a permanent all-epoch lockout.

### Quarter_01

| Epoch | LIO epoch `[start,end]` | Point time min/max | IMU count/window end | Raw/pre | Map q/success | Plane / assoc / P2P | State/cov finite | Gate / error |
|---:|---|---|---:|---:|---:|---:|---|---|
| N-2 = 4 | `[1710338099.2518284, 1710338099.3517115]` | `[1710338099.2518897, 1710338099.3517115]` | `121 / 1710338099.3509977` | `46205 / 46205` | `0 / 0` | `0 / 0 / 0` | `1 / 1` | `0 / empty` |
| N-1 = 5 | `[1710338099.3517115, 1710338099.4518354]` | `[1710338099.3517730, 1710338099.4518354]` | `161 / 1710338099.4511135` | `46227 / 46227` | `0 / 0` | `0 / 0 / 0` | `1 / 1` | `0 / empty` |
| N = 6 | `[1710338099.4518354, 1710338099.5516584]` | `[1710338099.4518969, 1710338099.5516598]` | `201 / 1710338099.5512278` | `46379 / 46379` | `0 / 0` | `0 / 0 / 0` | `1 / 1` | `0 / scheduler point time is outside the LIO epoch` |
| N+1 = 7 | `[1710338099.4518354, 1710338099.6515715]` | `[1710338099.5517197, 1710338099.6515715]` | `241 / 1710338099.6513450` | `46559 / 46559` | `14079 / 13731` | `10537 / 14054 / 14054` | `1 / 1` | `1 / empty` |

Quarter has `1716/1716` rejected rows with `point_time_max > epoch_end`; the first excess is about `1.4 us`, again greater than `20 ns`. Rejections recur through epoch 2893 in `878` failure runs, with successful epochs interleaved. This is the same recurrent producer-level failure, not an isolated event. The successful N+1 row also proves that no permanent estimator, map, plane, or state lockout occurred at the first rejection.

## Root-cause adjudication

Classification: `LIDAR_TIMESTAMP_EPOCH_BOUNDARY_MISMATCH`.

The concrete producer chain is:

1. The LIVO2 LiDAR-only scheduler defines the LIO endpoint from the scan header plus the last point's curvature: `meas.lidar_frame_end_time = meas.lidar_frame_beg_time + curvature/1000`, and uses that value as `m.lio_time` (`src/LIVMapper.cpp:1454-1474`).
2. The Prob IMU adapter reconstructs every point query time as `point_time_origin + curvature/1000` and rejects when that query exceeds `epoch_end + 2e-8 s` (`src/prob_livo/prob_imu_adapter.cpp:294-301`; tolerance construction at `src/prob_livo/prob_lio_backend.cpp:32-45`).
3. At the first rejected epochs, raw and preprocessed points are present, IMU windows are present, state and covariance are finite, and no map query, plane candidate, association, or P2P stage is reached. Therefore the first failure is before map/plane/association/ESKF update/output production.

The Palace final `last_error` text (`map-update covariance failed validation`) is not the first failure cause: it is stale final-state diagnostic text, while all bounded first failures carry the timestamp-boundary error and have finite covariance. The evidence does not support `IMU_PROPAGATION_FAILURE`, `MAP_QUERY_FAILURE`, `PLANE_MODEL_FAILURE`, `STATE_UPDATE_FAILURE`, `COVARIANCE_INVALID`, or `OUTPUT_GATING_FAILURE` as the first producer.

No corrective code was applied to the scheduler or estimator in Prompt16. Resolving this boundary contract is owner work for a later prompt.

## Final restoration proof

The formal Prompt15 runner defaults remain:

- affinity `0,2,4,6`
- `PROB_LIVO_WORKERS=4`
- TBB cap `4`
- OMP cap `4`
- no persistent `PROB_LIVO_*`, `OMP_*`, or `TBB_*` shell override

The 32-logical-CPU affinity and worker values were command-scoped Prompt16 diagnostic settings only. No `prob_livo_offline`, `roscore`, `catkin_make`, `make`, or `cc1plus` process remained after the runs. Prompt16 stops here; it does not enter P-LIVO corrective work.
