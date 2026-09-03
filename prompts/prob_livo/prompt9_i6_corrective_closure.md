# Prompt 9 — I6 Corrective Closure + I5 Query Cleanup + Normal Online/Offline Regression

## 0. Owner intent

当前 `Prompt8 / I6 Camera-ON` 主链已经证明结构上成立，但 **I6 仍然 OPEN**。本轮不是重做 I6，也不是进入 I8，而是只关闭已经被 Owner/Reviewer 明确定位的 corrective items：

1. 修正 H1 visual probabilistic 3σ gate 的 QR plane residual covariance Jacobian；
2. 把 `geometry_valid` 与 `uncertainty_valid` 从接口到生产行为真正拆开；
3. 移除 visual gate hot path 内重复的 covariance eigensolver 防御性检查；
4. 收掉 I5 Provider 每次 query 为 support radius 重算 3×3 support covariance eigensolver 的额外开销，**但禁止引入 cache**，保持 HKNN → QR 查询路径简单、无跨 query 状态；
5. 用修正后的 H1 重做消融；
6. 额外进行一次 **正常默认在线 ROS 路径 vs canonical offline runner** 的回归，不得再用 `prob_livo_one_callback_step` deterministic shim 伪装成普通在线；
7. 只有上述 hard gates 全部关闭后，I6 才允许 CLOSED。**本轮禁止进入 I8。**

当前已知有效 evidence：

- H0：camera scheduler ON / visual update OFF，ATE `0.091382590 m`；
- H2：FAST visual pipeline + Super legacy visual gate，ATE `0.087931045 m`；
- Prompt8 H1：ATE `0.088624546 m`，但其 probabilistic plane Jacobian 已确认错误，**只能保留为诊断证据，不得作为正式 H1 消融结果**；
- Prompt8 deterministic online one-callback ↔ offline：3979 行、逐字段 0 diff、counters 一致，但该结论只能称为 deterministic one-callback parity，不能外推到默认 `spinOnce()` 在线模式。

---

## 1. Agent State Consensus — 必须先做

在任何代码修改、build、test、run 前：

1. 确认仓库：
   - `/home/lc/super_livo/src/FAST-LIVO2`
2. 确认 branch、HEAD、`origin/<branch>`；
3. 预期起点应包含 Prompt8 closing commit：
   - `af3c82f6`
4. `git status --short` 必须 clean。
5. 若实际 HEAD 与上述 frontier 不一致：
   - 不得自行 reset/rebase/cherry-pick；
   - 记录 `EXPECTED_HEAD` / `ACTUAL_HEAD`；
   - 若只是 Owner 后续提交且历史包含 `af3c82f6`，可继续；
   - 若不包含 Prompt8 frontier，STOP。
6. 输出：
   - starting HEAD
   - branch
   - origin HEAD
   - worktree clean YES/NO
   - `git merge-base --is-ancestor af3c82f6 HEAD` 结果

### Prompt Registration

开始功能修改前，把本 Prompt 原文注册到 canonical history：

- `prompts/prob_livo/` 下新增 Prompt9 对应 markdown；
- 更新 `prompts/README.md`；
- active tracker / issue 若存在，引用该 canonical prompt；
- 最终 evidence 中记录 prompt 文件路径。

---

# 2. Scope freeze

## 2.1 本轮允许修改

只允许围绕以下 seam：

- `VisualPlaneGateInput`
- `EvaluateVisualPlaneGate(...)`
- visual plane provider / `ProbPlaneProvider`
- I5 arbitrary world-point plane query 的 validity 输出
- I5 support-radius 计算路径
- 与上述变更直接相关的 tests / benchmark / evidence / runner 参数记录
- 为“正常在线 vs offline”验证所需的**测试脚本/launch wrapper**，但不得修改算法执行语义来迎合 parity

## 2.2 明确禁止

本轮禁止：

- I8；
- LiDAR P5 re-enable；
- downsample covariance；
- 修改 Super `VoxelGridClosest`；
- 修改 FEJ 设计；
- 修改 visual pyramid / patch / reference update 语义；
- 修改 FAST visual map ownership；
- 新建第二套 filter/state；
- 为 performance 引入 plane cache / support cache / query memoization / persistent radius cache；
- 为了让正常在线与 offline “强行相等”而重写 callback/scheduler；
- 调整 H0/H1/H2 算法参数来追 ATE；
- 因 H1 ATE 结果“不好看”而改 gate threshold；
- 把新的 numerical adaptation 描述成算法创新。

---

# 3. G-P9.1 — Correct QR-native probabilistic visual gate

这是本轮第一 hard gate。

生产 residual 语义：

`r = n^T p_W + d`

QR-native plane parameter：

`theta = [n_x, n_y, n_z, d]^T`

plane covariance：

`Sigma_nd ∈ R^(4×4)`

因此 residual 对 plane 参数的 Jacobian 必须为：

`J_nd = [p_W^T, 1]`

plane uncertainty contribution 必须为：

`var_plane = J_nd * Sigma_nd * J_nd^T`

point contribution 保持当前既定语义：

`var_point = n^T * Sigma_point_W * n`

总 variance 使用当前 H1 已定义的组成，不得顺手引入新的噪声模型：

`var_total = var_plane + var_point (+ 仅现有生产定义中已经存在的项)`

3σ gate 保持 Prompt8 已采用的 strict boundary 语义，不得借 corrective 改 `<` / `<=`：

`|r| < 3 * sqrt(var_total)`

或与现有等价的平方形式。

## Required production change

`VisualPlaneGateInput` 必须显式携带产生该 residual 的 **同一个 world point `point_W`**。

禁止：

- 用 `normal` 代替 `point_W`；
- 用 LiDAR/body-frame point 代替 world point；
- 从 `sensor_range` 反推；
- 从 world-origin norm 反推；
- 在 gate 内重新查询 geometry 来恢复 point；
- 用 production 自己算出的 `total_variance` 构造 oracle。

## Independent oracle test

必须新增一个与 production variance 实现独立的 oracle。

fixture 必须满足：

- `point_W` 不是 unit vector；
- `point_W` 不与 `normal` 平行；
- `Sigma_nd` 非 `cI`；
- 至少含一个非零 cross-term；
- 数值应让 `[p_W^T,1] Sigma [p_W^T,1]^T` 与 `[n^T,1] Sigma [n^T,1]^T` 明显不同。

测试必须直接手算/独立矩阵表达得到 expected `var_plane`、`var_total` 和 3σ boundary，再与 production 比较。

### Required adversarial mutation

至少有一个测试必须证明：

若把 production Jacobian 人为 mutation 回：

`[n^T, 1]`

则测试 **必 FAIL**。

这不是可选测试。

### G-P9.1 PASS 条件

同时满足：

- production 使用 `point_W`；
- independent oracle 数值一致；
- wrong-Jacobian mutation 会失败；
- boundary inside/outside 两侧都测；
- strict `<` 边界行为明确；
- H1 runtime 的 visual gate counters 正常。

否则 G-P9.1 FAIL，禁止跑正式 H1 ATE。

---

# 4. G-P9.2 — 真正拆分 geometry_valid / uncertainty_valid

Prompt8 的“两个 bool”只是在 consumer 端形式上拆开，但 provider 仍可能在 covariance invalid 时直接 `return false`，这不算语义拆分。

## Authoritative invariant

一个 HKNN/QR support 可能满足：

- plane geometry 可用于 raycast / projection；
- 但 QR/P3 covariance 不可用于 probabilistic gate。

这时必须得到：

- `geometry_valid = true`
- `uncertainty_valid = false`

而不是整个 query invalid。

## Provider contract

Provider 结果必须能独立表达至少：

- plane center / centroid
- normal
- geometry_valid
- covariance（若有）
- uncertainty_valid

### Geometry validity

只由“是否存在满足当前 visual geometric query 要求的 plane geometry”决定。

不得因为：

- covariance non-finite
- covariance ill-conditioned
- covariance PSD oracle fail
- probabilistic uncertainty unavailable

而把一个本来可 raycast 的 plane geometry 改成 invalid。

### Uncertainty validity

只表示该 plane covariance 是否满足 H1 probabilistic gate 的 production 使用条件。

## Consumer semantics

- H2 Super legacy visual gate：
  - 只要求 geometry 有效；
  - 不得因为 `uncertainty_valid=false` 丢掉本可使用的 legacy geometry。
- H1 probabilistic 3σ：
  - geometry invalid → reject/no plane；
  - geometry valid 但 uncertainty invalid → **probabilistic second gate fail closed**；
  - 禁止静默 fallback 到 legacy gate，除非 Owner 另行授权。
- raycast / projection：
  - geometry valid 即可继续，不得强依赖 covariance。

## Required adversarial test

构造：

- geometry 明确有效；
- covariance 人工置为 invalid/non-finite 或由 covariance builder 返回 invalid。

必须证明：

- provider 返回 `geometry_valid=true`
- `uncertainty_valid=false`
- H2 geometry path 仍可消费该 plane
- H1 probabilistic gate 不接受该 uncertainty

### G-P9.2 PASS 条件

必须是 production seam test，而不仅是纯 struct unit test。

---

# 5. G-P9.3 — 删除 visual gate 重复 eigensolver hot path

当前 H1 gate 若每次调用都对：

- plane covariance
- point covariance

调用 `SelfAdjointEigenSolver`

做 PSD 防御性检查，属于重复 production hot-path validation。

本轮删除该重复行为。

## Production policy

visual gate 只信任上游显式 validity：

- provider 给出的 `uncertainty_valid`
- VisualPoint / point covariance 已有的 `covariance_valid_` 或对应 canonical flag

gate 内允许的轻量检查：

- 输入 scalar / matrix 元素 `allFinite()`，若当前接口确有必要；
- `var_total` finite；
- `var_total > 0` 或当前 canonical 最小合法条件。

gate 内禁止：

- 每 query `SelfAdjointEigenSolver`
- 每 query LDLT/eigen PSD oracle
- 为了“更安全”再做完整 covariance spectrum validation

严格 PSD / adversarial matrix 验证留在：

- covariance producer test
- unit test
- oracle test

## Required evidence

提供 grep/static evidence，证明 H1 visual gate production call chain 中不再发生该重复 eigensolver。

同时现有 covariance validity tests 必须继续覆盖非法 covariance。

---

# 6. G-P9.4 — I5 support-radius eigensolver cleanup，禁止 cache

当前 I5 Provider 每次 arbitrary plane query 对同一个 HKNN support 额外构造一次 3×3 support covariance 并跑 eigensolver，只为了取得 query/plane radius。

现有 evidence：

- 1000 次 query ≈ `2.76 ms`

因此这不是性能危机，本轮目标只是删除明显重复工作，**不是设计缓存系统**。

## Hard constraints

1. 禁止新增：
   - global cache
   - per-voxel persistent cache
   - support hash cache
   - radius memoization
   - generation/version invalidation machinery
2. 保持：
   - HKNN query authority 不变；
   - QR plane fit authority 不变；
   - 每次 query 根据当前 support 得到当前结果；
   - 无跨 query hidden state。
3. 不得为了省一个 eigensolver 引入比原方案更复杂、更难验证的长期状态。

## Required implementation direction

先审计 radius 的**真实语义和消费者**，然后把 radius 作为同一次 support/plane construction 中的轻量派生量产生，避免“为了 radius 再构造 covariance + eigensolver”的第二条统计路径。

优先级：

1. 若 support/QR 构造过程中已经存在可直接得到、且与生产所需 radius 语义等价的几何量，直接复用；
2. 若 radius 本质上只是 support spatial extent，则在已有 support traversal 中直接累计所需 extent，禁止额外 covariance eigensolver；
3. 若现有 radius 的数学定义确实严格依赖 `lambda_max(Sigma_support)`，不得偷偷改语义。此时：
   - 先寻找同一 plane construction 中已有的等价谱量；
   - 若没有 exact/simple equivalent，保留旧数学定义并在报告中明确说明“无法在不改变语义的前提下删除”，**不得为了完成任务做近似替换**。

也就是说：**目标是删重复 eigensolver；正确性优先于强行优化。**

## Required parity test

针对固定 HKNN support fixture：

- old/reference radius semantic
- new radius
- downstream geometry/raycast decision

必须证明语义一致。

若采用数学等价变换：
- 给出公式；
- numeric tolerance 必须接近 machine precision，而不是宽松工程阈值。

## Performance evidence

沿用同一 1000-query microbenchmark，记录：

- before（可引用 Prompt8 2.76 ms，但最好同机同 build 再测 reference）
- after
- query count
- build type / relevant config

**不设“必须快多少”的 KPI。**

PASS 条件是：

- 无新 cache；
- 无额外 persistent state；
- 不出现明显 regression；
- 代码路径更简单或至少不更复杂；
- correctness/parity 先闭环。

---

# 7. G-P9.5 — Corrected H1 ablation rerun

只有 G-P9.1～G-P9.4 全 PASS 后，才运行正式 corrected H1。

## H1 definition

保持 Prompt8 H1 其余配置不变：

- camera scheduler ON
- FAST visual pipeline ON
- probabilistic visual gate ON
- corrected QR-native `[p_W^T,1]` plane covariance propagation
- LiDAR P5 remains OFF
- same eee_01 bag
- same evaluator
- same visual pyramid/map/reference semantics
- no tuning

必须保存 effective ROS params。

## Result interpretation

新 H1 ATE 才是 canonical probabilistic visual-gate ablation result。

旧：

`0.088624546 m`

必须在 evidence 中明确标注：

`INVALID FOR ABLATION — wrong plane Jacobian`

不得覆盖/删除旧证据。

最终只允许比较：

- H0 `0.091382590 m`
- corrected H1 `NEW`
- H2 `0.087931045 m`

在 corrected H1 出来前禁止写：

- H2 > H1
- legacy gate better than probabilistic gate
- probabilistic gate improves/worsens H2

---

# 8. G-P9.6 — H0 / H2 rerun policy

H0、H2 不默认重跑。

先做 code-path inactivity proof：

## H0

证明 corrective code 在 visual-update-OFF 模式下不进入/不影响 estimator output。

若可证明：
- 保留 Prompt8 H0 evidence。

若不能证明：
- 重跑一次 H0。

## H2

证明：

- corrected H1 probabilistic Jacobian path
- uncertainty fail-closed path
- visual probabilistic gate internals

在 H2 `Super legacy visual gate` 下 byte-inactive。

I5 provider radius cleanup 若可能改变 H2 geometry query，则 **H2 必须重跑**；不能仅靠“这是 performance cleanup”跳过。

因此 H2 是否重跑由实际 diff 决定：

- 若 I5 radius change 对 H2 geometry/raycast 完全无行为影响且有 seam test 证明，可保留；
- 否则重跑 H2。

---

# 9. G-P9.7 — 正常在线 ROS 模式 vs offline runner 回归

这是本轮新增 hard gate。

Prompt8 已证明：

`offline record-by-record == deterministic online one-callback mode`

但这不是普通生产在线模式。

本轮必须再测一次 **默认正常在线路径**。

## Online run — 必须满足

使用真实 ROS 在线执行路径，例如 canonical launch/node + `rosbag play`：

- 使用正常 subscriber/callback queue；
- 使用正常 `ros::spinOnce()` / 当前默认 production event handling；
- `/common/prob_livo_one_callback_step` 必须：
  - unset，或
  - 显式 `false`
- 禁止：
  - offline bag reader 直接调用 callbacks
  - offline runner 内嵌 online node
  - `callOne()` deterministic shim
  - 为 parity 临时改 scheduler
  - 单线程 record-by-record callback 注入来冒充在线

必须在 evidence 中记录：

- launch command
- rosbag play command
- relevant ROS params
- `prob_livo_one_callback_step=false/unset`
- node PID/PGID
- trajectory path
- counters path

## Offline run

同一 commit、同一 bag、同一算法 params，用 canonical offline runner 跑 corrected H1。

## Comparison order

第一层：事件完整性

必须比较：

- trajectory row count
- timestamps
- visual calls
- photometric commits
- plane queries
- reference updates
- key estimator counters

不得只比 ATE。

第二层：逐字段 trajectory comparison

先做 exact comparison：

- identical rows
- identical timestamp sequence
- zero field diffs
- SHA

### Acceptance rule

如果默认 normal-online 与 offline 能做到 exact：
- 标 `NORMAL ONLINE/OFFLINE EXACT PARITY = PASS`

如果不能 exact：
- **禁止用 deterministic shim 重跑来替代**
- **禁止为了制造 exact parity 修改 scheduler**
- 保存 first divergent row / field / magnitude；
- 检查是否存在：
  - callback batching/order
  - queue timing
  - dropped/duplicated message
  - camera/lidar synchronization epoch difference
  - floating reduction/order difference
- 若存在任何 epoch/message/counter 不一致：
  - G-P9.7 FAIL，STOP FOR OWNER。
- 若仅有浮点数值差异、事件与 counters 完全相同：
  - 报告 exact parity FAIL；
  - 同时给出 max/mean pose delta 与 ATE delta；
  - **不要自行放宽 hard gate，也不要自行将其写成 PASS**；
  - STOP FOR OWNER 由 Owner 决定是否接受 numerical parity。

本轮的目的就是获得真实 normal-online evidence，而不是预设它一定 byte-identical。

---

# 10. Required tests

至少新增/更新以下测试组。

## T1 — Correct J_nd oracle

非各向同性 `Sigma_nd` + 非平行 `p_W,n`，验证：

`J=[p_W^T,1]`

数值正确。

## T2 — Wrong-Jacobian mutation killer

把 J 临时/测试 helper mutation 为 `[n^T,1]`，必须失败。

## T3 — 3σ strict boundary

测试：

- just inside → pass
- exactly boundary → 按当前 strict `<` 应 fail
- just outside → fail

boundary 必须来自 independent oracle，不得来自 production `decision.total_variance`。

## T4 — Geometry valid / uncertainty invalid

真实 provider seam：
- geometry valid
- uncertainty invalid

验证 H2 仍获得 geometry，H1 fail closed。

## T5 — Visual gate no hot-path eigensolver

静态或 instrumentation test，证明一次/多次 H1 gate 不进入 `SelfAdjointEigenSolver` covariance validation path。

## T6 — I5 radius semantic parity

固定 support 下 old/reference 与 new radius/下游 query 行为一致。

## T7 — I5 no-cache invariant

代码审计 + test/grep，证明没有新增 persistent query cache/memoization。

## T8 — H2 legacy invariant

验证公式仍为：

`sensor_range > 81 * residual^2`

其中 `sensor_range` 必须是当前 world point 经当前 IMU pose 和 LiDAR extrinsic 还原到 LiDAR frame 后的 norm。

禁止 world-origin norm。

## T9 — Existing I1–I6 regression

全量已有 I1–I6 tests 必须继续 PASS。

## T10 — Online mode identity guard

测试/证据必须能明确证明 normal online run 没有启用 one-callback mode。

---

# 11. Build / execution hygiene

继承既有 spinner-safe contract：

- 一个 shell invocation 只执行一个 bounded build/test/experiment；
- `set -o pipefail`；
- `tee` 时保存真实 return code；
- 每个长期命令打印明确 completion sentinel：
  - `__RC=<code>__`
- UI spinner 不等于 process still running；
- 失败/中断后先用 `pgrep` / PID / PGID 确认真实进程状态；
- 不得因为 UI 仍 spinning 就重复启动同一 bag/test；
- 每次 rosbag/node run 前检查同名 ROS node / roslaunch / rosbag play；
- 每次 run 后清理自己的 PGID；
- 不误杀无关用户进程。

## Diagnostics policy

production-like run：

- heavy diagnostics 默认 OFF；
- 不因为“想更保险”打开 per-point/per-query dumps；
- 先看 trajectory/counters；
- 只有异常时，按 hypothesis 开最小 instrumentation；
- eigensolver oracle、sanitizer、FD 等不要常驻正式跑包。

---

# 12. Performance policy

I5 1000-query ≈ 2.76 ms 已说明没有性能警报。

因此本轮：

- 不做 cache；
- 不做复杂 memoization；
- 不做提前优化；
- 不以 benchmark 数字驱动架构改变；
- 只收掉确定的重复 eigensolver/重复统计工作；
- 如果 exact semantic cleanup 不成立，宁可保留简单旧实现并明确报告，也不要引入隐式近似。

---

# 13. Evidence requirements

最终新增：

`spec/prob_livo/PROMPT9_EVIDENCE.md`

至少包含：

1. starting/final HEAD；
2. branch / origin / clean status；
3. Prompt registration；
4. production code diff summary；
5. G-P9.1～G-P9.7 每 gate：
   - semantic invariant
   - authoritative production path
   - test/evidence
   - adversarial test
   - PASS/FAIL
6. corrected H1：
   - row count
   - counters
   - ATE
   - effective params
7. H0/H2：
   - reused or rerun
   - reuse 时给出 byte-inactive/seam proof
8. normal online run：
   - exact command
   - default callback mode evidence
   - outputs
9. offline run：
   - exact command
   - outputs
10. normal-online vs offline：
   - row/timestamp/counter comparison
   - exact SHA / field diff
   - first divergence if any
11. I5 benchmark：
   - before/after
   - 1000 query
   - no-cache statement
12. old Prompt8 H1 明确标红：
   - invalid for probabilistic ablation
13. final H0/H1/H2 table；
14. architecture deviations；
15. numerical/determinism adaptations；
16. large-file audit；
17. final git status。

---

# 14. Commit / push contract

只有在所有代码和 test 完成后再提交。

建议逻辑拆分：

1. `fix(prob-livo): correct visual probabilistic plane gate`
2. `refactor(prob-livo): separate visual geometry and uncertainty validity`
3. `perf(prob-livo): remove redundant visual query eigensolvers`
4. `test(prob-livo): add normal online offline regression evidence`
5. `docs(prob-livo): close prompt9 i6 corrective evidence`

可根据实际 diff 合并，但禁止把大型运行日志/trajectory/bag/build 产物提交。

提交前：

- `git status --short`
- `git diff --stat <START>..HEAD`
- 检查 >10 MB tracked files
- 检查 rosbag / node.log / trajectory / build / devel / tmp evidence 是否误提交

push 后：

- local HEAD == origin HEAD
- worktree clean

---

# 15. STOP conditions

以下任一情况发生，立即 STOP FOR OWNER，不要自行扩大设计：

1. corrected H1 需要修改 FAST visual map ownership；
2. 正确 `J_nd` 需要改变 residual 定义本身；
3. geometry/uncertainty split 需要重构 I5 authority；
4. I5 radius 若不跑 eigensolver就只能靠近似定义，而无法保持现有语义；
5. normal online 出现 dropped/duplicated/reordered semantic epochs；
6. normal online vs offline counters 不一致；
7. 为了 exact parity 必须修改 scheduler；
8. H2 因 corrective 意外改变 legacy gate 公式；
9. LiDAR P5 被意外打开；
10. 任何需要进入 I8 的修改。

不要“先修再报告”。先保存证据并 STOP。

---

# 16. Final report format

最终回复严格按以下顺序：

## Prompt 9 Final Report

- Initial HEAD:
- Final HEAD:
- origin HEAD:
- worktree clean:
- Prompt registered:
- Architecture deviations:
- Numerical/determinism adaptations:

### G-P9.1 Correct QR gate
- production formula:
- point_W source:
- independent oracle:
- wrong-J mutation:
- result:

### G-P9.2 Geometry / uncertainty split
- provider behavior:
- H1 behavior:
- H2 behavior:
- adversarial invalid-cov case:
- result:

### G-P9.3 Visual hot-path eigensolver
- before:
- after:
- evidence:
- result:

### G-P9.4 I5 radius cleanup
- original radius source:
- new source:
- semantic parity:
- cache added: NO
- 1000-query before/after:
- result:

### G-P9.5 Corrected H1
- rows:
- visual calls:
- photometric commits:
- plane queries:
- reference updates:
- ATE:
- result:

### G-P9.6 H0/H2 preservation
- H0 reused/rerun:
- proof/result:
- H2 reused/rerun:
- proof/result:

### G-P9.7 Normal online vs offline
- normal-online callback mode:
- one_callback_step:
- online rows/counters/ATE:
- offline rows/counters/ATE:
- timestamp equality:
- counter equality:
- field diff:
- SHA:
- first divergence if any:
- result:

### I1–I6 regression
- build:
- tests:
- result:

### Canonical ablation
| Variant | Meaning | ATE | Validity |
|---|---|---:|---|
| H0 | scheduler ON / visual OFF | 0.091382590 | valid |
| H1-old | wrong QR Jacobian | 0.088624546 | INVALID FOR ABLATION |
| H1-corrected | correct probabilistic 3σ | ... | ... |
| H2 | Super legacy visual gate | 0.087931045 or rerun | valid if preserved/rerun |

### Final I6 decision
只能是以下之一：

- `I6 CLOSED`：所有 hard gates PASS，corrected H1 有效，normal-online/offline gate 按本 Prompt 的 exact 标准通过；
- `I6 CORRECTIVE CODE COMPLETE / OWNER DECISION REQUIRED`：仅 normal-online 存在纯 numerical 非 exact 差异，但 epochs/counters 完全一致；
- `I6 OPEN / FAILED`：任何数学、接口、语义或事件级 hard gate 未通过。

**禁止进入 I8。**
