# Prob-LIVO Integration — Prompt 3
## Close I2 Lifecycle Seam + I3 Full Prob-LIO P0–P4 Backend + NTU eee_01 Camera-OFF Baseline

> 本轮严格分两部分：
>
> **Part A：**关闭 I2 剩余的生产生命周期 seam。
>
> **Part B：**完成 I3，使 FAST-LIVO2 外壳下的 Prob-LIO 在 camera OFF 时可以真实完整跑 LIO，并在 NTU `eee_01` 上与之前 canonical Prob-LIO 的轨迹和 ATE 做直接比较。
>
> 本轮禁止进入视觉 I4+。

---

# 0. Owner Architecture Freeze

当前状态：

```text
I0 = CLOSED / OWNER VERIFIED
I1 = CLOSED / OWNER VERIFIED

I2:
  core IMU semantics        = GREEN
  endpoint/undistortion     = GREEN
  scheduler runtime seam    = PARTIAL
  init→runtime handoff      = CORRECTIVE REQUIRED
  Owner Verified            = NO

I3 = BLOCKED until Part A closes
I4–I8 = NOT STARTED
```

架构冻结：

```text
HOST authority   = FAST-LIVO2 public source
LIO authority    = canonical Prob-LIO P0–P4
Visual authority = FAST-LIVO2 public source

shared state     = one x19/P19
LiDAR geometry   = Prob OctVox only
visual map       = FAST feat_map
P5               = excluded
```

I3 最终 camera-OFF 链路必须是：

```text
FAST scheduler / ROS shell
        ↓
Super-native IMU init + propagation + undistortion
        ↓
ProbESKF19
        ↓
Super VoxelGridClosest
        ↓
Prob OctVox
        ↓
Super HKNN
        ↓
Super QR plane
        ↓
P1 / P2 / P3 / P4
        ↓
Super legacy association gate
        ↓
ProbESKF19 LiDAR update
        ↓
Prob map update
        ↓
trajectory
```

Camera/VIO 必须关闭。

---

# 1. Production Philosophy — runtime 要轻

Owner 明确要求：**不要堆防御性编程。**

```text
tests 可以严格、对抗性强
production hot path 必须保持轻量
```

生产路径只保留真正会防止静默语义错误的必要检查，例如：
- 不可能的 epoch 顺序；
- frame/authority 明显错位；
- 精确 endpoint 所必需的数据缺失；
- canonical Prob-LIO 本来就要求的 covariance/residual validity。

禁止新增：
- hot path 重型 eigensolver/PSD 检查；
- 大量层层 `if-invalid-return`；
- 重复 runtime validators；
- 每点/每邻居日志；
- 静默 fallback 到另一套算法；
- 为了“安全”复制状态/地图 authority。

需要诊断时优先放在 test；production 仅保留轻量 counters，并默认关闭 heavy diagnostics。

---

# 2. Repository / Frontier Consensus

Active host：

```text
~/super_livo/src/FAST-LIVO2
```

Expected branch：

```text
prob-livo
```

Expected start HEAD：

```text
54a215847c8c429f8ee926ba5ff0017114ef7f02
```

Prob-LIO reference：

```text
~/super_livo/ref/Super-LIO
branch: prob-lio
HEAD: 9fc949f46291c0fa76e5b7cdb372c940eb4b3f6e
```

Legacy evaluator/results source：

```text
~/prob_lio/src/Super-LIO
```

Dataset root：

```text
~/super_livo/bag/NTU
```

Canonical sequence：

```text
eee_01
```

启动先核对：

```bash
cd ~/super_livo/src/FAST-LIVO2
git status --short
git branch -vv
git rev-parse HEAD
git rev-parse origin/prob-livo
git log --oneline -15

cd ~/super_livo/ref/Super-LIO
git status --short
git branch -vv
git rev-parse HEAD
git log --oneline -10
```

要求：
- host clean；
- HEAD 精确等于 Owner frontier，否则先报告；
- reference clean/read-only；
- no force push；
- no history rewrite。

登记本 Prompt：

```text
prompts/prob_livo/prompt3_i2_close_i3_prob_lio_baseline.md
```

---

# 3. Baseline Build Before Edits

已知 build 环境：

```bash
source /home/lc/design_ws/devel/setup.bash
cd /home/lc/super_livo
catkin_make --pkg fast_livo
```

并运行已有 I1/I2 focused tests。

### HARD GATE G-P3.0

必须满足：
- start HEAD exact；
- worktree clean；
- host build PASS；
- I1 tests PASS；
- I2 tests PASS。

异常先分类，不要边坏边改。

---

# PART A — CLOSE I2 PRODUCTION LIFECYCLE

# 4. Remaining I2 Defect

当前 I2 底层数学已接受。

剩余问题是：

```text
scheduler epoch clock
!=
filter temporal state
```

在 `IMU_INIT / MAP_INIT` 阶段不能强制：

```text
filter.current_time == scheduler epoch_start
```

canonical Super 生命周期必须是：

```text
IMU_INIT
   ↓
MAP_INIT
   ↓
RUN
```

MAP_INIT 期间不能错误走正常 RUN propagation/update。

---

# 5. One Explicit Lifecycle Authority

建立一个小而明确的唯一生命周期 authority：

```text
ProbLioLifecycle
{
    IMU_INIT
    MAP_INIT
    RUN
}
```

它只能有一个 owner。

禁止在：
- LIVMapper；
- ProbImuAdapter；
- ProbLioBackend；
- test harness；

各自维护一份相互独立的 lifecycle state。

推荐未来由 integration/backend controller 持有，IMU adapter 与 map backend 只消费该状态。

---

# 6. Separate Two Time Authorities

必须明确区分：

## Scheduler epoch authority

```text
scheduler_epoch_start
scheduler_epoch_end
```

来自 FAST `LidarMeasureGroup` / `last_lio_update_time`，控制：
- packet cut；
- curvature origin；
- current/next LiDAR partition；
- scheduler IMU selection。

## Filter temporal authority

```text
current_time
last_imu_time
last_obs_time
current_obs_time
```

来自 ProbESKF19/Super semantics，控制：
- midpoint Predict；
- observation-boundary clipping；
- RUN 阶段 propagation。

IMU_INIT/MAP_INIT 时不要强制相等；进入 RUN 后必须严格闭合到同一 epoch boundary。

---

# 7. Scheduler Anchor Ownership

每个被成功消费的 LIO epoch，scheduler anchor 必须且只能推进一次：

```text
next scheduler epoch start = current epoch end
```

包括：
- IMU_INIT；
- MAP_INIT；
- RUN。

原因：LIVO current/next point curvature rebasing 依赖这个 anchor。

禁止 scheduler 和 adapter 双方都更新。

必须选一个 production owner 并写进 SPEC。

---

# 8. Canonical Super MAP_INIT Audit

先审 reference 真实生产代码：

```text
SuperLIO stateWaitKFInit
SuperLIO stateWaitMapInit
SuperLIO map_init(...)
SuperLIO stateProcess
```

记录：
- map-init 帧数/条件；
- map-init 是否做正常 undistort；
- 使用什么 pose；
- 原始点如何变换；
- 初始 covariance 如何生成；
- 何时 `SetLastObsTime(end_time)`；
- 何时第一次进入正常 propagation。

不能只按 Prompt 描述实现。

---

# 9. HARD GATE G-I2.C1 — real init→map-init→run continuity

使用**同一个 persistent scheduler/LidarMeasureGroup state** 驱动：

```text
FAST scheduler semantics
→ 多个 IMU_INIT epoch
→ init threshold crossing
→ MAP_INIT epochs
→ first RUN epoch
```

必须证明：
- scheduler anchor 每个成功 epoch 连续推进；
- 不允许 fixture 手工 reset `last_lio_update_time`；
- point-time origin 无跳变；
- IMU interval 不重不漏；
- MAP_INIT 不错误执行 normal propagation；
- `last_obs_time` 按 Super map-init 语义推进；
- first RUN Predict 从正确 observation boundary 开始；
- RUN end 精确等于 scheduler epoch end。

Required negative mutations：
1. init 期间 scheduler anchor 不推进；
2. 强制 `filter.current_time = scheduler time`；
3. MAP_INIT 正常传播 filter；
4. scheduler/adapter 双重推进 anchor；
5. MAP_INIT 不更新 `last_obs_time`。

这些 mutation 必须 RED。

---

# 10. HARD GATE G-I2.C2 — real scheduler IMU-buffer seam

上轮 point rebase 证据比 IMU-buffer seam 更强，本轮补齐。

必须实际 exercise production IMU selection，或抽出一个**真实被 `sync_packages()` 调用**的 helper。

证明：

```text
IMU in (last_lio_update_time, lio_time]
→ current epoch

first IMU > lio_time
→ remains buffered as look-ahead
```

并证明：
- 不重复 pop；
- 不丢 first next IMU；
- 不消费 current end 后的数据；
- 连续多个 epoch 无 gap/duplication。

禁止仅手工构造已经过滤好的 IMU vector 来冒充 production seam 测试。

---

# 11. Close I2

G-I2.C1/C2 GREEN 后：
- rerun all I2 tests；
- full build；
- 单独 commit；
- clean worktree。

Suggested commit：

```text
fix(prob-livo): close init-to-runtime lifecycle handoff
```

SPEC 更新：

```text
I2 = CLOSED / OWNER VERIFIED
I3 = ACTIVE
```

I2 未 clean 之前禁止进入 I3。

---

# PART B — I3 FULL PROB-LIO P0–P4 BACKEND

# 12. I3 Mission

I3 必须做到：

```text
FAST-LIVO2 shell + camera OFF + full Prob-LIO LIO backend
```

可真实完整运行 NTU `eee_01`，并使用旧 Prob-LIO 相同 evaluator 比较：
- trajectory；
- ATE。

这是新 host 第一次 canonical full-bag algorithm run。

---

# 13. Canonical Prob-LIO Source Authority

唯一算法参考：

```text
~/super_livo/ref/Super-LIO
@ 9fc949f46291c0fa76e5b7cdb372c940eb4b3f6e
```

必须逐项审 active production：

```text
map initialization
VoxelGridClosest
OctVox storage/insertion
HKNN
QR plane solve
P1 point covariance
P2 map covariance
P3 QR-plane covariance
Super legacy association
P4 probabilistic weighting
map update
config/defaults
```

P5 明确排除。

---

# 14. Migrate by Semantic Module, Not Whole Tree

继续 COPY-ON-DEMAND。

优先放：

```text
include/prob_livo/
src/prob_livo/
```

禁止复制：
- Super ROS wrapper；
- build/devel；
- P5 lifecycle；
- 大量历史 debug；
- FAST visual 相关代码。

所有 production import 写入 SPEC import ledger：
- source SHA/path；
- destination；
- adaptation；
- production/test-only。

---

# 15. ProbLioBackend Ownership

实现一个清楚的 backend authority，概念上：

```text
ProbLioBackend
```

职责：
- lifecycle；
- map init；
- downsample；
- current scan covariance；
- HKNN/QR association；
- P4 observation callback；
- ProbESKF19 LiDAR update；
- map update；
- trajectory/state output。

输入：
- FAST scheduler epoch；
- ProbImuAdapter output；
- shared `StatesGroup/ProbESKF19`。

拥有：
- Prob OctVox；
- per-scan temporary buffers。

禁止拥有第二份 persistent navigation state/covariance。

---

# 16. Camera-OFF Runtime Mode

I3 canonical eee_01 run 使用：

```text
FAST host shell
+ Prob IMU backend
+ Prob LIO backend
+ camera/VIO OFF
```

为了与旧 standalone Prob-LIO 做最干净比较，I3 canonical baseline **优先使用 FAST host 的 LIO-only scheduling mode**，不要引入 camera-cut packetization 作为额外变量。

记录 exact mode。

视觉不运行。

---

# 17. Map Initialization Parity

必须复现 canonical Super map-init：
- 相同帧数/transition 条件；
- 相同 pose authority；
- 相同点变换；
- 相同 OctVox insertion；
- 相同初始 covariance；
- 相同 `last_obs_time` 更新；
- 相同进入 RUN 的逻辑点。

### HARD GATE G-I3.1

Synthetic/reference parity：
- lifecycle transition count；
- initial map representative/count；
- representative position/covariance；
- final observation boundary。

Negative mutations：
1. map-init 正常 Predict；
2. frame count 改变；
3. 使用 FAST VoxelMap init；
4. 错误顺序插入；
5. RUN 提前/延后一帧。

---

# 18. Downsample Parity

Canonical：

```text
Super VoxelGridClosest
```

不是 FAST PCL centroid VoxelGrid。

### HARD GATE G-I3.2

同一 deterministic cloud：
- selected source-point identity；
- count；
- coordinates/intensity；
- closest-to-voxel-center rule；

必须与 reference 一致。

---

# 19. P1 Point Covariance

迁移 canonical：

```text
p_L
→ Sigma_L
→ p_I
→ Sigma_I
```

严格核实 LiDAR→IMU extrinsic 方向。

### HARD GATE G-I3.3

覆盖：
- 多 range；
- 多 beam direction；
- nonidentity R_LI；
- nonzero t_LI；
- covariance symmetry；
- frame equivalence。

不得混入 P5 current-pose association covariance。

---

# 20. P2 Map Covariance / Compact Storage

迁移 canonical：
- point + covariance storage；
- insertion pose covariance；
- representative aggregation；
- default double storage；
- canonical `livo2_compat` map pose model，除非旧 eee_01 authoritative config 明确不是它。

### HARD GATE G-I3.4

Reference parity：
- first insert；
- repeated aggregation；
- representative position；
- representative covariance；
- accepted count。

不要在本轮“修正/优化” independent-point approximation。

---

# 21. HKNN Parity

### HARD GATE G-I3.5

确定性 OctVox map/query 下比较：
- neighbor identities；
- order；
- count；
- distances；
- covariance pairing。

Prob mode 不允许 FAST neighbor search 参与。

---

# 22. QR Plane Parity

使用 Super QR，不使用 FAST PCA plane。

### HARD GATE G-I3.6

比较：
- q；
- normalized n,d；
- valid/rank；
- residual。

覆盖：
- N=4 full-rank；
- N=5；
- near-degenerate；
- rank-deficient reject。

---

# 23. P3 QR Plane Covariance

迁移 canonical 4×4 `[n,d]` covariance。

### HARD GATE G-I3.7

Reference parity：
- Sigma_nd；
- query point plane residual variance；
- valid/finite classification。

禁止伪造 FAST 6×6 plane covariance。

Production 不增加 heavy eigensolver hot path。

---

# 24. Super Legacy Association

LiDAR association 继续 canonical Super legacy gate。

P5 不进入生产。

### HARD GATE G-I3.8

Deterministic accept/reject parity：
- near plane；
- residual failure；
- range-dependent case。

Static audit：
- P5 disabled/absent；
- no LiDAR kσ gate。

---

# 25. P4 Probabilistic Weighting

Canonical：

```text
R_i =
0.001
+ sigma_QR_plane^2
+ sigma_sensor_point^2

w_i = 1 / R_i
```

当前 pose covariance **不得**进入 final P4 measurement variance。

### HARD GATE G-I3.9

Reference parity：
- plane term；
- sensor-point term；
- final variance；
- weight；
- invalid scalar handling。

只保留 canonical lightweight validation。

---

# 26. LiDAR Observation Lifecycle

I3 measurement producer 必须使用已经修正的 callback：

```text
state
need_converge
HT_Vinv_H
HT_Vinv_r
```

### HARD GATE G-I3.10

真实 backend synthetic seam 对比 reference：
- callback count；
- `[F,F,...,T]`；
- `need_converge` 对 HKNN/QR refresh 的真实控制；
- correspondence count；
- final state/covariance。

禁止重新丢失 `need_converge`。

---

# 27. Map Update Parity

LiDAR posterior 后：
- 用 canonical posterior state 变换 map points；
- 计算 map covariance；
- 更新 Prob OctVox。

### HARD GATE G-I3.11

多 scan reference parity：
- map size/count；
- representative identity/position；
- covariance；
- accepted count。

Prob mode 不更新 FAST VoxelMapManager。

---

# 28. Single LiDAR Geometry Authority

Prob backend selected 时：

```text
Prob OctVox = only active LIO geometry authority
```

FAST `VoxelMapManager` 可以保留源码/对象，但不得：
- StateEstimation；
- UpdateVoxelMap；
- 影响 Prob residual；
- 形成第二张 authoritative LiDAR geometry map。

### HARD GATE G-I3.12

要求 static + runtime evidence：
- Prob run 只走 Prob backend；
- FAST VoxelMapManager estimation/update count = 0 或路径不可达；
- 无 dual-map update。

---

# 29. Runtime Config Provenance

eee_01 运行前必须建立 exact effective config。

优先级：
1. 之前 canonical Prob-LIO eee_01 effective config/evidence；
2. dataset/algorithm official config；
3. 无来源时才用明确 default。

从：

```text
~/prob_lio/src/Super-LIO
```

只 COPY-ON-DEMAND 需要的 evaluator/config/tool。

至少冻结并记录：
- LiDAR topic；
- IMU topic；
- camera/VIO disabled；
- lidar type/preprocess；
- blind/maxrange；
- VoxelGridClosest resolution；
- OctVox params；
- HKNN params；
- QR thresholds；
- IMU noise；
- gravity norm；
- R_LI/t_LI；
- map covariance mode；
- storage precision；
- P4 mode；
- association mode；
- ESKF iteration/convergence。

禁止 sweep/tuning。

---

# 30. NTU eee_01 Topic Audit

正式跑前用 `rosbag info` 或等价 metadata audit：
- exact bag path；
- topics；
- message types；
- camera topic presence；
- LiDAR/IMU/GT topic；
- bag size/hash（可行则记录 SHA256）。

不要再从 calibration 文件推断 bag topic。

---

# 31. Reuse Previous Evaluator — COPY ON DEMAND

定位旧 evaluator：

```text
~/prob_lio/src/Super-LIO/eval/prob_lio
```

确认 old canonical eee_01 真实 evaluator provenance，然后只复制必需文件到：

```text
eval/prob_livo/
```

允许改路径/命名，不允许改 evaluator semantics。

历史 canonical anchor 需要从旧 evidence **重新核实**。当前预期约为：

```text
sequence: eee_01
trajectory rows: 3981
matched GT: 3329
canonical P4 ATE: 0.088831554 m
trajectory SHA prefix historically: 259d3fbc...
```

若 legacy evidence 不同，以 legacy authoritative evidence 为准，并在报告中说明。

对比对象必须是 **canonical P4 Prob-LIO**，不是 fixed-1000 baseline。

---

# 32. Canonical Runner

建立/迁移一个干净 runner 到：

```text
tools/prob_livo/
```

要求：
- bounded single run；
- unique run directory；
- capture real return code；
- git HEAD；
- git dirty state；
- config snapshot；
- bag path/hash；
- backend mode；
- camera OFF；
- trajectory identity/hash；
- no duplicate concurrent rosbag/mapper；
- no dirty canonical run。

canonical 顺序：

```text
modify
→ test
→ commit
→ verify clean
→ full eee_01
→ evaluator
→ trajectory comparison
→ evidence
```

---

# 33. HARD GATE G-I3.13 — end-to-end runtime authority

用轻量 counters/metadata 证明 canonical eee_01 run：

```text
scheduler                = FAST host
IMU backend              = ProbImuAdapter
filter                   = ProbESKF19
map                      = Prob OctVox
association              = Super legacy
measurement weighting    = Prob P4
camera/VIO updates       = 0
FAST VoxelMap estimation = 0
FAST VoxelMap update     = 0
```

禁止 log flood。

---

# 34. HARD GATE G-I3.14 — full eee_01 completion

Canonical run 必须：
- clean source；
- full bag completion；
- no crash；
- no silent early stop；
- no duplicate rosbag；
- no NaN/Inf trajectory。

记录：
- runtime；
- trajectory rows；
- start/end timestamp；
- completion state。

若失败，先诊断 seam/semantic，不调参。

---

# 35. Mandatory Trajectory Comparison with Previous Prob-LIO

定位旧 canonical eee_01 trajectory。

必须做两类比较。

## A. Raw trajectory-to-trajectory comparison

在直接对应 timestamps 上计算：
- row count；
- overlap count；
- timestamp delta mean/max；
- translation difference per pose；
- rotation-angle difference per pose；
- translation diff RMSE/median/max；
- rotation diff RMSE/median/max。

**不要先把两条算法轨迹互相 SE(3) align 后再做主比较。**

可额外做 aligned diagnostic，但不能替代 raw comparison。

## B. Same evaluator ATE comparison

对旧/新轨迹使用完全相同 evaluator semantics。

报告：

```text
ATE_old_prob_lio
ATE_new_prob_livo_host
absolute delta
percent delta
matched_GT_old
matched_GT_new
completion_old/new
```

---

# 36. HARD GATE G-I3.15 — comparison classification

不要为了 PASS 强行要求 byte parity；也不要因为能跑完就接受明显漂移。

分类只能三选一：

### I3_TRAJECTORY_EQUIVALENT
- effective config/input semantics 一致；
- trajectory 差异仅数值级；
- ATE 基本一致。

### I3_TRAJECTORY_CLOSE_NONIDENTICAL
- full completion；
- 未发现 semantic mismatch；
- host/shell/output path 导致可解释的小差异；
- ATE 接近。

### I3_SEMANTIC_MISMATCH
- material trajectory divergence；
- point/correspondence/timing/config semantics 不同；
- ATE 明显异常；
- 或无法解释的系统性差异。

若为 mismatch：

```text
I3 remains OPEN
I4 blocked
```

禁止通过调参把 mismatch 调成 close。

---

# 37. Historical Comparison Anchor

当前项目历史记忆中 canonical previous Prob-LIO `eee_01` 大约为：

```text
P4 canonical ATE = 0.088831554 m
rows = 3981
matched GT = 3329
```

原 fixed-weight baseline 约：

```text
0.118875639 m
```

但最终报告必须引用 legacy repository 的真实 evidence，而不是只引用 Prompt 数字。

---

# 38. HARD GATE G-I3.16 — no tuning / no P5 / no visual contamination

Canonical I3 run 必须：

```text
camera/VIO = OFF
P5 = OFF / absent
association = Super legacy
P4 probabilistic weight = ON
canonical map pose covariance mode
heavy diagnostics = OFF
```

无 sweep。
无根据新 ATE 的 post-hoc parameter adjustment。

---

# 39. Production Cleanliness

I3 结束时要求：
- 无临时实验 backend 分支；
- 无 dual-map authority；
- 无 hot-path 大 debug；
- 无绝对临时路径；
- 无 bag/result 大文件误提交；
- 无 build/devel 提交。

实现以小 adapter + canonical logic 为主，不为“防御性”添加多层框架。

---

# 40. Recommended Tests

```text
tests/prob_livo/test_i2_lifecycle_handoff.cpp
tests/prob_livo/test_i2_scheduler_imu_buffer.cpp

tests/prob_livo/test_i3_map_init.cpp
tests/prob_livo/test_i3_downsample.cpp
tests/prob_livo/test_i3_point_covariance.cpp
tests/prob_livo/test_i3_octvox.cpp
tests/prob_livo/test_i3_hknn.cpp
tests/prob_livo/test_i3_qr_plane.cpp
tests/prob_livo/test_i3_qr_covariance.cpp
tests/prob_livo/test_i3_association_weight.cpp
tests/prob_livo/test_i3_backend_seam.cpp
```

不要一个巨大 test 文件。
禁止 `std::vector<bool>`。

---

# 41. Mandatory Implementation Order

```text
A. baseline build/tests

B. close I2 lifecycle
→ persistent scheduler seam tests
→ commit
→ clean

C. audit canonical Super map-init/P0–P4

D. migrate map-init + OctVox
→ tests

E. VoxelGridClosest
→ tests

F. P1/P2
→ tests

G. HKNN + QR + P3
→ tests

H. legacy association + P4
→ tests

I. wire ProbESKF19 observation callback
→ backend seam parity

J. wire camera-OFF runtime mode
→ prove FAST map inactive

K. full build + full focused tests

L. freeze config/evaluator/runner
→ implementation commit
→ verify clean

M. ONE canonical full eee_01 run

N. same evaluator
→ raw trajectory comparison
→ ATE comparison

O. docs/evidence
→ final docs commit if needed
→ clean push
```

禁止 dirty canonical run。

---

# 42. SPEC Finalization

更新：

```text
spec/prob_livo/SPEC.md
```

若全部关闭：

```text
I0 = CLOSED / OWNER VERIFIED
I1 = CLOSED / OWNER VERIFIED
I2 = CLOSED / OWNER VERIFIED
I3 = CLOSED/PASS — Owner audit pending
I4–I8 = NOT STARTED
```

SPEC 必须记录：
- lifecycle authority；
- scheduler/filter clock separation；
- map-init semantics；
- P0–P4 ownership；
- runtime backend mode；
- eee_01 config provenance；
- old/new trajectory + ATE classification。

---

# 43. EVIDENCE_INDEX

记录：
- start HEAD；
- I2 closure SHA；
- I3 implementation SHA；
- reference SHA；
- imported source provenance；
- all gate evidence；
- config snapshot；
- bag identity/hash；
- evaluator identity；
- old canonical trajectory identity/hash；
- new trajectory identity/hash；
- old/new ATE；
- raw trajectory-difference metrics；
- final HEAD。

---

# 44. HISTORY

记录：
1. 为什么 init/map-init 期间 scheduler clock 与 filter clock 分离；
2. I2 如何关闭；
3. FAST 为什么仍是 scheduler authority；
4. Prob 为什么是完整 LIO authority；
5. P5 为什么继续排除；
6. 第一次真实 baseline 为什么 camera OFF / LIO-only；
7. 新 host trajectory 与旧 Prob-LIO 的最终关系。

---

# 45. Hard Gate Summary

最终逐项汇报：

```text
G-P3.0   baseline build/tests

G-I2.C1  init→map-init→run continuity
G-I2.C2  production IMU-buffer seam

G-I3.1   map-init parity
G-I3.2   VoxelGridClosest parity
G-I3.3   P1 covariance parity
G-I3.4   P2 map covariance parity
G-I3.5   HKNN parity
G-I3.6   QR plane parity
G-I3.7   P3 QR covariance parity
G-I3.8   Super legacy association parity
G-I3.9   P4 weighting parity
G-I3.10  observation lifecycle/backend parity
G-I3.11  map update parity
G-I3.12  single LiDAR geometry authority
G-I3.13  end-to-end runtime authority
G-I3.14  full eee_01 completion
G-I3.15  previous Prob-LIO trajectory/ATE comparison
G-I3.16  no tuning/P5/visual contamination
```

每个 Gate 必须独立给：
- semantic invariant；
- authoritative production/oracle path；
- observable evidence；
- threshold；
- negative mutation（适用时）；
- PASS/FAIL。

不能用一句 “all tests pass” 替代。

---

# 46. Commit Policy

建议：

### Commit A
```text
fix(prob-livo): close init-to-runtime lifecycle handoff
```

### Commit B
```text
feat(prob-livo): migrate canonical p0-p4 lidar backend
```

### Commit C
```text
test(prob-livo): close full lidar backend parity gates
```

### Commit D（canonical run 后文档）
```text
docs(prob-livo): record first camera-off eee01 baseline
```

No force push。
Fast-forward only。
Canonical run 前 worktree clean；最终也 clean。

---

# 47. Final Report Format

## Agent State Consensus
- start HEAD
- branch
- origin
- reference SHA
- clean status
- prompt registration

# PART A — I2 Closure

## Lifecycle Defect
- old assumption
- canonical Super lifecycle
- scheduler/filter time authorities

## I2 Corrective
- lifecycle owner
- scheduler anchor owner
- map-init temporal semantics

## G-I2.C1
- persistent multi-epoch sequence
- exact transition epochs
- negative mutations

## G-I2.C2
- real scheduler IMU buffer seam
- look-ahead preservation
- no duplicate/drop

## I2 Closure Commit

```text
I2 = CLOSED / OWNER VERIFIED
```

# PART B — I3

## Canonical Source Audit
- map-init
- downsample
- OctVox
- HKNN
- QR
- P1/P2/P3/P4
- legacy association

## Imported Production Components
- source SHA/path
- destination
- adaptations

## ProbLioBackend
- ownership
- lifecycle
- state/map authority
- camera-off runtime wiring

## G-I3.1–G-I3.12
完整 parity evidence。

## eee_01 Dataset / Config
- bag path/hash
- topics/types
- config provenance
- effective config
- backend mode
- camera OFF proof

## Canonical Run
- run directory
- algorithm HEAD
- clean status
- completion
- runtime
- trajectory rows/timestamps
- runtime authority counters

## Previous Canonical Prob-LIO
- legacy run/evidence path
- old algorithm SHA
- old trajectory SHA
- old rows/matched GT
- old ATE

## Raw Trajectory Comparison
- overlap rows
- timestamp delta mean/max
- translation diff RMSE/median/max
- rotation diff RMSE/median/max

## ATE Comparison

```text
old canonical Prob-LIO ATE:
new FAST-host Prob-LIO ATE:
absolute delta:
percent delta:
old matched GT:
new matched GT:
```

## Classification
必须唯一：

```text
I3_TRAJECTORY_EQUIVALENT
I3_TRAJECTORY_CLOSE_NONIDENTICAL
I3_SEMANTIC_MISMATCH
```

## G-I3.13–G-I3.16
运行、完成、对比、无污染。

## Scope Audit
确认：
- camera/VIO OFF；
- no P5；
- no tuning/sweep；
- FAST VoxelMapManager inactive in Prob mode；
- I4 not started。

## Build/Test
- focused tests
- full build
- RC

## Files / Commits
- changed files
- SHAs
- final HEAD
- clean
- push

## Final State

若 I3 全部关闭：

```text
I0 = CLOSED / OWNER VERIFIED
I1 = CLOSED / OWNER VERIFIED
I2 = CLOSED / OWNER VERIFIED
I3 = CLOSED/PASS — Owner audit pending
I4–I8 = NOT STARTED

camera-off FAST-host Prob-LIO = operational
eee_01 canonical run          = complete
previous Prob-LIO comparison  = recorded

Next stage = I4 pointWithVar-compatible current-scan adapter
```

若轨迹/ATE 显示 material semantic mismatch：

```text
I3 = OPEN / SEMANTIC_MISMATCH
I4 = BLOCKED
```

禁止继续。

---

# 48. Final CLOSE Criteria

Prompt 3 只有在以下全部成立时才能声明 I3 CLOSED/PASS：

```text
I2 lifecycle seam genuinely closed

camera-off runtime uses:
  FAST scheduler
  ProbImuAdapter
  ProbESKF19
  Prob OctVox
  Super HKNN/QR
  P1–P4
  Super legacy association

FAST LiDAR map backend inactive
P5 absent
VIO inactive

all focused parity gates GREEN
full host build GREEN

clean committed source
full eee_01 completes
same evaluator runs
old/new raw trajectory comparison produced
old/new ATE comparison produced

no tuning/sweep
SPEC/EVIDENCE/HISTORY consistent
worktree clean
fast-forward push complete
```

跑通 bag 本身不等于通过；若运行时混用了 FAST map、P5、视觉或错误 config，必须判 INVALID。

---

# 49. Review Contract

Agent final report 不是接受 authority。

Owner/reviewer 会独立审查：
- I2 lifecycle corrective；
- production scheduler IMU-buffer seam；
- map-init；
- P0–P4 imported production code；
- frame/extrinsic；
- Super legacy gate vs P5 exclusion；
- P4 covariance terms；
- single-map authority；
- runtime backend selection；
- effective config；
- evaluator provenance；
- actual trajectory file；
- old/new trajectory comparison；
- ATE；
- runner/evidence identity。

不要为“测试全绿”优化；目标是以最小、干净的生产实现复现 canonical Prob-LIO semantics。
