# Prompt 10 — Dead-Code Hygiene Closure + eee_01 Native-LIO vs Prob-LIO ATE & Memory Characterization

## 0. Owner intent

I6 已在 Prompt9 正式 CLOSED。本轮 **不是 I8，也不是视觉对比轮**。

本轮只做两件事，必须按顺序完成：

### Phase A — hygiene cleanup
清理 I1–I6 / Prompt9 过程中已经失去生产用途、且能够被严格证明为 dead / obsolete 的 helper 或兼容性残留。

已知第一候选：

- `src/prob_livo/visual_plane_gate.cpp`
  - `IsFinitePositiveSemidefinite(...)`
  - 其内部仍包含 `SelfAdjointEigenSolver`
  - Prompt9 后 `EvaluateVisualPlaneGate()` 已不再调用它

但本轮不能只删这一处。需要在 **Prob-LIO 新增/修改的局部生产 seam** 中做一次有限范围 dead-helper audit，把同类“以前有用途、当前已经零调用、也不是 API contract”的 helper 一并清理。

**禁止全仓库大扫除。**

所有 cleanup 完成后，必须先做轨迹回归，证明 cleanup 不改变已有 canonical 轨迹。回归未关闭前，禁止开始 native-LIO vs H0 benchmark。

### Phase B — pure-LIO characterization on eee_01
在 NTU VIRAL `eee_01` 上比较：

1. **N0 — FAST-LIVO2 native LIO**
   - 使用本项目创建 `prob-livo` 时的原生 host baseline：
     `0d2c0346107b75b59934975adec9a6eeeb913c64`
   - 原生 FAST-LIVO2 LIO authority
   - camera/VIO OFF
   - 不包含 Prob-LIO/Super backend

2. **H0 — current Prob-LIO canonical H0**
   - 当前 `prob-livo`
   - camera scheduler ON
   - visual update OFF
   - canonical H0 参考：
     - ATE `0.09138258970792523 m`
     - trajectory SHA256
       `9f9eae6fe119d23260d23c4c10a0fba900ba32798d0a58239861432a11d3c53f`

3. **H0-LIOONLY — diagnostic fair-memory control**
   - 当前 Prob-LIO LIO backend
   - camera ingestion/scheduler OFF
   - visual OFF
   - 仅用于将 H0 的 camera scheduler / image-buffer 常量开销与 LIO/map 内存增长拆开
   - **不是新的 canonical algorithm variant**
   - 不得覆盖或重定义 H0

本轮主要输出：

- N0 native-LIO ATE
- H0 ATE 回归
- N0 vs H0 process memory
- N0 vs H0-LIOONLY 的更公平纯-LIO memory
- memory growth curve
- 关键结构/地图规模解释

**本轮结束后 STOP FOR OWNER。不得自动进入 LIVO/H1/H2 对比。**

---

# 1. Agent State Consensus

任何修改前先确认：

Repository:
`/home/lc/super_livo/src/FAST-LIVO2`

Expected branch:
`prob-livo`

Expected starting frontier:
`9802d8f2222b9c4e95c5e0a4129ee70d056e7457`

必须输出：

- branch
- HEAD
- `origin/prob-livo`
- worktree status
- `git merge-base --is-ancestor 9802d8f HEAD`
- `git status --short`

规则：

- 若 HEAD == expected：继续；
- 若 HEAD 是 expected 的 descendant 且 worktree clean：记录 deviation 后继续；
- 若不包含 expected frontier：STOP；
- 不得自行 reset/rebase/cherry-pick。

同时核对项目 SPEC 中 host baseline：

`0d2c0346107b75b59934975adec9a6eeeb913c64`

必须确认该 commit 本地可解析，并确认它是 `prob-livo` 创建时记录的 FAST-LIVO2 host baseline。

---

# 2. Prompt Registration

功能修改前注册本 Prompt：

- `prompts/prob_livo/prompt10_native_lio_memory.md`
- 更新 `prompts/README.md`
- active tracker / project status 若存在则引用 Prompt10
- 最终 evidence 记录 prompt SHA256

Evidence：

`spec/prob_livo/PROMPT10_EVIDENCE.md`

---

# 3. Hard scope freeze

## 3.1 本轮允许

- 删除严格证明 dead/obsolete 的 Prob-LIO helper
- 与 cleanup 直接相关的 test cleanup
- 新增 **外部/工具级** memory monitor
- 新增 native baseline runner wrapper
- 新增 experiment/evaluation scripts
- 新增只读 map/count summary，如果已有 API 可轻量取得
- config overlay 仅用于：
  - camera on/off
  - visual on/off
  - output/PCD/image/debug publishing hygiene
  - memory measurement hygiene
- separate native worktree / build space
- evidence/docs

## 3.2 本轮禁止

- I8
- H1/H2 算法继续开发
- P5
- downsample covariance
- 参数 sweep
- 为了追 ATE 调参数
- 为了追 memory 改数据结构
- cache / memoization
- 新压缩结构
- allocator optimization
- map pruning 策略改变
- 改 local-map sliding
- 改 voxel size
- 改 point filter 策略
- 改 IMU noise
- 改 iteration count
- 改 association gate
- 改 estimator equations
- 修改 native baseline 源码来“适配”当前 Prob runner
- 用当前 upstream/main 替代 `0d2c034`
- 直接在主 worktree checkout native baseline
- 为了让 H0-LIOONLY 等于 H0 而改 scheduler
- 进入 LIVO comparison

---

# 4. Phase A — Dead / obsolete helper cleanup

## G-P10.A1 — bounded dead-helper audit

审计范围限定为：

- `src/prob_livo/**`
- `include/prob_livo/**`
- I1–I6 为 Prob-LIO integration 新增的直接 adapter/helper
- Prompt8/Prompt9 修改过的 visual-plane-provider / gate seam

不要审 FAST-LIVO2 全仓库历史代码。

### 第一已知候选

`IsFinitePositiveSemidefinite(...)`

当前 Prompt9 语义已经改为：

- producer/upstream validity
- `allFinite()`
- scalar variance finite/non-negative

因此若该 helper 已无任何 production/test caller，也不是 header/API contract，则应删除。

### 删除判据

一个 helper 只有同时满足以下条件才能删：

1. definition 可定位；
2. `rg`/symbol audit 显示零 live caller；
3. 不是 virtual/interface requirement；
4. 不是 external/public API；
5. 不是 test oracle 当前仍需要的独立实现；
6. 删除后不需要用另一段等价 dead code 替换；
7. 不改变 production control flow。

### 对每个 candidate 记录

| Helper | Definition | Current callers | Historical purpose | Current status | Action |
|---|---|---:|---|---|---|
| ... | ... | 0 | ... | dead | remove |
| ... | ... | >0 | ... | live | keep |

### Forbidden cleanup

禁止因为“看起来可以简化”而：

- 合并 live branches
- 改数学表达
- 改浮点运算顺序
- 重构 provider/gate
- 改 struct layout
- 删除 legacy alias 但仍有 caller
- 修改 public ABI

若无法明确证明 dead：
**KEEP，记录即可。**

---

# 5. G-P10.A2 — cleanup compile/test gate

cleanup commit 前后必须：

- full build PASS
- I1
- I2
- I3
- P4
- I4
- I5
- I6 visual gate

全部 PASS。

必须 grep/static evidence：

- 被删除 helper 不再存在
- 没有 dangling declaration
- 没有新增 warning/error
- cleanup diff 不包含算法参数和运行语义改变

---

# 6. G-P10.A3 — cleanup trajectory zero-effect regression

这是 Phase A 的最重要 hard gate。

仅 unit tests 不够。

## H1 corrected regression

使用 Prompt9 corrected H1 canonical offline configuration，完整 eee_01。

Expected:

- rows: `3979`
- ATE:
  `0.08795092773331592 m`
- trajectory SHA256:
  `9939bb6bc4688d9685cc156e6a473e2f7413e79a15585b8f72aba7170184f53a`

还必须比较：

- authority counters
- visual counters
- timestamps
- trajectory bytes

要求：

`byte-identical`

## H2 regression

若 cleanup 触及：

- shared provider
- shared visual gate source file
- legacy helper/include
- I5 provider interfaces

则必须完整重跑 H2。

Expected:

- rows: `3979`
- ATE:
  `0.08793104514326024 m`
- trajectory SHA256:
  `812d1bb9de1abeba0471632fa7515ee7afff682cbcad87bbab2786b109f5f6ca`

要求 byte-identical。

如果 cleanup 仅删除一个严格 H1-private dead function且能给出编译/符号证据 H2 无依赖，可不跑 H2；但默认倾向重跑，因为 Owner 要求 cleanup 后确认轨迹无影响。

## Failure rule

只要 cleanup 后：

- SHA 改变
- counters 改变
- rows 改变
- timestamp 改变

即：

`G-P10.A3 FAIL`

不要接受“ATE 几乎一样”。

逐 candidate 回退/定位，直到 cleanup 是真正 zero-effect。

**Phase A 未 PASS，禁止进入 Phase B。**

---

# 7. Commit hygiene between phases

Phase A 单独提交，例如：

`refactor(prob-livo): remove obsolete post-i6 helpers`

记录：

- cleanup commit SHA
- `git diff <P9_HEAD>..<CLEANUP_HEAD>`
- changed files
- regression evidence

Phase B benchmark 不允许夹带新的 estimator production 修改。

---

# 8. Phase B — authoritative variant definitions

## N0 — FAST-LIVO2 native LIO

Authority commit：

`0d2c0346107b75b59934975adec9a6eeeb913c64`

这是项目 SPEC 记录的 host baseline，而不是当前 upstream/main。

必须创建 **separate detached worktree**。

规则：

- 主 `prob-livo` worktree 不切 branch
- native worktree detached at exact commit
- native build directory/devel space 与 Prob-LIO 隔离
- 禁止复用 Prob-LIO object files
- native source必须保持 clean
- runner/helper 放 native worktree 外部，或作为 untracked temporary wrapper
- 不 commit 到 native baseline

### Native LIO semantics

使用 baseline 自带的 `config/NTU_VIRAL.yaml` 作为参数来源。

只允许 measurement overlay：

- `common/img_en = false`
- LiDAR/IMU enabled
- PCD save OFF
- image save OFF
- RViz 不启动
- heavy debug OFF
- 非 estimator-essential visualization/publish 可统一关闭，但必须在 N0/H0/H0-LIOONLY 三者采用一致 policy

禁止修改 native LIO equations / map / uncertainty / downsample / noise / iterations。

---

# 9. H0 — canonical current Prob-LIO control

当前 cleanup 后 `prob-livo` HEAD。

保持 Prompt9 canonical H0：

- camera scheduler ON
- visual update OFF
- probabilistic LIO backend active
- LiDAR P5 OFF
- no visual state commits

Expected：

- rows `3979`
- ATE `0.09138258970792523 m`
- SHA:
  `9f9eae6fe119d23260d23c4c10a0fba900ba32798d0a58239861432a11d3c53f`

如果 benchmark H0 轨迹不是这个 identity：

STOP。

---

# 10. H0-LIOONLY — pure-LIO diagnostic control

目的仅是解决 measurement confound：

canonical H0 的 estimator 虽然 visual update OFF，但 camera scheduler / image subscription 仍 ON。

因此增加一个 diagnostic：

- same cleanup HEAD
- same Prob-LIO backend
- camera ingestion OFF
- visual OFF
- LiDAR/IMU only
- no estimator code change

先比较 H0-LIOONLY 与 canonical H0。

### Case A — byte-identical

则：

- H0-LIOONLY 可作为 **pure-LIO memory primary control**
- canonical H0 仍是项目 H0
- H0 vs H0-LIOONLY 的内存差可解释为 camera infrastructure 常量开销

### Case B — not byte-identical

不得修 scheduler 来制造一致。

H0-LIOONLY 仅作为 diagnostic，不得伪称 H0。

---

# 11. Dataset and evaluator freeze

Dataset：

NTU VIRAL `eee_01`

必须记录 bag path / SHA / duration / topic counts。

Expected bag SHA256：

`7ea43946cffdd49c88d993ad3f192a4e90a8f6826eddc2ef1a9d4f5343ca6c17`

若 SHA 不同：STOP。

Evaluator 必须使用此前 canonical：

`ntu_viral_official_ate`

禁止换 evaluator。

---

# 12. Effective-config parity ledger

在跑 N0/H0 前生成 side-by-side ledger。

物理输入/calibration 必须一致；algorithm-native map/filter semantics 不强行伪装相等，也禁止参数 transplant。

必须记录：

- bag
- LiDAR/IMU topics
- extrinsics
- time offsets
- blind
- preprocessing
- IMU noise
- iterations
- voxel/map semantics
- map sliding
- camera
- visual update
- output/debug settings

---

# 13. Memory measurement contract

## 13.1 只测 estimator process

memory target 必须是实际 mapper PID。

不计：

- roscore
- roslaunch parent
- rosbag
- evaluator
- monitor
- RViz

## 13.2 同一 execution mode

三者都用 **正常 ROS online replay**：

- real callbacks
- real `rosbag play`
- no offline runner
- H0 `one_callback_step=false`
- playback rate `1.0x`
- one run at a time

## 13.3 External monitor

不要把 instrumentation 编进 estimator。

每 2 秒读取：

### `/proc/<PID>/status`

- VmRSS
- VmHWM
- VmSize
- RssAnon
- RssFile
- VmSwap

### `/proc/<PID>/smaps_rollup`

- Rss
- Pss
- Private_Clean
- Private_Dirty
- Shared_Clean
- Shared_Dirty
- Swap

定义：

`USS = Private_Clean + Private_Dirty`

保存 CSV。

## 13.4 Required metrics

- warm/early RSS
- final RSS
- peak RSS
- VmHWM
- peak/final PSS
- peak/final USS
- RSS/PSS/USS growth

Early baseline 使用 estimator 初始化完成、mapping 已开始后的固定稳定窗口，优先取 elapsed `20–40 s` median。

## 13.5 Memory curve

报告：

- 10%
- 25%
- 50%
- 75%
- 90%
- end
- peak

对应 RSS/PSS/USS。

目的：区分固定进程开销与 map growth。

---

# 14. Repetition / reproducibility

至少：

- N0 ×2
- H0 ×2
- H0-LIOONLY ×2

交错执行，不能按 variant 成批跑完。

同 variant Peak RSS/PSS/USS：

- spread ≤3%：接受
- >3%：第三次
- 三次仍 >5%：`MEMORY_ENVIRONMENT_NOISY`

ATE/trajectory run-to-run 不一致：STOP。

---

# 15. Memory comparison

同时计算：

### Canonical system
`N0 vs H0`

### Pure-LIO
如果 H0-LIOONLY 与 H0 trajectory-equivalent：

`N0 vs H0-LIOONLY`

### Camera infrastructure
`H0 vs H0-LIOONLY`

计算：

- peak RSS delta/%
- peak PSS delta/%
- peak USS delta/%
- final memory
- growth memory

---

# 16. Interpretation bands

仅描述，不是 PASS threshold：

- saving ≥20%：LARGE
- 10–20%：MODERATE
- 3–10%：SMALL
- |diff| <3%：TIE
- H0 >3% more：REGRESSION

禁止因结果不符合预期而调参。

---

# 17. Structural memory explanation

补充轻量：

- relevant native types `sizeof`
- Prob types `sizeof`
- existing map point/node/plane counts if readily available

如果 count 需要 intrusive instrumentation：
不加。

明确写 unavailable。

若双方有可靠 point count，可附 USS/point，但不能把 allocator/node overhead 伪装成严格 per-point cost。

---

# 18. ATE comparison

最终至少：

| Variant | LIO authority | Camera | Visual | Rows | GT | ATE |
|---|---|---|---|---:|---:|---:|
| N0 | FAST-LIVO2 native | OFF | OFF | | | |
| H0 | Prob-LIO P0–P4 | ON scheduler | OFF | | | |
| H0-LIOONLY | Prob-LIO P0–P4 | OFF | OFF | | | |

计算 H0 vs N0 ATE absolute/relative delta。

不规定谁必须更准。

---

# 19. Confounds

最终前检查：

- same bag
- same calibration
- same evaluator
- no PCD/RViz
- same playback rate
- no duplicate mapper/bag
- same build type/compiler
- complete trajectory
- no backlog
- thread/runtime environment recorded
- native-specific local map policy如果和 Prob 不同，要明确标记为 system semantic difference，不能偷偷统一

---

# 20. Build-type parity

尽可能相同：

- compiler
- Release/optimization
- Eigen/TBB/OpenMP environment
- ROS distro
- machine

不同则 benchmark invalid。

---

# 21. Execution hygiene

继承现有 spinner-safe contract：

- one bounded operation per shell
- pipefail
- true RC
- completion sentinel
- process existence before rerun
- no duplicate bag/mapper
- own PGID cleanup
- no global pkill
- heavy diagnostics OFF

---

# 22. Hard gates

- A1 dead helper strict proof
- A2 build/tests
- A3 H1/H2 cleanup trajectory byte parity
- B1 native exact `0d2c034`
- B2 dataset/calibration/evaluator authority
- B3 H0 canonical identity
- B4 estimator-only memory PID
- B5 ≥2 valid runs
- B6 trajectory reproducibility
- B7 H0-LIOONLY role determined by parity
- B8 no tuning based on result

Fail → STOP FOR OWNER.

---

# 23. Artifacts

建议：

- `tools/prob_livo/memory_monitor.py`
- `tools/prob_livo/run_eee01_native_lio.sh`
- `tools/prob_livo/compare_memory_runs.py`

monitor 必须外部只读 `/proc`。

---

# 24. Evidence

`spec/prob_livo/PROMPT10_EVIDENCE.md`

必须包含：

- state consensus
- dead-helper audit
- cleanup regression
- native authority
- config ledger
- ATE table
- memory methodology
- per-run memory
- aggregate comparison
- memory curve
- structural explanation
- confound audit
- final interpretation

---

# 25. Final status

只能：

`PROMPT10 CHARACTERIZATION CLOSED`

或

`PROMPT10 OWNER DECISION REQUIRED`

或

`PROMPT10 FAILED`

本轮结束后：

**STOP。Do not enter LIVO/H1/H2/I8.**
