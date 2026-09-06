# Prompt19 — Canonical Native-Compatible Prob-LIVO Stride + Resource Comparison

## Final status

PROMPT19 CLOSED — CANONICAL STRIDE + RESOURCE COMPARISON COMPLETE

Prompt hash: b9254719102da7e4ce63ce7479152e9541686f0c1fca0119a8e98a7f4416f0eb.

The Oxford comparison is complete. P0/P0S are the project-owned Prob-LIVO
implementation with Native-compatible blind carry; N0/N1 are the accepted
unchanged Native LIVO2 benchmark artifacts. All formal primary cells have
three repetitions, fixed 0,2,4,6 affinity, four workers, Release builds, and
complete evaluator/resource artifacts.

## A. State consensus

Primary repository: /home/lc/super_livo/src/FAST-LIVO2
Branch: prob-livo
Expected Prompt18 frontier:
3974c77356315b735eb8c09cddd5422fcb2f710c

Prompt19 execution started from that frontier. The exact sequence of
Prompt19 commits before this evidence close is:

| Commit | Purpose |
|---|---|
| 5379f63 | explicit native_blind_carry / strict_reclass policy switch |
| f3bafab | canonical balanced Prompt19 run order |
| 31b9a8a | corrected affinity ledger encoding |
| 9a3f003 | finalized canonical run IDs |
| 781ce08 | predeclared replacement runs |
| 407da0a | predeclared clean serial replacement order |

No Native estimator math was modified. The exact owner prompt is registered
at prompts/prob_livo/prompt19_canonical_stride_resource_closure.md. Its
registration is recorded in prompts/prob_livo/README.md; this repository does
not have a separate top-level prompts/README.md.

## B. Canonical P0 restoration

The explicit switch preserves Prompt18 strict reclassification while making
native_blind_carry the canonical default. Carry is promoted as Native does;
strict carry-reclassification counters remain zero. Native-compatible
late-point deskew remains active.

The required pre-batch restoration gate used Quarter_01 and returned:

| Check | Result |
|---|---:|
| evaluator / run RC | 0 / 0 |
| trajectory rows | 5737 |
| backend attempted/success/rejected | 5744 / 5744 / 0 |
| visual calls / camera callbacks | 5737 / 5746 |
| scheduler count | 11487 |
| current / next / current-future points | 155663231 / 119442916 / 42205812 |
| blind carry points | 119399228 |
| carry reclassified current/next | 0 / 0 |
| ATE RMSE | 0.0534 m |

This establishes Prompt17 scheduler and bucket semantics. It is not one of
the three formal P0 repetitions: visual processing is numerically
nondeterministic, so no byte-identical trajectory or exact ATE claim is made
against the earlier single Prompt17 artifact.

## C. Oxford camera policy authority

The frozen Prompt18 policy was used without result-dependent tuning:

| Sequence | P0/P1/N0 full rate | P0S/P2/N1 stride |
|---|---:|---:|
| Church_05 | 1 | 1, negative control |
| College_03 | 1 | 2 |
| Palace_01 | 1 | 2 |
| Quarter_01 | 1 | 2 |

The corrected Prompt16 official Oxford overlay was retained, including
filter_size_surf=0.5 and lio/min_eigen_value=0.01. No visual, LiDAR, IMU,
map, exposure, pyramid, residual, or scheduler tuning was done. NTU was not
rerun.

## D. Camera timestamp identity

Selected timestamp streams are exact. P0S and N1 match byte-for-byte; for
Church, the stride-1 negative-control streams also match the full-rate
streams.

| Sequence | Full-rate count / SHA256 | Stride count / SHA256 | Identity |
|---|---|---|---|
| Church_05 | 3961 / ada0f71ab84fe97d78a9d7ae4c17418f08876e98599731687d32f5778ef30336 | 3961 / same | PASS |
| College_03 | 5682 / 40843a601ddb1db664cc929d474c35772bf1df829b8391199a1d863fb61e5b12 | 2841 / 30df87679ddd892df4026617ebf7c051e788d4e65905bcbffd6191be7598b089 | PASS, exact even-index subsequence |
| Palace_01 | 8041 / 8db5c4ba7b35334f4f20007a49f58e8321909b351649c4a5815bbdcf0d0cc0da | 4021 / b313f82875991a00193731dbf99d240b5c3db8cdf21e0e2ce24611fe05352986 | PASS, exact even-index subsequence |
| Quarter_01 | 5746 / bbcbc21cfa3130cc6b5800132555fa58363dbcf30d39b1c859100f9ada4b11c8 | 2873 / 4308c1d304108bf0ff767ac33bc467d3300deb479803233cce9030576f8d15fc | PASS, exact even-index subsequence |

## E. Canonical run ledger

The complete row-level ledger is in PROMPT19_RUN_LEDGER.csv. It contains
73 rows: 24 canonical primary rows, 48 Prompt18/Prompt15
resource-compatible secondary rows (P1/P2/N0/N1), and one P0S online parity
row.

Every formal primary row has evaluator, counter, and completion RC 0; complete
trajectory and ground truth; affinity 0,2,4,6; worker limit 4; Release
build; selected-camera SHA; and offline system memory sampling.

The balanced order is in PROMPT19_RUN_ORDER.csv. Three stale runner shell
groups from earlier tool sessions survived and overlapped. They were
terminated after exact process-group inspection. Affected complete/partial
extra directories are excluded from all statistics; formal repetitions 2/3
use the predeclared clean-serial replacement directories in
PROMPT19_CLEAN_SERIAL_RUN_ORDER.csv. No excluded row is silently substituted
or deleted.

## F. Accuracy

Each cell is mean ± sample SD, followed by median and min/max in metres.

### Table A — Canonical accuracy

| Sequence | P0 | P0S | ΔProb stride | N0 | N1 | ΔNative stride |
|---|---|---|---:|---|---|---:|
| Church_05 | 0.2296 ± 0.0018; 0.2290; 0.2282/0.2316 | 0.2282 ± 0.0042; 0.2298; 0.2234/0.2314 | -0.0014 | 0.2111 ± 0.0016; 0.2108; 0.2097/0.2128 | 0.2093 ± 0.0009; 0.2094; 0.2084/0.2102 | -0.0018 |
| College_03 | 0.1103 ± 0.0052; 0.1095; 0.1055/0.1159 | 0.0923 ± 0.0013; 0.0928; 0.0908/0.0932 | -0.0180 | 0.0746 ± 0.0002; 0.0746; 0.0744/0.0748 | 0.0514 ± 0.0001; 0.0514; 0.0513/0.0514 | -0.0232 |
| Palace_01 | 0.2190 ± 0.0032; 0.2181; 0.2163/0.2225 | 0.1301 ± 0.0016; 0.1306; 0.1284/0.1314 | -0.0889 | 0.1294 ± 0.0004; 0.1295; 0.1289/0.1297 | 0.1549 ± 0.0009; 0.1547; 0.1541/0.1559 | +0.0255 |
| Quarter_01 | 0.0660 ± 0.0026; 0.0674; 0.0630/0.0675 | 0.0654 ± 0.0014; 0.0655; 0.0640/0.0668 | -0.0006 | 0.0722 ± 0.0027; 0.0709; 0.0703/0.0753 | 0.0662 ± 0.0000; 0.0662; 0.0662/0.0662 | -0.0060 |

### Table B — Canonical Prob/Native gap

| Sequence | P0−N0 full rate | P0S−N1 stride |
|---|---:|---:|
| Church_05 | +0.0185 m | +0.0189 m |
| College_03 | +0.0357 m | +0.0409 m |
| Palace_01 | +0.0896 m | -0.0248 m |
| Quarter_01 | -0.0062 m | -0.0008 m |

Church is a no-treatment control because both arms use stride 1. On the
three true stride sequences, canonical Prob changes by -0.0180 m, -0.0889 m,
and -0.0006 m for College/Palace/Quarter. Native changes by -0.0232 m,
+0.0255 m, and -0.0060 m. Stride therefore has no universal accuracy
direction; the largest tested Prob benefit is Palace.

## G. Runtime

The wall/epoch values below are estimator wall compute divided by the
authoritative estimator epoch count. Total offline wall is shown separately.

### Table C — Estimator wall compute

| Sequence | P0 wall/epoch s | P0S wall/epoch s | ratio | N0 wall/epoch s | N1 wall/epoch s | ratio |
|---|---:|---:|---:|---:|---:|---:|
| Church_05 | 0.016226 | 0.016362 | 1.008 | 0.045592 | 0.049117 | 1.077 |
| College_03 | 0.009429 | 0.014940 | 1.584 | 0.014681 | 0.032807 | 2.235 |
| Palace_01 | 0.008340 | 0.013934 | 1.671 | 0.015874 | 0.028561 | 1.799 |
| Quarter_01 | 0.009981 | 0.016216 | 1.625 | 0.019098 | 0.037058 | 1.941 |

### Total wall and estimator CPU medians

| Sequence | P0 wall / CPU s | P0S wall / CPU s | Prob wall / CPU ratio | N0 wall / CPU s | N1 wall / CPU s | Native wall / CPU ratio |
|---|---:|---:|---:|---:|---:|---:|
| Church_05 | 187.0 / 335.3 | 188.7 / 337.3 | 1.009 / 1.006 | 411.5 / 948.3 | 443.6 / 1031.4 | 1.078 / 1.088 |
| College_03 | 183.8 / 269.4 | 127.5 / 221.1 | 0.694 / 0.821 | 223.2 / 441.7 | 224.4 / 507.5 | 1.006 / 1.149 |
| Palace_01 | 242.3 / 344.2 | 172.3 / 293.4 | 0.711 / 0.852 | 347.0 / 685.2 | 283.0 / 616.3 | 0.815 / 0.899 |
| Quarter_01 | 194.8 / 294.4 | 137.9 / 248.9 | 0.708 / 0.845 | 284.5 / 590.1 | 252.1 / 578.2 | 0.886 / 0.980 |

## H. CPU

### Table D — Estimator process CPU per epoch

| Sequence | P0 CPU/epoch s | P0S CPU/epoch s | ratio | N0 CPU/epoch s | N1 CPU/epoch s | ratio |
|---|---:|---:|---:|---:|---:|---:|
| Church_05 | 0.042334 | 0.042591 | 1.006 | 0.119735 | 0.130228 | 1.088 |
| College_03 | 0.023721 | 0.038933 | 1.641 | 0.038886 | 0.089365 | 2.298 |
| Palace_01 | 0.021414 | 0.036495 | 1.704 | 0.042626 | 0.076668 | 1.798 |
| Quarter_01 | 0.025628 | 0.043334 | 1.691 | 0.051367 | 0.100675 | 1.961 |

Median raw whole-process CPU utilization and normalized four-core
utilization (raw/4) were:

| Sequence | P0 | P0S | N0 | N1 |
|---|---:|---:|---:|---:|
| Church_05 | 256.2% / 64.1% | 256.7% / 64.2% | 239.1% / 59.8% | 246.7% / 61.7% |
| College_03 | 214.9% / 53.7% | 254.0% / 63.5% | 250.8% / 62.7% | 258.1% / 64.5% |
| Palace_01 | 214.9% / 53.7% | 257.4% / 64.4% | 254.0% / 63.5% | 250.4% / 62.6% |
| Quarter_01 | 219.6% / 54.9% | 260.1% / 65.0% | 251.6% / 62.9% | 250.3% / 62.6% |

## I. Memory

All memory values are OFFLINE_SYSTEM_PROCESS_MEMORY: RSS/PSS/USS include
the bag reader, image decode, adapter, and estimator. They are not pure map
memory and are not compared to Prompt15 estimator-only online percentages.

### Table E — Peak/final memory medians (MiB)

| Sequence | Arm | Peak PSS | Final PSS | Peak USS | Final USS |
|---|---|---:|---:|---:|---:|
| Church_05 | P0 | 8054.3 | 8054.3 | 8051.9 | 8051.9 |
| Church_05 | P0S | 8065.5 | 8065.5 | 8061.4 | 8061.4 |
| Church_05 | N0 | 8384.9 | 8380.9 | 8381.7 | 8377.8 |
| Church_05 | N1 | 8351.9 | 8351.9 | 8349.2 | 8349.2 |
| College_03 | P0 | 10847.3 | 10847.3 | 10842.4 | 10842.4 |
| College_03 | P0S | 5913.0 | 5913.0 | 5910.8 | 5910.8 |
| College_03 | N0 | 11491.7 | 11491.7 | 11488.8 | 11488.8 |
| College_03 | N1 | 6019.3 | 6019.3 | 6016.6 | 6016.6 |
| Palace_01 | P0 | 14773.5 | 14773.5 | 14769.4 | 14769.4 |
| Palace_01 | P0S | 7945.9 | 7945.9 | 7942.0 | 7942.0 |
| Palace_01 | N0 | 15968.4 | 15968.4 | 15965.3 | 15965.3 |
| Palace_01 | N1 | 8467.5 | 8467.5 | 8464.6 | 8464.6 |
| Quarter_01 | P0 | 10879.1 | 10879.1 | 10875.4 | 10875.4 |
| Quarter_01 | P0S | 6096.1 | 6096.1 | 6093.8 | 6093.8 |
| Quarter_01 | N0 | 11772.7 | 11768.5 | 11770.0 | 11765.8 |
| Quarter_01 | N1 | 6564.6 | 6564.6 | 6561.9 | 6561.9 |

P0S/P0 peak PSS ratios are 1.001, 0.545, 0.538, and 0.560 for
Church/College/Palace/Quarter. Native N1/N0 peak PSS ratios are 0.996,
0.524, 0.530, and 0.558. Peak USS ratios follow the same pattern:
Prob 1.001/0.545/0.538/0.560 and Native 0.996/0.524/0.530/0.558.

Fixed progress checkpoints were sampled at 10%, 25%, 50%, 75%, 90%, end,
and peak. Each tuple is RSS/PSS/USS MiB, median over three runs:

| Sequence / arm | 10% | 25% | 50% | 75% | 90% | end / peak |
|---|---|---|---|---|---|---|
| Church P0 | 845.7/828.6/824.4 | 2125.6/2110.8/2108.6 | 4161.6/4146.7/4144.3 | 6051.2/6036.8/6034.3 | 7292.1/7277.3/7274.9 | 8069.0/8054.3/8051.9 |
| Church P0S | 847.0/829.9/825.8 | 2089.4/2073.9/2071.8 | 4166.9/4149.8/4145.7 | 6020.3/6006.9/6003.1 | 7267.6/7250.5/7246.4 | 8082.7/8065.5/8061.4 |
| College P0 | 1170.7/1153.5/1149.4 | 2982.2/2966.8/2964.8 | 5851.3/5836.1/5834.1 | 8464.8/8447.6/8443.6 | 9969.8/9954.0/9951.9 | 10859.8/10847.3/10842.4 |
| College P0S | 701.1/686.3/683.6 | 1687.0/1670.1/1666.1 | 3131.1/3114.1/3110.1 | 4571.0/4556.4/4553.7 | 5337.2/5321.9/5319.6 | 5928.2/5913.0/5910.8 |
| Palace P0 | 1869.2/1854.2/1852.1 | 4258.4/4241.2/4237.1 | 7566.3/7549.1/7545.0 | 10947.8/10930.8/10926.7 | 13229.2/13214.1/13212.0 | 14790.6/14773.5/14769.4 |
| Palace P0S | 1137.8/1122.5/1120.5 | 2309.7/2295.5/2292.4 | 4059.4/4045.6/4042.5 | 5725.7/5710.3/5708.3 | 6995.1/6981.1/6978.0 | 7962.8/7945.9/7942.0 |
| Quarter P0 | 1376.5/1364.3/1360.6 | 3058.3/3041.2/3037.1 | 5864.6/5851.2/5847.4 | 8551.0/8533.8/8529.7 | 10028.8/10011.5/10007.4 | 10892.4/10879.1/10875.4 |
| Quarter P0S | 824.3/809.4/807.0 | 1645.7/1628.7/1624.6 | 3213.6/3196.5/3192.4 | 4741.1/4725.8/4723.9 | 5569.2/5553.9/5551.9 | 6111.0/6096.1/6093.8 |

The N0/N1 fixed-progress samples are present in the reused Prompt15/18
memory.csv artifacts; their peak/final medians are in Table E and all
row-level values are in the ledger.

## J. Visual workload

### Table F — Retained camera and visual workload medians

| Sequence | Arm | Input seen / retained | VIO calls | Visual commits |
|---|---|---:|---:|---:|
| Church_05 | P0 | 3961 / 3961 | 3956 | 3954 |
| Church_05 | P0S | 3961 / 3961 | 3956 | 3954 |
| Church_05 | N0 | 3961 / 3961 | 3960 | 3959 |
| Church_05 | N1 | 3961 / 3961 | 3960 | 3959 |
| College_03 | P0 | 5682 / 5682 | 5673 | 2872 |
| College_03 | P0S | 5682 / 2841 | 2834 | 2832 |
| College_03 | N0 | 5682 / 5682 | 5679 | 5672 |
| College_03 | N1 | 5682 / 2841 | 2839 | 2837 |
| Palace_01 | P0 | 8041 / 8041 | 8031 | 4053 |
| Palace_01 | P0S | 8041 / 4021 | 4014 | 4012 |
| Palace_01 | N0 | 8041 / 8041 | 8037 | 8029 |
| Palace_01 | N1 | 8041 / 4021 | 4019 | 4017 |
| Quarter_01 | P0 | 5746 / 5746 | 5737 | 2893 |
| Quarter_01 | P0S | 5746 / 2873 | 2866 | 2864 |
| Quarter_01 | N0 | 5746 / 5746 | 5743 | 5735 |
| Quarter_01 | N1 | 5746 / 2873 | 2871 | 2869 |

On true stride-2 sequences, retained images and VIO calls are almost halved.
This explains the lower total wall, estimator CPU, and system memory. Church
has identical camera workload by construction.

## K. P0/P0S canonical stride effect

| Sequence | Wall ratio | Estimator CPU ratio | Peak PSS ratio | Peak USS ratio |
|---|---:|---:|---:|---:|
| Church_05 | 1.009 | 1.006 | 1.001 | 1.001 |
| College_03 | 0.694 | 0.821 | 0.545 | 0.545 |
| Palace_01 | 0.711 | 0.852 | 0.538 | 0.538 |
| Quarter_01 | 0.708 | 0.845 | 0.560 | 0.560 |

On College/Palace/Quarter, Prob stride reduces total wall by approximately
30.6%, 28.9%, and 29.2%, and estimator CPU by 17.9%, 14.8%, and 15.5%.

## L. N0/N1 Native stride effect

| Sequence | Wall ratio | Estimator CPU ratio | Peak PSS ratio | Peak USS ratio |
|---|---:|---:|---:|---:|
| Church_05 | 1.078 | 1.088 | 0.996 | 0.996 |
| College_03 | 1.006 | 1.149 | 0.524 | 0.524 |
| Palace_01 | 0.815 | 0.899 | 0.530 | 0.530 |
| Quarter_01 | 0.886 | 0.980 | 0.558 | 0.558 |

N0/N1 reuse is valid because the Prompt15/18 artifacts are complete and
resource-compatible under the same four-core contract. Native estimator code
was not changed.

## M. P0/P0S/P1/P2 factorial interpretation

P1/P2 are secondary Prompt18 strict-reclassification evidence and do not
define the canonical Prob arm.

### Table G — Carry policy × stride ATE

Each cell is mean ± SD; median; min/max in metres.

| Sequence | P0 blind/full | P0S blind/stride | P1 strict/full | P2 strict/stride |
|---|---|---|---|---|
| Church_05 | 0.2296 ± 0.0018; 0.2290; 0.2282/0.2316 | 0.2282 ± 0.0042; 0.2298; 0.2234/0.2314 | 0.2250 ± 0.0016; 0.2241; 0.2240/0.2268 | 0.2312 ± 0.0021; 0.2306; 0.2294/0.2335 |
| College_03 | 0.1103 ± 0.0052; 0.1095; 0.1055/0.1159 | 0.0923 ± 0.0013; 0.0928; 0.0908/0.0932 | 0.0966 ± 0.0018; 0.0961; 0.0952/0.0986 | 0.0921 ± 0.0012; 0.0923; 0.0908/0.0931 |
| Palace_01 | 0.2190 ± 0.0032; 0.2181; 0.2163/0.2225 | 0.1301 ± 0.0016; 0.1306; 0.1284/0.1314 | 0.3291 ± 0.0060; 0.3291; 0.3231/0.3350 | 0.1302 ± 0.0002; 0.1302; 0.1300/0.1303 |
| Quarter_01 | 0.0660 ± 0.0026; 0.0674; 0.0630/0.0675 | 0.0654 ± 0.0014; 0.0655; 0.0640/0.0668 | 0.0704 ± 0.0036; 0.0694; 0.0673/0.0744 | 0.0655 ± 0.0013; 0.0658; 0.0641/0.0667 |

Requested contrasts in metres:

| Sequence | P1−P0 | P2−P0S | P0S−P0 | P2−P1 |
|---|---:|---:|---:|---:|
| Church_05 | -0.0046 | +0.0030 | -0.0014 | +0.0062 |
| College_03 | -0.0137 | -0.0002 | -0.0180 | -0.0045 |
| Palace_01 | +0.1101 | +0.0001 | -0.0889 | -0.1989 |
| Quarter_01 | +0.0044 | +0.0001 | -0.0006 | -0.0049 |

Strict carry interacts strongly with stride on Palace: P1 is 0.3291 m and P2
is 0.1302 m. The other under-stride P2/P0S differences are near the
observed numerical spread. This is limited to the tested factorial.

## N. Online/offline verification

The required P0S check used:
online: results/prob_livo/runs/prompt19_online_p0s_Quarter_01
offline: results/prob_livo/runs/prompt19_clean_p0s_Quarter_01_r3

Both have 2866 trajectory rows, backend 2872/2872/0, visual calls 2866,
visual commits 2864, selected count 2873, and selected SHA
4308c1d304108bf0ff767ac33bc467d3300deb479803233cce9030576f8d15fc. Their
bucket traces have 2872 rows and exact SHA
b075387679ae9c141c57a234f803e5a89ebeb4c791e03a28a6f3864cb21c42c8.
Online ATE is 0.0655 m; offline ATE is 0.0640 m. Trajectory bytes differ:
online SHA 5921df9335e8eb048b9c0be4f354af01480ff8e05c8d7a9a8c2367a3f815fe7;
offline SHA 6ba17916734a37f5f1160dbaa4b3e407acd832113db9d36c1f5a83f252436052.
This is within the accepted visual numerical nondeterminism contract.
Event source, selected input, bucket semantics, counters, and evaluator
coverage are identical.

Native N1 Quarter event identity was established in Prompt18:
image_seen=5746, image_read=2873, image_dropped=2873, scheduler
steps/sync=5743/5743, visual calls=2871, commits=2869, trajectory rows=2870,
with exact selected-list identity across N1 repetitions. No event-source
mismatch was found.

## O. Build/resource authority

The project build passed with:

    source /opt/ros/noetic/setup.bash
    catkin_make --pkg fast_livo -j4

All formal Prob runs used TBB/Prob worker limit 4, OMP limit 4, Release,
affinity 0,2,4,6, and no RViz, PCD, sanitizer, debug trace, or heavy
diagnostics. Native N0/N1 reuse has the same four-core affinity and worker
contract.

Focused I2, I3, I4, I5, and I6 visual-gate tests passed after the final
transport correction. No Prompt15 full rerun, NTU stride run, P5, I8, or
H1/H2 tuning was performed.

## P. Failures and deviations

The only execution-integrity incident was stale process-group overlap from
earlier tool sessions. It explains the approximately 15 GiB host memory
pressure together with the intrinsically large single-process Palace P0 peak
(about 14.79 GiB PSS). This was not intentional parallel benchmark design.
Exact stale groups and children were terminated; after cleanup no
prob_livo_offline, fastlivo_mapping, or Prompt19 runner remained.

Affected old/incomplete/overlapped directories are retained for provenance
but classified CONTAMINATED, EXECUTION_FAIL, or INCOMPLETE_TRAJECTORY as
applicable and excluded. Clean serial replacement rows are the authoritative
repetitions 2/3. No canonical primary row uses an event-source mismatch,
missing evaluator, missing trajectory, or missing resource artifact.

The online parity trajectory is not claimed byte-identical because visual
floating-point/order nondeterminism is an accepted contract; its event and
input identity is exact. This is the only expected non-byte-identity
deviation in the accepted parity result.

## Q. Final owner answers

1. Prompt17 P0 is reproducibly canonical at scheduler and bucket-semantics
   level: Native blind carry, native late-point deskew, zero strict
   reclassification, and complete accepted epochs. Visual trajectory bytes
   remain nondeterministic.
2. Canonical Prob P0 to P0S is Church -0.0014 m as a stride-1 control,
   College -0.0180 m, Palace -0.0889 m, and Quarter -0.0006 m.
3. Native N0 to N1 is Church -0.0018 m, College -0.0232 m, Palace +0.0255 m,
   and Quarter -0.0060 m.
4. Full-rate P0−N0 is +0.0185, +0.0357, +0.0896, -0.0062 m; stride
   P0S−N1 is +0.0189, +0.0409, -0.0248, -0.0008 m in sequence order.
5. Prob stride reduces total wall/estimator CPU on the true stride sequences
   by about 30%/18%, 29%/15%, and 29%/16% for College/Palace/Quarter.
6. High-rate Prob peak PSS/USS falls to 54.5%/53.8%/56.0% of full-rate;
   Native falls to 52.4%/53.0%/55.8% on peak PSS/USS.
7. Retained images and visual calls are nearly halved under stride, directly
   explaining the process-level resource reduction.
8. Strict carry interaction is strongest on Palace, P1 0.3291 m to P2
   0.1302 m. P1/P2 remain secondary and do not redefine P0/P0S.

Prompt19 stops here. No automatic Prompt15 rerun, NTU extension, or tuning
follows this report.
