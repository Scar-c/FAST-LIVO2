## Agent State Consensus

- Prompt6 start HEAD: `f85c7d0a9b7f7b62d19656d7c4f48aa003401238`.
- Final HEAD is recorded in `## Files/Commits`; branch is `prob-livo`.
- Prompt6 startup tree was clean at the expected start HEAD. The final tree is
  clean after the final evidence commit and fast-forward push.
- Prompt registration:
  `prompts/prob_livo/prompt6_i3_numeric_close_i4_pointwithvar_adapter.md`.
  SHA256: `86fcd4a4fef39c3aab705bc0c204c93a673cb2b56f6f2351583b8693de11621`.
- Reference implementation remained
  `/home/lc/super_livo/ref/Super-LIO` at
  `9fc949f46291c0fa76e5b7cdb372c940eb4b3f6e`.
- G-P6.0: PASS. The final host build and I1/I2/I3/P4/I4 focused tests all
  returned zero. There is no independent P5 executable target; Prompt5's
  already-closed evidence was retained and no P5 code was reopened.

## 58-IMU Fixture

The fixture is the exact first 58 `/imu/imu` messages from
`/home/lc/super_livo/bag/NTU/eee_01/eee_01.bag`, read in bag record order.
The final sample timestamp is `1609059013.2591093`. The diagnostic producer is
`tools/prob_livo/attribute_i3_init_numeric.py`; the captured result is
`spec/prob_livo/evidence/i3_numeric_decomposition.json`.

M1–M4 use the same physical samples and preserve the two source operation
orders without algebraic rewriting:

```text
legacy:   mean += (sample - mean) / n
migrated: mean = (mean * count + sample) / (count + 1)
```

## M1–M4

The delta columns are L2 norm for the acceleration/gyro vectors and absolute
scalar difference for `imu_scale`, all relative to M1.

| Variant | mean_acc | mean_gyro | imu_scale | delta acc / gyro / scale to M1 |
|---|---|---|---:|---:|
| M1 float + legacy | `(0.8442454934120178, 0.02315736748278141, -9.578536033630371)` | `(0.003033317625522614, 0.003774180775508285, 0.002952069276943804)` | `1.0186052322387695` | `0 / 0 / 0` |
| M2 float + migrated | `(0.8442453145980835, 0.02315737307071686, -9.578539848327637)` | `(0.003033318324014545, 0.00377418170683086, 0.002952069044113159)` | `1.0186047554016113` | `3.818890005813027e-06 / 1.1872079953534493e-09 / 4.76837158203125e-07` |
| M3 double + legacy | `(0.8442454019497182, 0.02315736831374594, -9.578537381928541)` | `(0.003033317496060747, 0.003774181543247261, 0.002952068698379725)` | `1.0186051084624186` | `1.3513970541561463e-06 / 9.700102592533295e-10 / 1.2377635094651396e-07` |
| M4 double + migrated | `(0.8442454019497181, 0.02315736831374595, -9.578537381928543)` | `(0.003033317496060747, 0.003774181543247259, 0.002952068698379725)` | `1.0186051084624184` | `1.351397055935948e-06 / 9.700102578224582e-10 / 1.2377635116855856e-07` |

## SO(3) Split

Using the same M4 physical mean, the source-faithful float and double
gravity-alignment/yaw results are:

```text
R_float =
  [ 0.9961380362510681, -0.0002121735742548481,  0.08779922127723694 ]
  [ 0.0000000992238895, -0.9999970793724060,   -0.002417638199403882 ]
  [ 0.08779947459697723, 0.002408310072496533, -0.9961351156234741 ]

R_double =
  [ 0.9961382417440776, -0.0002122641176189030,  0.08779839565140932 ]
  [ 0.0000000000000001, -0.9999970775431354,   -0.002417623872411485 ]
  [ 0.08779865223918326, 0.002408287593462494, -0.9961353305730349 ]
```

`||Log(R_float^T R_double)|| = 0.0005217556475790149 rad`.

The corresponding initial gravity values are:

```text
float:  (-0.8599526882171631, -0.023588214069604874, 9.756747245788574)
double: (-0.8599526792218906, -0.023588213662927366, 9.756747108830652)
```

Initial bias is `bg_float = M1.mean_gyro` and `bg_double = M4.mean_gyro`.
The initial P18 maximum absolute delta is `1.351996054173299e-11`.

## Attribution

- Scalar-width contribution, holding recurrence intent fixed: M1→M3 gives
  `1.3513970541561463e-06` acc, `9.700102592533295e-10` gyro, and
  `1.2377635094651396e-07` scale; M2→M4 gives
  `2.4679500454773262e-06` acc, `9.120299839861628e-10` gyro, and
  `3.5306080703456644e-07` scale.
- Recurrence-order contribution, holding scalar width fixed: M1→M2 gives
  `3.818890005813027e-06` acc, `1.1872079953534493e-09` gyro, and
  `4.76837158203125e-07` scale; M3→M4 gives
  `1.7798364309050242e-15` acc, `1.788112030672749e-18` gyro, and
  `2.220446049250313e-16` scale.
- SO(3) precision contribution: the measured rotation split is
  `0.0005217556475790149 rad`; the gravity and P18 deltas are listed above.

No sole cause is claimed. The measured decomposition confirms that both
scalar width and operation order can contribute, with the SO(3) split being a
separate finite-precision effect.

## G-I3.N1 / G-I3.N2

- G-I3.N1: PASS. Scalar width, recurrence order, and SO(3) precision were
  separated on the bounded fixture; no input, map, TBB, or lifecycle variable
  was changed.
- G-I3.N2: PASS. All variants use identical 58 samples, the same gravity
  normalization and bias definitions, and the same P18 covariance semantics.
  Only finite precision and recurrence operation order differ.

## I3 Decision

```text
I3 = CLOSED / OWNER VERIFIED
classification = NUMERIC_IMPLEMENTATION_DIFFERENCE_CONFIRMED
legacy = float ESKF/init numeric implementation
FAST host = double shared-state/ESKF/init numeric implementation
remaining difference = finite precision / operation ordering
semantics = equivalent
```

Production was not downgraded from double to float.

## FAST pointWithVar Source Audit

FAST defines the authoritative type in `include/common_lib.h:106-127`:

```text
point_b, point_i, point_w: Eigen::Vector3d
var_nostate, body_var, var, point_crossmat: Eigen::Matrix3d
normal: Eigen::Vector3d
```

The inline comments identify `point_b` as LiDAR body, `point_i` as IMU body,
and `point_w` as world. The first real consumer-facing production function is
`VIOManager::processFrame(cv::Mat&, vector<pointWithVar>&, ... , double)` at
`src/vio.cpp:1786`; the host call is `src/LIVMapper.cpp:387` in the camera/VIO
path. Inside that path `generateVisualMapPoints` consumes `point_w`, requires
`normal != 0`, and stores `pt_var.var` as the visual point covariance at
`src/vio.cpp:804-890`.

FAST's native LIO producer fills `point_b`, `point_w`, `body_var`, and `var` in
`src/voxel_map.cpp:338-453`; its `var` also includes pose covariance. That
native producer is not reused as Prob LIO authority. The Prob production
source is `ProbImuAdapter::prob_scan_undistort_imu` (scan-end IMU/body frame),
and `ComputeBodyCovListWithExtrinsic` provides P1 sensor covariance in that
same IMU frame at `include/prob_livo/super_native/prob_geometry_p0_p4.h:166-185`.

## Adapter Contract

| Field | Meaning | Frame | Unit | Producer | Lifetime | Consumer | Validity |
|---|---|---|---|---|---|---|---|
| `point_i` | authoritative undistorted current point | IMU body at scan end | m | Prob scan + P1 boundary | current scan buffer | future FAST visual path | valid |
| `point_b` | inverse-extrinsic LiDAR view of `point_i` | LiDAR body | m | I4 adapter | current scan buffer | FAST `pointWithVar` consumers | valid |
| `point_w` | current point transformed by shared x19 pose | world | m | I4 adapter | current scan buffer | visual map preparation | valid |
| `body_var` | P1 sensor-point covariance | IMU body | m² | `body_covariances_` | current scan buffer | compatibility/body view | valid |
| `var_nostate` | world-rotated sensor covariance, no state term | world | m² | `R_WI Sigma_I R_WI^T` | current scan buffer | visual covariance view | valid |
| `var` | same world sensor covariance used by future visual consumer | world | m² | I4 adapter | current scan buffer | `generateVisualMapPoints` | valid; no pose covariance |
| `point_crossmat` | skew matrix of `point_i` | IMU-body point | m in entries | I4 adapter | current scan buffer | later geometry compatibility | valid |
| `normal` | no plane normal is available in I4 | none | none | zero sentinel only | current scan buffer | no I4 consumer | invalid/unavailable |

The exact formulas are:

```text
point_i = p_I
point_b = R_IL^T (p_I - t_IL)
point_w = R_WI p_I + t_WI
body_var = Sigma_I
var_nostate = var = R_WI Sigma_I R_WI^T
point_crossmat = skew(point_i)
```

`normal == 0` is not a valid normal; capability is separately represented by
`normals_available = false`. The sidecar identity buffer preserves fields not
present in FAST's type:

| Sidecar | Meaning |
|---|---|
| `scan_id` | monotonically assigned current RUN scan identity |
| `source_index` | stable index at the authoritative Prob downsample stage |
| `intensity` | source point intensity |
| `relative_time_ms` | source point curvature/relative time in milliseconds |

## Implementation

- Added `include/prob_livo/prob_point_with_var_adapter.h` and
  `src/prob_livo/prob_point_with_var_adapter.cpp`.
- `ProbPointWithVarAdapter::Build` consumes the current Prob
  `downsampled_scan_`, `points_body_`, and P1 `body_covariances_` after the
  shared ProbESKF update. It does not call `Process2`, undistort again, or
  query FAST's `VoxelMapManager`.
- `ProbLioBackend::ProcessRun` builds the temporary output before `UpdateMap`
  and records `adapted_scans`/`adapted_points`. The backend exposes const
  current-scan accessors for the future real consumer seam.
- Camera-OFF `LIVMapper::handleProbLio` does not copy the buffer into FAST's
  `_pv_list`; that container is reserved for actual VIO execution. This keeps
  the adapter dormant while the backend still produces its operational output.
- No adapter-owned filter, P19, OctVox, plane provider, `feat_map`, or
  trajectory state was added.

## Gates

- G-I4.1 field contract: PASS — all FAST fields and sidecar metadata have an
  explicit frame, unit, producer, lifetime, consumer, and validity contract.
- G-I4.2 coordinate parity: PASS — identity, translation, rotation, coupled
  SE(3), inverse extrinsic, point order, and wrong-direction/double-transform
  negatives are covered by the focused fixture.
- G-I4.3 covariance parity: PASS — full 3x3 checks cover `Sigma_I`,
  `R_WI Sigma_I R_WI^T`, wrong-direction and double-rotation negatives; no
  pose covariance is added.
- G-I4.4 identity/order: PASS — source index, scan identity, intensity,
  relative time, output count, order, and input isolation are covered.
- G-I4.5 no P5/duplicate map: PASS — source audit shows no FAST map query,
  second LiDAR map, pose covariance, plane association, P5 gate, `feat_map`
  update, or VIO call in the adapter/backend integration.
- G-I4.6 real consumer compatibility: PASS — the focused test binds the
  exact production `VIOManager::processFrame` member-function type accepting
  `std::vector<pointWithVar>`. Camera/VIO itself was not enabled.
- G-I4.7 dormant runtime integration: PASS — final clean offline run records
  `adapted_scans=3981` and `adapted_points=13787674`, with no per-point logs.
- G-I4.8 LIO invariance: PASS — adapter-ON and pre-adapter baseline runs both
  have 3981 rows and byte-identical trajectory SHA
  `d06e472b04f7d304d1462b30b2077766f62bf7047cd4247f909c96b3ca277f03`.
  `map_update_inserts`, downsampled points, HKNN, QR, weighted measurement,
  and lifecycle counts are identical; only the new adapter counters differ.

## Build/Test

Final build:

```bash
cmake --build /home/lc/super_livo/build --target all -- -j4
```

Return code: `0`. The build uses `-j4` to stay within the host memory budget;
the offline runtime remains TBB maximum parallelism 32.

Focused commands, all return code `0`:

```bash
/home/lc/super_livo/devel/lib/fast_livo/prob_livo_i1_tests
/home/lc/super_livo/devel/lib/fast_livo/prob_livo_i2_tests
/home/lc/super_livo/devel/lib/fast_livo/prob_livo_i3_tests
/home/lc/super_livo/devel/lib/fast_livo/prob_livo_p4_tests
/home/lc/super_livo/devel/lib/fast_livo/prob_livo_i4_tests
```

The I4 runner reports `[PASS] G-I4 pointWithVar current-scan adapter gates
checks=47`. The clean production seam run was the repository-owned
in-process runner with `super_ntu_legacy`, camera OFF, and TBB32:

```text
run: results/prob_livo/runs/prompt6_i4_super_input_clean
rows: 3981
ATE: 0.09099574805341126 m
matched GT: 3329
trajectory SHA256: d06e472b04f7d304d1462b30b2077766f62bf7047cd4247f909c96b3ca277f03
callbacks/emitted/attempted/success/reject: 3987/3986/3986/3986/0
IMU_INIT/MAP_INIT/RUN: 1/4/3981
adapted_scans/adapted_points: 3981/13787674
TBB maximum parallelism: 32
return code: 0
```

## Scope Audit

- Camera/VIO runtime remained OFF.
- I5 was not started; no plane-provider logic was implemented.
- P5 association was not enabled or reopened.
- No second geometry map, FAST-native LIO covariance, duplicate undistortion,
  or extra filter/state owner was added.
- The adapter has no generic validator flood, fallback estimator, or
  per-point/per-neighbor production logging.
- The only complete bag run used to close I4 was camera-OFF and produced the
  same LIO trajectory before and after adapter integration.

## Files/Commits

Prompt6 implementation and evidence files:

```text
prompts/prob_livo/prompt6_i3_numeric_close_i4_pointwithvar_adapter.md
spec/prob_livo/evidence/i3_numeric_decomposition.json
tools/prob_livo/attribute_i3_init_numeric.py
include/prob_livo/prob_point_with_var_adapter.h
src/prob_livo/prob_point_with_var_adapter.cpp
tests/prob_livo/test_i4_point_with_var_adapter.cpp
include/prob_livo/prob_lio_backend.h
src/prob_livo/prob_lio_backend.cpp
src/LIVMapper.cpp
CMakeLists.txt
spec/prob_livo/SPEC.md
spec/prob_livo/EVIDENCE_INDEX.md
spec/prob_livo/HISTORY.md
spec/prob_livo/PROMPT6_EVIDENCE.md
```

Commits:

```text
3ae8f8e test(prob-livo): close initialization numeric attribution
15faeed feat(prob-livo): add pointWithVar current-scan adapter
a3ef69f fix(prob-livo): keep I4 adapter dormant for camera-off LIO
final evidence/docs commit: recorded after this report is committed
```

The final commit, clean status, and fast-forward `origin/prob-livo` result are
recorded in the final handoff after the evidence commit.

## Final State

```text
I0 = CLOSED / OWNER VERIFIED
I1 = CLOSED / OWNER VERIFIED
I2 = CLOSED / OWNER VERIFIED
I3 = CLOSED / OWNER VERIFIED
I4 = CLOSED/PASS — Owner audit pending
I5–I8 = NOT STARTED

pointWithVar-compatible current-scan adapter = operational
camera/VIO runtime                            = OFF

Next stage = I5 ProbPlaneProvider
```
