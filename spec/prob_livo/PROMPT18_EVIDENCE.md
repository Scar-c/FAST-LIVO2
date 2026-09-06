# Prompt18 Oxford P-LIVO Bucket Corrective + Stride Ablation

## Final status

`PROMPT18 BUCKET CLOSED / STRIDE OWNER DECISION REQUIRED`

The P-LIVO current/next corrective is implemented and verified on all four
Oxford sequences. The frozen camera stride ablation is complete for Prob and
Native. The remaining owner decision is explicit: the current Native source
still contains the original blind carry promotion, while corrected Prob-LIVO
reclassifies carried points against the new endpoint. Therefore this report
does not claim the Prompt18 forbidden point-by-point `Native current == Prob
current` contract. Native was kept unchanged for N0/N1 as required by Axis B.

Prompt hash: `6d332f9e75cebd426b021f775de566e2bdb34bb440c3b5c85a44da1f233b9da7`.

## Scope and execution contract

Only these two variables were changed:

1. P-LIVO current/next bucket ownership and carry reclassification.
2. Deterministic camera transport stride, frozen before ATE was inspected.

All canonical runs used Oxford `0,2,4,6` CPU affinity, four workers, the
Prompt16 official overlay, `fast_native` semantics, and the project-owned
in-process rosbag reader for Prob. No estimator, LiDAR, IMU, exposure, map,
pyramid, residual, or visual-gate tuning was performed. NTU was limited to the
rate audit; no NTU stride benchmark was run.

The P0 source was `dc5242cae8f2e9c2902ed322ba71e9212ea6cd8a`; corrected P1/P2
offline runs used `c3f7f69e132a4ae536977af8b96c33480e7118a3`. The online parity
run used final transport commits through `c70386490a553196bd4b27483670c11a9f535290`.
The Native stride runner was built from local commits through
`84757c4bbd4c74ef2791395a94f3df07cb9a5c1a`.

## Pre-edit gates and rate audit

The required pre-edit online H0 completed before the Prompt18 algorithm edit:

| Run | RC | Rows | ATE RMSE | Backend attempted/success/rejected |
|---|---:|---:|---:|---:|
| `results/prob_livo/runs/prompt18_pre_online_h0_eee01` | 0 | 3979 | 0.04914769199782388 m | 3984/3984/0 |

The full frozen rate table is in
[`PROMPT18_RATE_AUDIT.csv`](PROMPT18_RATE_AUDIT.csv). Oxford decisions were
Church stride 1 and College/Palace/Quarter stride 2. Church is therefore a
negative control; no ATE-dependent stride selection was used.

## Native scheduler retrace and corrective

Before changing P-LIVO, the Native FAST-LIVO2 scheduler was retraced against
synthetic boundary cases (`t_i < T_e`, `t_i == T_e`, `t_i > T_e`, future points,
non-monotone point order, multiple camera epochs, and carry-over). The strict
boundary rule keeps equality in the next bucket. The real Oxford P0 trace
confirmed the carry leak:

| Sequence | P0 late points | Epochs with late points | Median epoch median lateness | Maximum lateness |
|---|---:|---:|---:|---:|
| Church_05 | 0 | 0 | n/a | n/a |
| College_03 | 37,761,895 | 2,820 | 11,006.95 us | 50,721.41 us |
| Palace_01 | 56,539,219 | 3,983 | 12,220.86 us | 50,242.19 us |
| Quarter_01 | 42,205,770 | 2,846 | 13,554.28 us | 50,237.66 us |

The corrected project path first copies the previous next bucket into a local
carry set, clears both buckets, reclassifies every carried point against the
new endpoint, and then classifies new raw points. `t_i == T_e` remains next;
no double rebase, endpoint clamp, or `T_e=max(point_time)` was added. The
synthetic regression is covered by the I2 continuity suite.

The unresolved contract is visible in the source audit: Native
`src/LIVMapper.cpp` still performs blind `current = next` carry promotion at
its LIVO cut, whereas project P1 uses
`prob_livo::RebaseLivoCarryOverPoint`. The Native estimator/config/build was
not changed for N0/N1, so the report records this as an owner decision rather
than silently calling the two classifications identical.

## P0 → P1: independent bucket effect

P0 is the pre-corrective full-rate baseline. P0 direct bucket counters were not
present in the pre-edit binary; its authoritative `late_point_trace.csv`
provides the leakage counts above. P1 has the new bucket counters and every
formal P1 run reports `current_future_points=0`, backend rejected `0`, and full
trajectory coverage.

| Sequence | P0 ATE runs (m) | P0 mean±SD | P1 ATE runs (m) | P1 mean±SD | ΔATE P1−P0 |
|---|---|---:|---|---:|---:|
| Church_05 | 0.2303, 0.2342, 0.2352 | 0.2332±0.0026 | 0.2268, 0.2241, 0.2240 | 0.2250±0.0016 | -0.0083 |
| College_03 | 0.2593, 0.1070, 0.1072 | 0.1578±0.0879 | 0.0961, 0.0986, 0.0952 | 0.0966±0.0018 | -0.0612 |
| Palace_01 | 0.2200, 0.2166, 0.2232 | 0.2199±0.0033 | 0.3350, 0.3231, 0.3291 | 0.3291±0.0060 | +0.1091 |
| Quarter_01 | 0.0689, 0.0681, 0.0639 | 0.0670±0.0027 | 0.0694, 0.0673, 0.0744 | 0.0704±0.0036 | +0.0034 |

P1 bucket telemetry is deterministic per sequence across the three runs. The
representative totals are:

| Sequence | Current points | Next points | Carry points | Carry→current | Carry→next | Current future points |
|---|---:|---:|---:|---:|---:|---:|
| Church_05 | 271,874,198 | 65,660,460 | 65,626,649 | 65,626,649 | 0 | 0 |
| College_03 | 173,382,109 | 171,567,927 | 171,527,020 | 133,764,156 | 37,762,864 | 0 |
| Palace_01 | 200,345,124 | 206,039,501 | 206,019,539 | 149,479,566 | 56,539,973 | 0 |
| Quarter_01 | 155,663,231 | 161,649,254 | 161,605,566 | 119,399,228 | 42,206,338 | 0 |

All P1 backend counters are accepted (`attempted=success`, `rejected=0`).
Palace P-LIO regression also passes independently: run
`results/prob_livo/runs/prompt18_regression_p_lio_Palace_01`, 4047 rows,
4052/4052/0 backend attempted/success/rejected, ATE `0.1272 m`, exactly the
Prompt17 reference value.

## P1 → P2 and N0 → N1 stride ablation

N0 reuses the 12 valid Prompt15 Native N-LIVO full-rate cells because Prompt18
prohibits a Prompt15 full rerun. Those cells use the same Oxford config,
`0,2,4,6` affinity, worker=4, and unchanged Native estimator. N1 is the new
Native stride batch: 12 runs, all clean, all evaluator RC0.

| Sequence | Frozen stride | P1 full-rate | P2 stride | ΔP | Native N0 full-rate | Native N1 stride | ΔN |
|---|---:|---:|---:|---:|---:|---:|---:|
| Church_05 | 1 | 0.2250±0.0016 | 0.2312±0.0021 | +0.0062 | 0.2111±0.0016 | 0.2093±0.0009 | -0.0018 |
| College_03 | 2 | 0.0966±0.0018 | 0.0921±0.0012 | -0.0046 | 0.0746±0.0002 | 0.0514±0.0001 | -0.0232 |
| Palace_01 | 2 | 0.3291±0.0060 | 0.1302±0.0002 | -0.1989 | 0.1294±0.0004 | 0.1549±0.0009 | +0.0255 |
| Quarter_01 | 2 | 0.0704±0.0036 | 0.0655±0.0013 | -0.0048 | 0.0722±0.0027 | 0.0662±0.0000 | -0.0060 |

The full-rate and same-rate Prob/Native gaps are:

| Sequence | `gap_full = P1−N0` | `gap_stride = P2−N1` |
|---|---:|---:|
| Church_05 | +0.0139 m | +0.0218 m |
| College_03 | +0.0220 m | +0.0407 m |
| Palace_01 | +0.1997 m | -0.0247 m |
| Quarter_01 | -0.0018 m | -0.0007 m |

Stride is sequence-dependent. It strongly improves Prob on Palace and modestly
improves College/Quarter, while slightly worsens Church. Native improves on
Church/College/Quarter but worsens Palace. The largest beneficiary in this
ablation is Prob-LIVO on Palace; the gap narrows substantially there after
stride, but widens on Church and College.

Selected input identity passed before interpreting these ATEs:

| Sequence | P1 selected count/SHA | P2 selected count/SHA | P2 = P1 subsample |
|---|---|---|---|
| Church_05 | 3961 / `ada0f71ab84fe97d78a9d7ae4c17418f08876e98599731687d32f5778ef30336` | 3961 / same | PASS, exact |
| College_03 | 5682 / `40843a601ddb1db664cc929d474c35772bf1df829b8391199a1d863fb61e5b12` | 2841 / `30df87679ddd892df4026617ebf7c051e788d4e65905bcbffd6191be7598b089` | PASS, exact even-index sequence |
| Palace_01 | 8041 / `8db5c4ba7b35334f4f20007a49f58e8321909b351649c4a5815bbdcf0d0cc0da` | 4021 / `b313f82875991a00193731dbf99d240b5c3db8cdf21e0e2ce24611fe05352986` | PASS, exact even-index sequence |
| Quarter_01 | 5746 / `bbcbc21cfa3130cc6b5800132555fa58363dbcf30d39b1c859100f9ada4b11c8` | 2873 / `4308c1d304108bf0ff767ac33bc467d3300deb479803233cce9030576f8d15fc` | PASS, exact even-index sequence |

Native N1 selected lists equal the corresponding P2 lists exactly for all four
sequences. Native Quarter N1 r1/r2/r3 also has identical event-level reader
and scheduler counts: `image_seen=5746`, `image_read=2873`,
`image_dropped=2873`, scheduler steps/sync packages `5743/5743`, visual calls
`2871`, commits `2869`, and trajectory rows `2870`. This is the required Native
N1 event-semantic identity check; trajectory bytes remain subject to the
accepted visual numerical nondeterminism envelope.

## Online/offline parity

Quarter_01 is the high-rate online/offline check. The online runner was
extended to subscribe to the bag's actual `sensor_msgs/CompressedImage` topic,
apply the same pre-decode deterministic stride gate, record selected raw image
timestamps, and emit the same bucket trace. The previous type-mismatch and
double-gate attempts are retained as failed infrastructure evidence and are
not used below.

| Check | P1 full-rate | P2 stride=2 |
|---|---:|---:|
| Offline rows / online rows | 5737 / 5737 | 2866 / 2866 |
| Selected timestamp identity | exact, 5746, SHA `bbcbc21c...` | exact, 2873, SHA `4308c1d3...` |
| Bucket trace rows / semantic hashes | 5744 / exact | 2872 / exact |
| Backend attempted/success/rejected | 5744/5744/0 both | 2872/2872/0 both |
| Visual process calls | 5737 both | 2866 both |
| Visual commits | 4877 offline / 4869 online | 2864 / 2864 |
| ATE RMSE | 0.0694 offline / 0.0692 online | 0.0641 offline / 0.0646 online |
| Trajectory timestamps | exact | exact |
| Max / mean position difference | 0.0742 / 0.0075 m | 0.0271 / 0.0052 m |

P1's eight-commit difference and non-byte-identical trajectory are runtime
visual-order effects; the event schedule, selected inputs, current/next
semantic hashes, backend counters, timestamps, and ATE remain within the
observed 4-core canonical spread. P2 is exact on visual calls and commits.
No byte-identical trajectory claim is made for the P1 online/offline pair.

## Build, tests, and artifacts

- Project build: `catkin_make --pkg fast_livo -j4`, successful.
- Focused I2, I3, I4, I5, and I6 suites: all PASS after the final transport
  correction.
- Native isolated build: `catkin_make_isolated ... -j4`, successful; Native
  reader tests pass.
- Canonical run details and all run directories are recorded in
  [`PROMPT18_RUN_LEDGER.csv`](PROMPT18_RUN_LEDGER.csv).
- No NTU stride benchmark, Prompt15 full rerun, P5, I8, or new Prompt18 H1/H2
  ablation was started.

The corrective implementation and runner are committed on branch `prob-livo`.
The final project push is recorded in the handoff after the report commit.
