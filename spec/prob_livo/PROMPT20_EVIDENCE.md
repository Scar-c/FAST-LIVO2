# Prompt20 — Visual Patch Leak + Parent-LRU Lifecycle Evidence

## Final status

`PROMPT20 LEAK CLOSED / VISUAL-LRU OWNER DECISION REQUIRED`

Prompt20 was completed on Oxford `Palace_01`, with Prob V0/V1/V2 at
stride 1/2 and Native N0L/N1L at full rate. The complete row-level ledger is
in [PROMPT20_RUN_LEDGER.csv](PROMPT20_RUN_LEDGER.csv).

The status is deliberately not the stronger lifecycle-validated status:
the implementation-level parent-LRU eviction contract is validated, but the
Palace runs used the Super parent domain capacity of 1,000,000 and reached
only about 84.7k parents at stride 1 and 73.4k at stride 2. Thus no runtime
eviction occurred, and the observed live visual state continued to grow with
the parent map. A future owner decision is required for a bounded runtime
visual-lifetime policy; no per-parent VP cap or selector was added here.

## A. State and scope

| Item | Value |
|---|---|
| Primary repository | `/home/lc/super_livo/src/FAST-LIVO2` |
| Primary branch | `prob-livo` |
| Prob implementation commit | `955af97` |
| Native repository | `/home/lc/super_livo/p15_native_FAST-LIVO2` |
| Native implementation commit | `ea7702b` |
| Dataset | Oxford / Palace_01 |
| Prob cells | V0/V1/V2 × stride 1/2 × 3 repetitions = 18 |
| Native cells | N0L/N1L × full-rate × 3 repetitions = 6 |
| Affinity | logical CPUs `0,2,4,6` |
| Worker/build | TBB/MP=4, Release, compilation `-j4` |
| Input semantics | `fast_native`, `native_blind_carry`, `livo2_prob_3sigma` |
| Result | all 24 runs `CLEAN_SUCCESS`, node/evaluator RC 0 |

The V1 stride-2 repetition 1 is the escalated ROS sandbox run
`prompt20_smoke_v1_Palace_01_s2_escalated`; it is included as the formal
V1/s2/r1 row, not silently discarded.

## B. Implemented ownership contract

| Object | Sole owner | References | Release order |
|---|---|---|---|
| Super parent host | `VisualParentRegistry::owners_` | eight non-owning local attachment vectors | registry eviction/clear |
| `VisualPoint` | `VisualParentHost::voxel_points` | `Feature*` observations | host destruction |
| `Feature` | `VisualPoint::obs_` | `cv::Mat img_`, patch owner | point destruction |
| Patch | `Feature::patch_owner_` (`unique_ptr<VisualPatch>`) | owned float buffer | Feature destruction |
| Observation | `VisualPoint::obs_` entry | non-owning point back-reference | Feature destruction |
| Visual index | `feat_map` / lookup | non-owning Feature/host references | erase before host destruction |
| Frame view | `visual_submap` | borrowed current-frame pointers | reset before parent eviction |
| Image reference | `Feature::img_` | OpenCV ref-counted image storage | Feature destruction/refcount release |

The V2 eviction sequence is: erase borrowed visual indexes, reset the
borrowed frame view, destroy the parent host, then recursively release
VisualPoints, Features, patches, observations, and image references. The
legacy V0 raw temporary allocation remains intentionally present only as the
control arm; V1/V2 use explicit local ownership and transfer it to Feature.

## C. Tests and implementation gates

The following command was run after the implementation build:

```text
/home/lc/super_livo/devel/lib/fast_livo/prob_livo_i7_visual_memory_tests
PROMPT20 I7 visual memory tests passed
```

The test source-contract gate rejects reintroduction of a raw temporary patch
allocation. Dynamic tests cover rejected-candidate RAII release, accepted
ownership transfer, Feature destruction without double free, 1,000 repeated
synthetic allocation/free cycles, zero live-patch accounting, and the
early-exit-equivalent local-owner path. Parent-LRU tests cover eight local
attachments, parent eviction, representative replacement, lookup without LRU
touch, stale-index/no-resurrection behavior, and release accounting.

The existing I6 visual gate regression also passed:

```text
[PASS] G-I6 visual plane gate policy and sensor-range oracle checks=19
```

## D. Required final table

Values are three-run means; memory is peak/final MiB, and ATE is mean ± sample
SD in metres. Calls and commits are the mean authoritative visual counters.

| System | Stage | Stride | ATE | Peak PSS | Final PSS | Peak USS | Final USS | VIO calls | Commits |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Prob | V0 | 1 | 0.2175 ± 0.0047 | 14758.2 | 14758.2 | 14755.8 | 14755.8 | 8031 | 4053.3 |
| Prob | V1 | 1 | 0.2205 ± 0.0032 | 14732.5 | 14732.5 | 14730.2 | 14730.2 | 8031 | 4053.7 |
| Prob | V2 | 1 | 0.2199 ± 0.0057 | 14713.0 | 14711.9 | 14710.9 | 14709.8 | 8031 | 4055.0 |
| Prob | V0 | 2 | 0.1298 ± 0.0012 | 7932.4 | 7930.8 | 7930.1 | 7928.5 | 4014 | 4011.7 |
| Prob | V1 | 2 | 0.1313 ± 0.0003 | 7892.6 | 7891.4 | 7889.8 | 7888.6 | 4014 | 4012.0 |
| Prob | V2 | 2 | 0.1305 ± 0.0007 | 7909.1 | 7907.6 | 7907.0 | 7905.4 | 4014 | 4011.7 |
| Native | N0L | 1 | 0.1305 ± 0.0016 | 15956.3 | 15956.3 | 15954.0 | 15954.0 | 8037 | 8029 |
| Native | N1L | 1 | 0.1292 ± 0.0015 | 15964.8 | 15964.8 | 15962.4 | 15962.4 | 8037 | 8029 |

Native counter activity stayed fixed at 8037 camera epochs, 8037 visual
process calls, and 8029 commits in all six runs. Prob activity stayed at 8031
calls for stride 1 and 4014 for stride 2; commit variation is the accepted
visual nondeterminism envelope, not a changed event source.

## E. Lifecycle checkpoints

The following are median checkpoints over the three V1 or V2 runs. Each
`parent/VP/Feature/observation` count is live; patch is live RAII-owned patch
bytes. `max` columns are the maximum attachment load observed in any parent.

| Stage/stride | Checkpoint | Parents | VP | Features | Obs | Patch MiB | VP/parent max | Feature/parent max | Patch/parent max |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| V1/1 | 10% | 8560 | 63054 | 72539 | 72539 | 17.7 | 84 | 95 | 24320 B |
| V1/1 | 25% | 21862 | 149133 | 177557 | 177557 | 43.4 | 99 | 107 | 27392 B |
| V1/1 | 50% | 46120 | 324692 | 386559 | 386559 | 94.3 | 99 | 112 | 28672 B |
| V1/1 | 75% | 67675 | 538139 | 618095 | 618095 | 150.9 | 123 | 147 | 37632 B |
| V1/1 | 90% | 80146 | 689191 | 776127 | 776127 | 189.4 | 123 | 147 | 37632 B |
| V1/1 | end | 84740 | 752640 | 844505 | 844505 | 206.2 | 129 | 147 | 37632 B |
| V2/1 | 10% | 8573 | 63187 | 72584 | 72584 | 17.7 | 83 | 104 | 26624 B |
| V2/1 | 25% | 21841 | 149358 | 177324 | 177324 | 43.3 | 100 | 120 | 30720 B |
| V2/1 | 50% | 46064 | 324724 | 387134 | 387134 | 94.5 | 100 | 120 | 30720 B |
| V2/1 | 75% | 67538 | 537717 | 617964 | 617964 | 150.8 | 127 | 136 | 34816 B |
| V2/1 | 90% | 79971 | 688585 | 775845 | 775845 | 189.4 | 127 | 136 | 34816 B |
| V2/1 | end | 84572 | 752251 | 844380 | 844380 | 206.2 | 127 | 136 | 34816 B |
| V1/2 | 10% | 8075 | 54088 | 63143 | 63143 | 15.4 | 79 | 79 | 20224 B |
| V1/2 | 25% | 19266 | 114716 | 141319 | 141319 | 34.5 | 81 | 92 | 23552 B |
| V1/2 | 50% | 40153 | 208471 | 265972 | 265972 | 64.9 | 81 | 92 | 23552 B |
| V1/2 | 75% | 59077 | 335392 | 410101 | 410101 | 100.1 | 94 | 116 | 29696 B |
| V1/2 | 90% | 71050 | 459849 | 540926 | 540926 | 132.1 | 94 | 116 | 29696 B |
| V1/2 | end | 73376 | 533851 | 616819 | 616819 | 150.6 | 153 | 153 | 39168 B |
| V2/2 | 10% | 8059 | 54030 | 63149 | 63149 | 15.4 | 79 | 81 | 20736 B |
| V2/2 | 25% | 19269 | 114456 | 140979 | 140979 | 34.4 | 80 | 94 | 24064 B |
| V2/2 | 50% | 40181 | 208283 | 265951 | 265951 | 64.9 | 80 | 94 | 24064 B |
| V2/2 | 75% | 59016 | 335051 | 410050 | 410050 | 100.1 | 89 | 116 | 29696 B |
| V2/2 | 90% | 70995 | 459697 | 541232 | 541232 | 132.1 | 89 | 116 | 29696 B |
| V2/2 | end | 73308 | 533229 | 616753 | 616753 | 150.5 | 153 | 153 | 39168 B |

These traces show that V2's parent registry remained below its configured
capacity, so runtime parent eviction was not exercised. They also show that
V2 does not impose an intra-parent VP/Feature cap, as required by scope.

## F. Memory attribution

The deltas below are mean MiB, expressed as `next stage - previous stage` in
the order Peak PSS / Final PSS / Peak USS / Final USS:

| Stride | Leak correction V1−V0 | Parent-LRU V2−V1 | Total V2−V0 |
|---:|---:|---:|---:|
| 1 | −25.7 / −25.7 / −25.6 / −25.6 | −19.5 / −20.6 / −19.3 / −20.4 | −45.2 / −46.3 / −44.9 / −46.0 |
| 2 | −39.8 / −39.4 / −40.3 / −39.9 | +16.5 / +16.2 / +17.2 / +16.8 | −23.3 / −23.2 / −23.1 / −23.1 |

These are whole-process offline RSS/PSS/USS measurements, including bag
reader, image decode, ROS transport, and estimator state. The differences are
therefore not a byte-exact estimate of the small temporary-patch leak. The
source contract and allocation accounting isolate the ownership correction;
the Palace process-level memory signal does not resolve a statistically clean
leak-only reduction. In particular, V2 is not claimed to be a monotonic
memory optimization: stride 2 is +16.5 MiB PSS versus its V1 repetitions.

Native N1L−N0L is +8.5 MiB peak PSS and +8.4 MiB peak USS, while ATE changes
from 0.1305 to 0.1292 m. This is within run-to-run process memory and accepted
Native visual nondeterminism; the Native leak contribution is not separable
from the approximately 15.9 GiB full-rate visual/map footprint by this
monitor. It is not evidence that the ownership fix failed: the Native source
path and ownership tests close the allocation contract.

## G. Required answers

1. **Patch leak:** Confirmed in the V0 temporary raw-allocation path and
   closed in V1/V2 with local RAII plus explicit Feature ownership transfer.
2. **Native contribution:** No separable process-level reduction; the measured
   N1L−N0L difference is +8.5 MiB PSS / +8.4 MiB USS, within noise.
3. **P-LIVO contribution:** V1 is lower by 25.7 MiB PSS at stride 1 and
   39.8 MiB at stride 2, but those whole-process deltas are not a direct byte
   count of the patch leak.
4. **V1 long-term growth:** Live visual state continues growing over Palace;
   the ownership fix removes the rejected-temporary leak but does not evict
   accepted visual state.
5. **V2 extra memory:** −19.5 MiB PSS at stride 1; +16.5 MiB at stride 2;
   no consistent reduction is established.
6. **Parent steady state:** Not reached in the Palace runs because 1,000,000
   is far above the observed 84.7k/73.4k parent counts. The implementation
   invariant is `live parent hosts <= configured capacity`.
7. **VP/Feature/patch plateau:** No; all grow with the parent map in both
   stride cells.
8. **Intra-parent accumulation:** The current run does not satisfy the
   prompt's “parent stable” precondition. Parent-domain growth is observed,
   and no per-parent policy is claimed.
9. **Eviction release:** Yes in the synthetic capacity-2 LRU test, including
   all eight local attachments and patch accounting. No Palace runtime
   eviction occurred at capacity 1,000,000.
10. **Visual LRU touch:** Yes. `findNoTouch` leaves LRU order unchanged;
    only insertion/replacement touches the parent entry, and the test passes.
11. **V2 stride validity:** Both stride 1 and stride 2 complete successfully
    with unchanged call structure and no ATE degradation beyond the accepted
    nondeterminism envelope.
12. **Palace ATE:** V1/V2 means are 0.2205/0.2199 m at stride 1 and
    0.1313/0.1305 m at stride 2; no accuracy stop condition was triggered.
13. **Next policy:** A per-parent visual lifetime/selector decision is needed
    only if the owner requires runtime visual-state plateau. It is outside
    Prompt20 and was not added automatically.

No other dataset, Native LRU, long-term landmark map, or per-parent selector
was introduced.
